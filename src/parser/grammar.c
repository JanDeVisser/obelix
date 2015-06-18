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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <grammar.h>
#include <list.h>
#include <logging.h>

int grammar_debug = 0;

static void                _grammar_init(void) __attribute__((constructor(102)));
static void                _ge_init(void) __attribute__((constructor));

static ge_t *              _ge_initialize(ge_t *, grammar_t *, ge_t *, grammar_element_type_t);
static void                _ge_free(ge_t *);

static void                _grammar_free(grammar_t *);
static char *              _grammar_tostring(grammar_t *);
static void                _grammar_get_firsts_visitor(entry_t *);
static int *               _grammar_check_LL1_reducer(entry_t *, int *);
static void                _grammar_build_parse_table_visitor(entry_t *);
static ge_t *              _grammar_set_option(ge_t *, token_t *, token_t *);
static function_t *        _grammar_resolve_function(grammar_t *, char *, char *);
static char *              _grammar_tostring(grammar_t *);

static void                _nonterminal_free(nonterminal_t *);
static set_t *             _nonterminal_get_firsts(nonterminal_t *);
static set_t *             _nonterminal_get_follows(nonterminal_t *);
static int                 _nonterminal_check_LL1(nonterminal_t *);
static void                _nonterminal_build_parse_table(nonterminal_t *);
static grammar_t *         _nonterminal_dump_terminal(unsigned int, grammar_t *);
static void                _nonterminal_dump(ge_t *);
static char *              _nonterminal_tostring(nonterminal_t *);

static void                _rule_free(rule_t *);
static char *              _rule_tostring(rule_t *);
static set_t *             _rule_get_firsts(rule_t *);
static set_t *             _rule_get_follows(rule_t *);
static void                _rule_build_parse_table(rule_t *);
static rule_t *            _rule_add_parse_table_entry(long, rule_t *);
static void                _rule_dump(ge_t *);
static char *              _rule_tostring(rule_t *);

static rule_entry_t *      _rule_entry_create(rule_t *, int, void *);
static void                _rule_entry_free(rule_entry_t *);
static char *              _rule_entry_tostring(rule_entry_t *);

static set_t *             _rule_entry_get_firsts(rule_entry_t *, set_t *);
static set_t *             _rule_entry_get_follows(rule_entry_t *, set_t *);

static data_t *            _ga_create(int, va_list);
static char *              _ga_tostring(grammar_action_t *);
static void                _ga_free(grammar_action_t *);

