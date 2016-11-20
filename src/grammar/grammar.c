/*
 * /obelix/src/grammar.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libgrammar.h"

int grammar_debug = 0;

static grammar_t * _grammar_new(grammar_t *, va_list);
static void        _grammar_free(grammar_t *);
static char *      _grammar_tostring(grammar_t *);
static grammar_t * _grammar_set(grammar_t *, char *, data_t *);
static char *      _grammar_tostring(grammar_t *);

static list_t *    _grammar_dump_nonterminal_reducer(nonterminal_t *, list_t *);
static grammar_t * _grammar_dump_pre(ge_dump_ctx_t *);
static grammar_t * _grammar_dump_get_children(grammar_t *, list_t *);
static grammar_t * _grammar_dump_post(ge_dump_ctx_t *);

static vtable_t _vtable_grammar[] = {
  { .id = FunctionNew,      .fnc = (void_t) _grammar_new },
  { .id = FunctionFree,     .fnc = (void_t) _grammar_free },
  { .id = FunctionToString, .fnc = (void_t) _grammar_tostring },
  { .id = FunctionSet,      .fnc = (void_t) _grammar_set },
  { .id = FunctionUsr2,     .fnc = (void_t) _grammar_dump_pre },
  { .id = FunctionUsr3,     .fnc = (void_t) _grammar_dump_get_children },
  { .id = FunctionUsr4,     .fnc = (void_t) _grammar_dump_post },
  { .id = FunctionNone,     .fnc = NULL }
};

grammar_element_type_t Grammar = -1;

/* ------------------------------------------------------------------------ */

extern void grammar_init(void) {
  if (GrammarAction < 0) {
    logging_register_category("grammar", &grammar_debug);
    grammar_action_register();
    grammar_element_register();
    nonterminal_register();
    rule_register();
    rule_entry_register();

    Grammar = typedescr_create_and_register(
        Grammar, "grammar", _vtable_grammar, NULL);
    typedescr_set_size(Grammar, grammar_t);
    typedescr_assign_inheritance(Grammar, GrammarElement);
  }
}

/* -- G R A M M A R ------------------------------------------------------- */

grammar_t * _grammar_new(grammar_t *grammar, va_list args) {
  va_arg(args, grammar_t *); // grammar
  va_arg(args, ge_t *); // owner

  grammar -> entrypoint = NULL;
  grammar -> prefix = NULL;
  grammar -> libs = NULL;
  grammar -> strategy = ParsingStrategyTopDown;
  grammar -> dryrun = FALSE;

  grammar -> keywords = intdata_dict_create();
  grammar -> nonterminals = strdata_dict_create();
  grammar -> lexer = lexer_config_create();

  return grammar;
}

void _grammar_free(grammar_t *grammar) {
  if (grammar) {
    dict_free(grammar -> nonterminals);
    dict_free(grammar -> keywords);
    lexer_config_free(grammar -> lexer);
    free(grammar -> prefix);
  }
}

char * _grammar_tostring(grammar_t *grammar) {
  return "Grammar";
}

grammar_t * _grammar_set(grammar_t *g, char *name, data_t *value) {
  char       *val = NULL;

  if (data_type(value) == Token) {
    val = token_token((token_t *) value);
  } else if (value) {
    val = data_tostring(value);
  }
  if (!strcmp(name, LIB_STR)) {
    if (val) {
      resolve_library(val);
      if (!g -> libs) {
        g -> libs = array_create(4);
        array_set_free(g -> libs, (free_t) free);
      }
      array_push(g -> libs, strdup(val));
    }
  } else if (!strcmp(name, LEXER_STR)) {
    if (!lexer_config_add_scanner(g -> lexer, val)) {
      g = NULL;
    }
  } else if (!strcmp(name, PREFIX_STR)) {
    g -> prefix = (val) ? strdup(val) : strdup("");
  } else if (!strcmp(name, STRATEGY_STR)) {
    if (val) {
      if (!strncmp(val, "topdown", 7)|| !strncmp(val, "ll(1)", 5)) {
        grammar_set_parsing_strategy(g, ParsingStrategyTopDown);
      } else if (!strncmp(val, "bottomup", 8) || !strncmp(val, "lr(1)", 5)) {
        grammar_set_parsing_strategy(g, ParsingStrategyBottomUp);
      }
    }
  } else {
    g = NULL;
  }
  return g;
}

/* ------------------------------------------------------------------------ */

