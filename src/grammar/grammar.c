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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <data.h>
#include "libgrammar.h"

int grammar_debug = 0;

static grammar_t * _grammar_new(grammar_t *, va_list);
static void        _grammar_free(grammar_t *);
static char *      _grammar_tostring(grammar_t *);
static grammar_t * _grammar_set(grammar_t *, char *, data_t *);
static char *      _grammar_tostring(grammar_t *);

static list_t *    _grammar_dump_nonterminal_reducer(nonterminal_t *, list_t *);
static grammar_t * _grammar_dump_pre(grammar_t *);
static grammar_t * _grammar_dump_get_children(grammar_t *, list_t *);
static grammar_t * _grammar_dump_post(grammar_t *);

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
    grammaraction_register();
    grammarelement_register();
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
  grammar -> lexer = str_list_create();

  return grammar;
}

void _grammar_free(grammar_t *grammar) {
  if (grammar) {
    dict_free(grammar -> nonterminals);
    dict_free(grammar -> keywords);
    list_free(grammar -> lexer);
    free(grammar -> prefix);
  }
}

char * _grammar_tostring(grammar_t *grammar) {
  return "Grammar";
}

grammar_t * _grammar_set(grammar_t *g, char *name, data_t *value) {
  char       *val;

  if (!value) {
    return NULL;
  } else if (data_type(value) == Token) {
    val = token_token((token_t *) value);
  } else {
    val = data_tostring(value);
  }

  if (!strcmp(name, LIB_STR)) {
    resolve_library(val);
    if (!g -> libs) {
      g -> libs = array_create(4);
      array_set_free(g -> libs, (free_t) free);
    }
    array_push(g -> libs, strdup(val));
  } else if (!strcmp(name, PREFIX_STR)){
    g -> prefix = strdup(val);
  } else if (!strcmp(name, STRATEGY_STR)){
    if (!strncmp(val, "topdown", 7)|| !strncmp(val, "ll(1)", 5)) {
      grammar_set_parsing_strategy(g, ParsingStrategyTopDown);
    } else if (!strncmp(val, "bottomup", 8) || !strncmp(val, "lr(1)", 5)) {
      grammar_set_parsing_strategy(g, ParsingStrategyBottomUp);
    }
  } else if (!strcmp(name, LEXER_STR)){
    grammar_set_lexer_option(g, val);
//} else if (!strcmp(token_token(name), CASE_SENSITIVE_STR)){
//  grammar_set_lexer_option(g, LexerOptionCaseSensitive,
//                           data_parse(Bool, token_token(val)));
  } else {
    g = NULL;
  }
  return g;
}

/* ------------------------------------------------------------------------ */

list_t * _grammar_dump_nonterminal_reducer(nonterminal_t *nt, list_t *children) {
  grammar_t *grammar = nonterminal_get_grammar(nt);

  if (grammar -> entrypoint ||
      !strcmp(nt -> name, grammar -> entrypoint -> name)) {
    list_unshift(children, nt);
  } else {
    list_append(children, nt);
  }
  return children;
}

grammar_t * _grammar_dump_get_children(grammar_t *grammar, list_t *children) {
  dict_reduce(grammar -> nonterminals,
              (reduce_t) _grammar_dump_nonterminal_reducer,
              children);
  return grammar;
}

grammar_t * _grammar_dump_pre(grammar_t *grammar, char *prefix, char *variable) {
  int   ix;
  char *lib;

  printf("#include <grammar.h>\n"
         "\n"
         "grammar_t * build_grammar() {\n"
         "  grammar_t      *grammar;\n"
         "  nonterminal_t  *nonterminal;\n"
         "  rule_t         *rule;\n"
         "  rule_entry_t   *rule_entry;\n"
         "  token_t        *token_name, *token_value;\n"
         "  lexer_config_t *lexer_config;\n"
         "\n"
         "  grammar = grammar_create();\n");

  if (grammar -> prefix && grammar -> prefix[0]) {
    printf("  token_name = token_create(TokenCodeIdentifier, PREFIX_STR);\n"
           "  token_value = token_create(TokenCodeIdentifier, \"%s\");\n"
           "  grammar_set_option(grammar, token_name, token_value);\n"
           "  token_free(token_name);\n"
           "  token_free(token_value);\n",
           grammar -> prefix);
  }
  if (grammar -> libs && array_size(grammar -> libs)) {
    for (ix = array_size(grammar -> libs) - 1; ix >= 0; ix--) {
      lib = (char *) array_get(grammar -> libs, ix);
      printf("  token_name = token_create(TokenCodeIdentifier, LIB_STR);\n"
             "  token_value = token_create(TokenCodeDQuotedStr, \"%s\");\n"
             "  grammar_set_option(grammar, token_name, token_value);\n"
             "  token_free(token_name);\n"
             "  token_free(token_value);\n",
             lib);
    }
  }
  if (grammar -> lexer) {
    lexer_config_dump(grammar -> lexer);
    printf("  grammar -> lexer = lexer_config;\n");
  }
  printf("\n");
  return grammar;
}

grammar_t * _grammar_dump_post(grammar_t *grammar) {
  printf("  grammar_analyze(grammar);\n"
         "  return grammar;\n"
         "}\n");
  return grammar;
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

nonterminal_t * grammar_get_nonterminal(grammar_t *grammar, char *rule) {
  return (nonterminal_t *) dict_get(grammar -> nonterminals, rule);
}

grammar_t * grammar_set_parsing_strategy(grammar_t *grammar, strategy_t strategy) {
  grammar -> strategy = strategy;
  return grammar;
}

strategy_t grammar_get_parsing_strategy(grammar_t *grammar) {
  return grammar -> strategy;
}

grammar_t * grammar_set_lexer_option(grammar_t *grammar, char *value) {
  list_append(grammar -> lexer, strdup(value));
  return grammar;
}

//data_t * grammar_get_lexer_option(grammar_t *grammar, lexer_option_t option) {
//  return grammar -> lexer_options[(int) option];
//}

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

  if (grammar_debug) {
    debug("Building FIRST sets");
  }
  dict_visit(grammar -> nonterminals, (visit_t) _grammar_get_firsts_visitor);

  if (grammar_debug) {
    debug("Building FOLLOW sets");
  }
  sum = 0;
  iter = 1;
  do {
    prev_sum = sum;
    sum = 0;
    dict_reduce(grammar -> nonterminals, (reduce_t) _grammar_follows_reducer, &sum);
    if (grammar_debug) {
      debug("_grammar_analyze - build follows: iter: %d sum: %d", iter++, sum);
    }
  } while (sum != prev_sum);

  if (grammar_debug) {
    debug("Checking grammar for LL(1)-ness");
    debug("Keywords: %s", dict_tostring(grammar -> keywords));
  }
  ll_1 = 1;
  dict_reduce(grammar -> nonterminals, (reduce_t) _grammar_check_LL1_reducer, &ll_1);
  if (ll_1) {
    if (grammar_debug) {
      info("Grammar is LL(1)");
    }
    dict_visit(grammar -> nonterminals, (visit_t) _grammar_build_parse_table_visitor);
    if (grammar_debug) {
      debug("Parse tables built");
    }
  } else {
    error("Grammar is not LL(1)");
  }
  return (ll_1) ? grammar : NULL;
}