static vtable_t _vtable_ga[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _ga_free },
  { .id = FunctionCmp,      .fnc = (void_t) grammar_action_cmp },
  { .id = FunctionHash,     .fnc = (void_t) grammar_action_hash },
  { .id = FunctionToString, .fnc = (void_t) _ga_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_ga = {
  .type      = -1,
  .type_name = "grammaraction",
  .vtable    = _vtable_ga
};

static vtable_t _vtable_ge[] = {
  { .id = FunctionFree, .fnc = (void_t) _ge_free },
  { .id = FunctionNone, .fnc = NULL }
};

static typedescr_t _typedescr_ge = {
  .type      = -1,
  .type_name = "grammarelement",
  .vtable    = _vtable_ge
};

static vtable_t _vtable_grammar[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _grammar_free },
  { .id = FunctionToString, .fnc = (void_t) _grammar_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_grammar = {
  .type      = -1,
  .type_name = "grammar",
  .vtable    = _vtable_grammar
};

static vtable_t _vtable_nonterminal[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _nonterminal_free },
  { .id = FunctionToString, .fnc = (void_t) _nonterminal_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_nonterminal = {
  .type      = -1,
  .type_name = "nonterminal",
  .vtable    = _vtable_nonterminal
};

static vtable_t _vtable_rule[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _rule_free },
  { .id = FunctionToString, .fnc = (void_t) _rule_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_rule = {
  .type      = -1,
  .type_name = "rule",
  .vtable    = _vtable_rule
};

static vtable_t _vtable_rule_entry[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _rule_entry_free },
  { .id = FunctionToString, .fnc = (void_t) _rule_entry_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_rule_entry = {
  .type      = -1,
  .type_name = "rule_entry",
  .vtable    = _vtable_rule_entry
};

grammar_element_type_t GrammarAction = -1;
grammar_element_type_t GrammarElement = -1;
grammar_element_type_t Grammar = -1;
grammar_element_type_t NonTerminal = -1;
grammar_element_type_t Rule = -1;
grammar_element_type_t RuleEntry = -1;
grammar_element_type_t Terminal = -1;

/* ------------------------------------------------------------------------ */

void _grammar_init(void) {
  logging_register_category("grammar", &grammar_debug);
}

void _ge_init(void) {
  GrammarAction = typedescr_register(&_typedescr_ga);
  GrammarElement = typedescr_register(&_typedescr_ge);
  _typedescr_grammar.inherits[0] = GrammarElement;
  _typedescr_nonterminal.inherits[0] = GrammarElement;
  _typedescr_rule.inherits[0] = GrammarElement;
  _typedescr_rule_entry.inherits[0] = GrammarElement;

  Grammar = typedescr_register(&_typedescr_grammar);
  NonTerminal = typedescr_register(&_typedescr_nonterminal);
  Rule = typedescr_register(&_typedescr_rule);
  RuleEntry = typedescr_register(&_typedescr_rule_entry);
}

/* -- G R A M M A R _ A C T I O N ----------------------------------------- */

void _ga_free(grammar_action_t *grammar_action) {
  if (grammar_action) {
    function_free(grammar_action -> fnc);
    data_free(grammar_action -> data);
    free(grammar_action);
  }
}

char * _ga_tostring(grammar_action_t *grammar_action) {
  if (!grammar_action -> _d.str) {
    if (grammar_action -> data) {
      asprintf(&grammar_action -> _d.str, "%s [%s]",
               function_tostring(grammar_action -> fnc),
               data_tostring(grammar_action -> data));
    } else {
      asprintf(&grammar_action -> _d.str, "%s",
               function_tostring(grammar_action -> fnc));
    }
  }
  return NULL;
}

/* ------------------------------------------------------------------------ */

grammar_action_t * grammar_action_create(function_t *fnc, data_t *data) {
  grammar_action_t *ret;

  ret = data_new(GrammarAction, grammar_action_t);
  ret -> fnc = function_copy(fnc);
  ret -> data = data_copy(data);
  return ret;
}

int grammar_action_cmp(grammar_action_t *ga1, grammar_action_t *ga2) {
  int ret;

  ret = function_cmp(ga1 -> fnc, ga2 -> fnc);
  if (!ret) {
    ret = data_cmp(ga1 -> data, ga2 -> data);
  }
  return ret;
}

unsigned int grammar_action_hash(grammar_action_t *grammar_action) {
  return hashblend(function_hash(grammar_action -> fnc),
                   data_hash(grammar_action -> data));
}

/* -- G R A M M A R _ E L E M E N T --------------------------------------- */

ge_t * _ge_initialize(ge_t *ge, grammar_t *grammar, ge_t *owner, grammar_element_type_t type) {
  va_list      args;

  data_settype(&ge -> _d, type);
  ge -> grammar = grammar;
  ge -> owner = owner;
  ge -> actions = data_list_create();
  ge -> variables = strtoken_dict_create();
  return ge;
}

void _ge_free(ge_t *ge) {
  free_t free_fnc;

  if (ge) {
    dict_free(ge -> variables);
    list_free(ge -> actions);
    free(ge);
  }
}

char * _ge_dump_variable(entry_t *entry, char *varname) {
  token_t *tok = (token_t *) entry -> value;

  printf("  dict_put(%s -> ge.variables, strdup(\"%s\"), token_create(%d, \"%s\"));\n",
         varname, (char *) entry -> key, token_code(tok), token_token(tok));
  return varname;
}

char ** _ge_dump_action(grammar_action_t *ga, char **ctx) {
  function_t *fnc = ga -> fnc;
  data_t     *d = ga -> data;
  char       *data;

  if (d) {
    asprintf(&data, "data_parse(%d, \"%s\")", data_type(d), data_tostring(d));
  } else {
    data = strdup("NULL");
  }

  printf("  %s_add_action(%s,\n"
         "    grammar_action_create(\n"
         "      grammar_resolve_function(grammar, \"%s\"), %s));\n",
         ctx[0], ctx[1], name_tostring_sep(fnc -> name, ":"), data);
  free(data);
  return ctx;
}

void _ge_dump(ge_t *ge, char *prefix, char *varname) {
  char *ctx[2];

  dict_reduce(ge -> variables, (reduce_t) _ge_dump_variable, varname);
  ctx[0] = prefix;
  ctx[1] = varname;
  list_reduce(ge -> actions, (reduce_t) _ge_dump_action, ctx);
}

/* ------------------------------------------------------------------------ */

ge_t * ge_add_action(ge_t *ge, grammar_action_t *action) {
  list_push(ge -> actions, action);
  return ge;
}

ge_t * ge_set_option(ge_t *ge, token_t *name, token_t *val) {
  function_t *fnc;
  data_t     *data;
  char       *namestr;

  if (grammar_debug) {
    debug("  Setting option %s on grammar element %s", token_tostring(name), ge_tostring(ge));
  }
  namestr = token_token(name);
  if (!ge -> set_option_delegate || !ge -> set_option_delegate(ge, name, val)) {
    if (*namestr == '_') {
      if (val) {
        dict_put(ge -> variables, strdup(namestr + 1), token_copy(val));
      } else {
        error("ge_set_option: Cannot set grammar option '%s' on %s",
              namestr, ge_tostring(ge));
        ge = NULL;
      }
    } else {
      fnc = grammar_resolve_function(ge -> grammar, namestr);
      if (fnc) {
        data = token_todata(val);
        ge_add_action(ge, grammar_action_create(fnc, data));
        data_free(data);
        function_free(fnc);
      } else {
        error("ge_set_option: Cannot set grammar option '%s' on %s",
              namestr, ge_tostring(ge));
        ge = NULL;
      }
    }
  }
  return ge;
}

/* -- G R A M M A R ------------------------------------------------------- */

void _grammar_free(grammar_t *grammar) {
  if (grammar) {
    dict_free(grammar -> nonterminals);
    dict_free(grammar -> keywords);
    array_free(grammar -> lexer_options);
    free(grammar -> prefix);
  }
}

char * _grammar_tostring(grammar_t *grammar) {
  return "Grammar";
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

ge_t * _grammar_set_option(ge_t *ge, token_t *name, token_t *val) {
  int         b;
  grammar_t  *g = (grammar_t *) ge;
  char       *str;
  function_t *fnc;

  if (!val) return NULL;

  if (!strcmp(token_token(name), LIB_STR)) {
    resolve_library(token_token(val));
    if (!g -> libs) {
      g -> libs = array_create(4);
      array_set_free(g -> libs, (free_t) free);
    }
    array_push(g -> libs, strdup(token_token(val)));
  } else if (!strcmp(token_token(name), PREFIX_STR)){
    g -> prefix = strdup(token_token(val));
  } else if (!strcmp(token_token(name), STRATEGY_STR)){
    str = token_token(val);
    if (!strncmp(str, "topdown", 7)|| !strncmp(str, "ll(1)", 5)) {
      grammar_set_parsing_strategy(g, ParsingStrategyTopDown);
    } else if (!strncmp(str, "bottomup", 8) || !strncmp(str, "lr(1)", 5)) {
      grammar_set_parsing_strategy(g, ParsingStrategyBottomUp);
    }
  } else if (!strcmp(token_token(name), IGNORE_STR)){
    str = token_token(val);
    if (strstr(str, "whitespace")) {
      grammar_set_lexer_option(g, LexerOptionIgnoreWhitespace, 1);
    }
    if (strstr(str, "newlines")) {
      grammar_set_lexer_option(g, LexerOptionIgnoreNewLines, 1);
    }
    if (strstr(str, "allwhitespace")) {
      grammar_set_lexer_option(g, LexerOptionIgnoreAllWhitespace, 1);
    }
  } else if (!strcmp(token_token(name), CASE_SENSITIVE_STR)){
    grammar_set_lexer_option(g, LexerOptionCaseSensitive,
                             atob(token_token(val)));
  } else if (!strcmp(token_token(name), HASHPLING_STR)){
    grammar_set_lexer_option(g, LexerOptionHashPling,
                             atob(token_token(val)));
  } else if (!strcmp(token_token(name), SIGNED_NUMBERS_STR)){
    grammar_set_lexer_option(g, LexerOptionSignedNumbers,
                             atob(token_token(val)));
  } else {
    ge = NULL;
  }
  return ge;
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
    free(ret);
    ret = NULL;
  }
  if (prefix && prefix[0]) {
    free(fname);
  }
  return ret;
}

/* -- G R A M M A R  P U B L I C  F U N C T I O N S -----------------------*/

grammar_t * grammar_create() {
  grammar_t *ret;
  int        ix;

  ret = NEW(grammar_t);
  ret = (grammar_t *) _ge_initialize((ge_t *) ret,
                                     ret,
                                     NULL,
                                     Grammar);
  ret -> entrypoint = NULL;
  ret -> prefix = NULL;
  ret -> libs = NULL;
  ret -> strategy = ParsingStrategyTopDown;
  ret -> dryrun = FALSE;

  ret -> keywords = intdata_dict_create();
  ret -> nonterminals = strdata_dict_create();

  ret -> lexer_options = array_create((int) LexerOptionLAST);
  for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
    grammar_set_lexer_option(ret, ix, 0L);
  }

  ret -> ge.set_option_delegate = (set_option_t) _grammar_set_option;
  return ret;
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

grammar_t * grammar_set_lexer_option(grammar_t *grammar, lexer_option_t option, long value) {
  array_set(grammar -> lexer_options, (int) option, (void *) value);
  return grammar;
}

long grammar_get_lexer_option(grammar_t *grammar, lexer_option_t option) {
  return (long) array_get(grammar -> lexer_options, (int) option);
}

void grammar_dump(grammar_t *grammar) {
  int   ix;
  char *lib;

  printf("#include <grammar.h>\n"
         "\n"
         "grammar_t * build_grammar() {\n"
         "  grammar_t     *grammar;\n"
         "  nonterminal_t *nonterminal;\n"
         "  rule_t        *rule;\n"
         "  rule_entry_t  *entry;\n"
         "  token_t       *token_name, *token_value;\n"
         "\n"
         "  grammar = grammar_create();\n");
  for (ix = 0; ix != LexerOptionLAST; ix++) {
    printf("  grammar_set_lexer_option(grammar, %s, %ld);\n",
           lexer_option_name(ix), grammar_get_lexer_option(grammar, ix));
  }
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
  _ge_dump((ge_t *) grammar, "grammar", "grammar");
  printf("\n");
  if (grammar -> entrypoint) {
    nonterminal_dump(grammar -> entrypoint);
  }
  dict_visit_values(grammar -> nonterminals, (visit_t) _nonterminal_dump);
  printf("  grammar_analyze(grammar);\n"
         "  return grammar;\n"
         "}\n\n");
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

/* -- N O N T E R M I N A L  F U N C T I O N S --------------------------- */

char * _nonterminal_tostring(nonterminal_t *nonterminal) {
  return nonterminal -> name;
}

void _nonterminal_free(nonterminal_t *nonterminal) {
  if (nonterminal) {
    free(nonterminal -> name);
    array_free(nonterminal -> rules);
    set_free(nonterminal -> firsts);
    set_free(nonterminal -> follows);
    dict_free(nonterminal -> parse_table);
  }
}

/*
    Rules for First Sets

    If X is a terminal then First(X) is just X!
    If there is a Production X → ε then add ε to first(X)
    If there is a Production X → Y1Y2..Yk then add first(Y1Y2..Yk) to first(X)
      First(Y1Y2..Yk) is -
        if First(Y1) doesn't contain ε
          First(Y1)
        else if First(Y1) does contain ε
          First (Y1Y2..Yk) is everything in First(Y1) <except for ε >
          as well as everything in First(Y2..Yk)
        If First(Y1) First(Y2)..First(Yk) all contain ε
          add ε to First(Y1Y2..Yk) as well.
*/

set_t * _nonterminal_get_firsts(nonterminal_t *nonterminal) {
  int     i;
  rule_t *rule;

  if (!nonterminal -> firsts) {
    nonterminal -> firsts = intset_create();
    if (nonterminal -> firsts) {
      for (i = 0; i < array_size(nonterminal -> rules); i++) {
        rule = nonterminal_get_rule(nonterminal, i);
        set_union(nonterminal -> firsts, _rule_get_firsts(rule));
      }
      if (set_empty(nonterminal -> firsts)){
        set_add_int(nonterminal -> firsts, TokenCodeEmpty);
      }
    }
  }
  return nonterminal -> firsts;
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

set_t * _nonterminal_get_follows(nonterminal_t *nonterminal) {
  int             i, j;
  nonterminal_t   r;
  rule_t         *option;
  rule_entry_t   *item, *next;

  if (!nonterminal -> follows) {
    nonterminal -> follows = intset_create();
    if (nonterminal == nonterminal_get_grammar(nonterminal) -> entrypoint) {
      set_add_int(nonterminal -> follows, TokenCodeEnd);
    }
  }
  return nonterminal -> follows;
}

int _nonterminal_check_LL1(nonterminal_t *nonterminal) {
  int     i, j, ret, ok;
  rule_t *r_i, *r_j;
  set_t  *f_i, *f_j, *follows;
  str_t  *s;

  ret = 1;
  for (i = 0; i < array_size(nonterminal -> rules); i++) {
    r_i = nonterminal_get_rule(nonterminal, i);
    f_i = _rule_get_firsts(r_i);
    for (j = i + 1; j < array_size(nonterminal -> rules); j++) {
      r_j = nonterminal_get_rule(nonterminal, j);
      f_j = _rule_get_firsts(r_j);
      ok = set_disjoint(f_i, f_j);
      if (!ok) {
        error("Grammar not LL(1): non-terminal %s - Firsts for rules %d and %d not disjoint", nonterminal -> name, i, j);
        error("FIRSTS(%d): %s", i, set_tostring(f_i));
        error("FIRSTS(%d): %s", j, set_tostring(f_j));
      }
      ret &= ok;
      if (set_has_int(f_j, TokenCodeEnd)) {
        ok = set_disjoint(f_i, follows);
        if (!ok) {
          error("Grammar not LL(1): non-terminal %s - Firsts for rule %d follows not disjoint", nonterminal -> name, i);
        }
        ret &= ok;
        ret = ret && set_disjoint(f_i, nonterminal -> follows);
      }
    }
  }
  //if (grammar_debug) {
  //  debug("Non-terminal %s checks %sOK for LL(1) conditions", nonterminal -> name, (ret) ? "": "NOT ");
  //}
  return ret;
}

void _nonterminal_build_parse_table(nonterminal_t *nonterminal) {
  nonterminal -> parse_table = intdict_create();
  if (nonterminal -> parse_table) {
    array_visit(nonterminal -> rules, (visit_t) _rule_build_parse_table);
  }
}

grammar_t * _nonterminal_dump_terminal(unsigned int code, grammar_t *grammar) {
  token_t *token;

  if (code < 200) {
    fprintf(stderr, " %s", token_code_name(code));
  } else {
    token = (token_t *) dict_get_int(grammar -> keywords, code);
    if (token) {
      fprintf(stderr, " \"%s\"", token_token(token));
    } else {
      fprintf(stderr, " [?%u]", code);
    }
  }
  return grammar;
}

void _nonterminal_dump(ge_t *ge_nt) {
  grammar_t *grammar = ge_nt -> grammar;
  nonterminal_t *nt = (nonterminal_t *) ge_nt;

  if (!grammar -> entrypoint ||
      strcmp(nt -> name, grammar -> entrypoint -> name)) {
    nonterminal_dump((nonterminal_t *) ge_nt);
  }
}

/*
 * nonterminal_t public functions
 */

nonterminal_t * nonterminal_create(grammar_t *grammar, char *name) {
  nonterminal_t *ret;

  ret = NEW(nonterminal_t);
  ret = (nonterminal_t *) _ge_initialize((ge_t *) ret,
                                         grammar,
                                         (ge_t *) grammar,
                                         NonTerminal);
  ret -> firsts = NULL;
  ret -> follows = NULL;
  ret -> parse_table = NULL;
  ret -> name = strdup(name);
  ret -> rules = data_array_create(2);
  ret -> state = strhash(name);
  dict_put(grammar -> nonterminals, strdup(ret -> name), ret);
  if (!grammar -> entrypoint) {
    grammar -> entrypoint = ret;
  }
  return ret;
}

void nonterminal_dump(nonterminal_t *nonterminal) {
  int             i, j;
  rule_t         *rule;
  list_t         *parse_table;
  listiterator_t *iter;
  entry_t        *entry;

  printf("  nonterminal = nonterminal_create(grammar, \"%s\");\n", nonterminal -> name);
  _ge_dump((ge_t *) nonterminal, "nonterminal", "nonterminal");
  array_visit(nonterminal -> rules, (visit_t) rule_dump);
  printf("\n");
}

rule_t * nonterminal_get_rule(nonterminal_t *nonterminal, int ix) {
  assert(ix >= 0);
  assert(ix < array_size(nonterminal -> rules));
  return (rule_t *) array_get(nonterminal -> rules, ix);
}

/*
 * rule_t static functions
 */

void _rule_free(rule_t *rule) {
  if (rule) {
    array_free(rule -> entries);
    set_free(rule -> firsts);
    set_free(rule -> follows);
  }
}

char * _rule_tostring(rule_t *rule) {
  return array_tostring(rule -> entries);
}

/*
    Rules for First Sets

    If X is a terminal then First(X) is just X!
    If there is a Production X → ε then add ε to first(X)
    If there is a Production X → Y1Y2..Yk then add first(Y1Y2..Yk) to first(X)
      First(Y1Y2..Yk) is -
        if First(Y1) doesn't contain ε
          First(Y1)
        else if First(Y1) does contain ε
          First (Y1Y2..Yk) is everything in First(Y1) <except for ε >
          as well as everything in First(Y2..Yk)
        If First(Y1) First(Y2)..First(Yk) all contain ε
          add ε to First(Y1Y2..Yk) as well.
*/
set_t * _rule_get_firsts(rule_t *rule) {
  rule_entry_t *e;
  int           j;

  if (!rule -> firsts) {
    rule -> firsts = intset_create();
    if (rule -> firsts) {
      set_add_int(rule -> firsts, TokenCodeEmpty);
      for (j = 0; set_has_int(rule -> firsts, TokenCodeEmpty) && (j < array_size(rule -> entries)); j++) {
        set_remove_int(rule -> firsts, TokenCodeEmpty);
        e = rule_get_entry(rule, j);
        _rule_entry_get_firsts(e, rule -> firsts);
      }
    }
  }
  return rule -> firsts;
}

set_t * _rule_get_follows(rule_t *rule) {
  return rule -> follows;
}

rule_t * _rule_add_parse_table_entry(long tokencode, rule_t *rule) {
  nonterminal_t *nonterminal;

  nonterminal = rule_get_nonterminal(rule);
  if (tokencode != TokenCodeEmpty) {
    if (!dict_has_int(nonterminal -> parse_table, (int) tokencode)) {
      dict_put_int(nonterminal -> parse_table, (int) tokencode, rule);
    }
  } else {
    set_reduce(nonterminal -> follows,
               (reduce_t) _rule_add_parse_table_entry, rule);
  }
  return rule;
}

void _rule_build_parse_table(rule_t *rule) {
  if (rule -> firsts) {
    set_reduce(rule -> firsts, (reduce_t) _rule_add_parse_table_entry, rule);
  }
}

/*
 * rule_t public functions
 */

rule_t * rule_create(nonterminal_t *nonterminal) {
  rule_t  *ret = NEW(rule_t);

  ret = (rule_t *) _ge_initialize((ge_t *) ret,
                                  nonterminal_get_grammar(nonterminal),
                                  (ge_t *) nonterminal,
                                  Rule);
  ret -> firsts = NULL;
  ret -> follows = NULL;
  ret -> entries = data_array_create(3);
  array_push(nonterminal -> rules, ret);
  return ret;
}

void rule_dump(rule_t *rule) {
  printf("  rule = rule_create(nonterminal);\n");
  _ge_dump((ge_t *) rule, "rule", "rule");
  array_visit(rule -> entries, (visit_t) rule_entry_dump);
}

rule_entry_t * rule_get_entry(rule_t *rule, int ix) {
  assert(ix >= 0);
  assert(ix < array_size(rule -> entries));
  return (rule_entry_t *) array_get(rule -> entries, ix);
}


/* -- R U L E _ E N T R Y ------------------------------------------------ */

rule_entry_t * _rule_entry_create(rule_t *rule, int terminal, void *ptr) {
  rule_entry_t *ret;

  ret = NEW(rule_entry_t);
  ret = (rule_entry_t *) _ge_initialize((ge_t *) ret,
                                        rule_get_grammar(rule),
                                        (ge_t *) rule,
                                        RuleEntry);
  ret -> terminal = terminal;
  if (terminal) {
    ret -> token = (ptr) ? token_copy((token_t *) ptr)
                         : token_create(TokenCodeEmpty, "E");
  } else {
    ret -> nonterminal = strdup((char *) ptr);
  }
  array_push(rule -> entries, ret);
  return ret;
}

char * _rule_entry_tostring(rule_entry_t *entry) {
  if (!entry -> ge._d.str) {
    if (entry -> terminal) {
      asprintf(&entry -> ge._d.str, "'%s'", token_token(entry -> token));
    } else {
      entry -> ge._d.str = strdup(entry -> nonterminal);
    }
  }
  return NULL;
}

void _rule_entry_free(rule_entry_t *entry) {
  if (entry) {
    if (entry -> terminal) {
      token_free(entry -> token);
    } else {
      free(entry -> nonterminal);
    }
  }
}

set_t * _rule_entry_get_firsts(rule_entry_t *entry, set_t *firsts) {
  nonterminal_t *nonterminal;

  if (entry -> terminal) {
    set_add_int(firsts, token_code(entry -> token));
  } else {
    nonterminal = grammar_get_nonterminal(rule_entry_get_grammar(entry),
                                          entry -> nonterminal);
    assert(nonterminal);
    set_union(firsts, _nonterminal_get_firsts(nonterminal));
  }
  return firsts;
}

set_t * _rule_entry_get_follows(rule_entry_t *entry, set_t *follows) {
  return follows;
}

/* ----------------------------------------------------------------------- */

rule_entry_t * rule_entry_non_terminal(rule_t *rule, char *nonterminal) {
  return _rule_entry_create(rule, 0, nonterminal);
}

rule_entry_t * rule_entry_terminal(rule_t *rule, token_t *token) {
  rule_entry_t *ret;
  unsigned int  code;
  char         *str;
  token_t      *t;

  code = token_code(token);
  str = token_token(token);
  if ((code == TokenCodeDQuotedStr) && strcmp(str, "\"")) {
    code = (int) strhash(str);
    token = token_create(code, str);
    dict_put_int(rule_get_grammar(rule) -> keywords, code, token);
  } else if (code > 200) {
    dict_put_int(rule_get_grammar(rule) -> keywords, code, token);
  }
  return _rule_entry_create(rule, TRUE, token);
}

rule_entry_t * rule_entry_empty(rule_t *rule) {
  return _rule_entry_create(rule, 1, NULL);
}

void rule_entry_dump(rule_entry_t *entry) {
  int           code;
  token_t      *kw;

  if (entry -> terminal) {
    printf("  entry = rule_entry_terminal(rule, token_create(%d, \"%s\"));\n",
           token_code(entry -> token),
           (token_code(entry -> token) != 34) ? token_token(entry -> token) : "\\\"");
  } else {
    printf("  entry = rule_entry_non_terminal(rule, \"%s\");\n", entry -> nonterminal);
  }
  _ge_dump((ge_t *) entry, "rule_entry", "entry");
}