list_t * _grammar_dump_nonterminal_reducer(nonterminal_t *nt, list_t *children) {
  grammar_t *grammar = nonterminal_get_grammar(nt);

  if (grammar -> entrypoint &&
      !strcmp(nt -> name, grammar -> entrypoint -> name)) {
    list_unshift(children, nt);
  } else {
    list_append(children, nt);
  }
  return children;
}

grammar_t * _grammar_dump_get_children(grammar_t *grammar, list_t *children) {
  dict_reduce_values(grammar -> nonterminals,
                     (reduce_t) _grammar_dump_nonterminal_reducer,
                     children);
  return grammar;
}

grammar_t * _grammar_dump_pre(ge_dump_ctx_t *ctx) {
  int        ix;
  char      *lib;
  char      *escaped;
  grammar_t *grammar = (grammar_t *) ctx -> obj;

  printf("#include <grammar.h>\n");
  printf("#include <datastack.h>\n\n");
  if (grammar -> lexer) {
    lexer_config_dump(grammar -> lexer);
  }

  printf("\n"
         "grammar_t * build_grammar() {\n"
         "  grammar_t      *grammar;\n"
         "  ge_t           *ge;\n"
         "  ge_t           *owner = NULL;\n"
         "  datastack_t    *stack;\n"
         "  data_t         *value;\n"
         "\n"
         "  stack = datastack_create(\"build_grammar\");\n"
         "  grammar = grammar_create();\n");

  if (grammar -> prefix && grammar -> prefix[0]) {
    /*
     * No need to escape - can't have quotes or backslashes in C function names
     */
    printf("  value = (data_t *) str_wrap(\"%s\");\n"
           "  grammar_set_variable(grammar, PREFIX_STR, value);\n"
           "  data_free(value);\n",
           grammar -> prefix);
  }
  if (grammar -> libs && array_size(grammar -> libs)) {
    for (ix = array_size(grammar -> libs) - 1; ix >= 0; ix--) {
      lib = (char *) array_get(grammar -> libs, ix);
      escaped = c_escape(lib);
      printf("  value = (data_t *) str_wrap(\"%s\");\n"
             "  grammar_set_variable(grammar, LIB_STR, value);\n"
             "  data_free(value);\n",
             escaped);
      free(escaped);
    }
  }
  if (grammar -> lexer) {
    printf("  grammar -> lexer = lexer_config_build(lexer_config_create());\n");
  }
  printf("  ge = (ge_t *) grammar;\n\n");
  return grammar;
}

grammar_t * _grammar_dump_post(ge_dump_ctx_t *ctx) {
  printf("  assert(ge == (ge_t *) grammar);\n"
         "  grammar_analyze(grammar);\n"
         "  datastack_free(stack);\n"
         "  return grammar;\n"
         "}\n");
  return (grammar_t *) ctx -> obj;
}

/* ------------------------------------------------------------------------ */

void _grammar_get_firsts_visitor(entry_t *entry) {
  nonterminal_t *nonterminal;

  nonterminal = (nonterminal_t *) entry -> value;
//  if (grammar_debug) {
//    debug("Building FIRST sets for rule %s", nonterminal -> name);
//  }
  _nonterminal_get_firsts(nonterminal);
}

/*
    Rules for Follow Sets

    First put $ (the end of input marker) in Follow(S) (S is the start symbol)
    If there is a production A → aBb, (where a can be a whole string)
      then everything in FIRST(b) except for ε is placed in FOLLOW(B).
    If there is a production A → aB,
      then everything in FOLLOW(A) is in FOLLOW(B)
    If there is a production A → aBb, where FIRST(b) contains ε,
      then everything in FOLLOW(A) is in FOLLOW(B)
*/
void * _grammar_follows_reducer(entry_t *entry, int *current_sum) {
  nonterminal_t *nonterminal, *nt;
  int            i, j, k;
  rule_t        *rule;
  rule_entry_t  *rule_entry, *it, *next;
  set_t         *follows, *f, *next_firsts;

  nonterminal = (nonterminal_t *) (entry -> value);

//  if (grammar_debug) {
//    debug("Building FOLLOW sets for rule %s", nonterminal -> name);
//  }
  follows = _nonterminal_get_follows(nonterminal);
  for (i = 0; i < array_size(nonterminal -> rules); i++) {
    rule = nonterminal_get_rule(nonterminal, i);
    for (j = 0; j < array_size(rule -> entries); j++) {
      rule_entry = rule_get_entry(rule, j);
      if (!rule_entry -> terminal) {
        next_firsts = intset_create();
        next = NULL;
        for (k = j + 1; k < array_size(rule -> entries); k++) {
          it = rule_get_entry(rule, k);
          debug(grammar, "--> k: %d it: '%s'", k, data_tostring((data_t *) it));
          if (!next) {
            next = it;
          }
          _rule_entry_get_firsts(it, next_firsts);
          if (!set_has_int(next_firsts, TokenCodeEmpty)) {
            break;
          }
        }
        nt = grammar_get_nonterminal(
               nonterminal_get_grammar(rule_entry),
               rule_entry -> nonterminal);
        f = _nonterminal_get_follows(nt);
        if ((next == NULL) || set_has_int(next_firsts, TokenCodeEmpty)) {
          set_union(f, follows);
        }
        set_remove_int(next_firsts, TokenCodeEmpty);
        set_union(f, next_firsts);
        set_free(next_firsts);
      }
    }
  }
  *current_sum += set_size(follows);
  return current_sum;
}

int * _grammar_check_LL1_reducer(entry_t *entry, int *ok) {
  nonterminal_t *nonterminal;

  if (*ok) {
    nonterminal = (nonterminal_t *) entry -> value;
    *ok = _nonterminal_check_LL1(nonterminal);
  }
  return ok;
}

void _grammar_build_parse_table_visitor(entry_t *entry) {
  nonterminal_t *nonterminal;

  nonterminal = (nonterminal_t *) entry -> value;
  _nonterminal_build_parse_table(nonterminal);
}

function_t * _grammar_resolve_function(grammar_t *grammar, char *prefix, char *func_name) {
  char       *fname = NULL;
  function_t *ret;

  if (prefix && prefix[0]) {
    asprintf(&fname, "%s%s", prefix, func_name);
  } else {
    fname = func_name;
  }
  ret = function_create(fname, NULL);
  if (!ret -> fnc) {
    function_free(ret);
    ret = NULL;
  }
  if (prefix && prefix[0]) {
    free(fname);
  }
  return ret;
}

/* -- G R A M M A R  P U B L I C  F U N C T I O N S -----------------------*/

grammar_t * grammar_create() {
  grammar_init();
  return (grammar_t *) data_create(Grammar, NULL, NULL);
}

grammar_t * grammar_set_parsing_strategy(grammar_t *grammar, strategy_t strategy) {
  grammar -> strategy = strategy;
  return grammar;
}

function_t * grammar_resolve_function(grammar_t *grammar, char *func_name) {
  function_t *ret = NULL;
  char       *prefix = grammar -> prefix;
  int         starts_with_prefix;

  if (grammar -> dryrun) {
    ret = function_create_noresolve(func_name);
    return ret;
  }

  starts_with_prefix = prefix && prefix[0] && !strncmp(func_name, prefix, strlen(prefix));
  if (!starts_with_prefix) {
    ret = _grammar_resolve_function(grammar, prefix, func_name);
    if (!ret && strncmp(func_name, "parser_", 7)) {
      ret = _grammar_resolve_function(grammar, "parser_", func_name);
    }
  }
  if (!ret) {
    ret = _grammar_resolve_function(grammar, NULL, func_name);
  }
  if (!ret) {
    error("Could not resolve function '%s'", func_name);
  }
  return ret;
}

grammar_t * grammar_analyze(grammar_t *grammar) {
  int sum, prev_sum, iter, ll_1;

  debug(grammar, "Building FIRST sets");
  dict_visit(grammar -> nonterminals, (visit_t) _grammar_get_firsts_visitor);

  debug(grammar, "Building FOLLOW sets");
  sum = 0;
  iter = 1;
  do {
    prev_sum = sum;
    sum = 0;
    dict_reduce(grammar -> nonterminals, (reduce_t) _grammar_follows_reducer, &sum);
    debug(grammar, "_grammar_analyze - build follows: iter: %d sum: %d", iter++, sum);
  } while (sum != prev_sum);

  debug(grammar, "Checking grammar for LL(1)-ness");
  debug(grammar, "Keywords: %s", dict_tostring(grammar -> keywords));
  ll_1 = 1;
  dict_reduce(grammar -> nonterminals, (reduce_t) _grammar_check_LL1_reducer, &ll_1);
  if (ll_1) {
    if (grammar_debug) {
      info("Grammar is LL(1)");
    }
    dict_visit(grammar -> nonterminals, (visit_t) _grammar_build_parse_table_visitor);
    debug(grammar, "Parse tables built");
  } else {
    error("Grammar is not LL(1)");
  }
  return (ll_1) ? grammar : NULL;
}
