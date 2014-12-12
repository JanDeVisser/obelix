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
#include <stdio.h>
#include <string.h>

#include <grammar.h>
#include <list.h>

int grammar_debug = 0;

static ge_t *              _ge_create(grammar_t *, ge_t *, grammar_element_type_t, ...);

static grammar_t *         _grammar_create(ge_t *, va_list);
static void                _grammar_free(grammar_t *);
static void                _grammar_get_firsts_visitor(entry_t *);
static int *               _grammar_check_LL1_reducer(entry_t *, int *);
static void                _grammar_build_parse_table_visitor(entry_t *);
static ge_t *              _grammar_set_option(ge_t *, char *, token_t *);

static nonterminal_t *     _nonterminal_create(ge_t *, va_list);
static void                _nonterminal_free(nonterminal_t *);
static set_t *             _nonterminal_get_firsts(nonterminal_t *);
static set_t *             _nonterminal_get_follows(nonterminal_t *);
static int                 _nonterminal_check_LL1(nonterminal_t *);
static void                _nonterminal_build_parse_table(nonterminal_t *);
static grammar_t *         _nonterminal_dump_terminal(long, grammar_t *);

static rule_t *            _rule_create(ge_t *, va_list);
static void                _rule_free(rule_t *);
static set_t *             _rule_get_firsts(rule_t *);
static set_t *             _rule_get_follows(rule_t *);
static void                _rule_build_parse_table(rule_t *);
static rule_t *            _rule_add_parse_table_entry(long, rule_t *);

static rule_entry_t *      _rule_entry_new(ge_t *, va_list);
static rule_entry_t *      _rule_entry_create(rule_t *, int, void *);
static void                _rule_entry_free(rule_entry_t *);
static set_t *             _rule_entry_get_firsts(rule_entry_t *, set_t *);
static set_t *             _rule_entry_get_follows(rule_entry_t *, set_t *);

static typedescr_t elements[] = {
  {
    type:                  GETGrammar,
    typecode:              "G",
    new:      (new_t)      _grammar_create,
    copy:                  NULL,
    cmp:      (cmp_t)      NULL,
    free:     (free_t)     _grammar_free,
    tostring: (tostring_t) NULL,
    parse:                 NULL,
    hash:                  NULL
  },
  {
    type:                  GETNonTerminal,
    typecode:              "N",
    new:      (new_t)      _nonterminal_create,
    copy:                  NULL,
    cmp:      (cmp_t)      NULL,
    free:     (free_t)     _nonterminal_free,
    tostring: (tostring_t) NULL,
    parse:                 NULL,
    hash:                  NULL
  },
  {
    type:                  GETRule,
    typecode:              "N",
    new:      (new_t)      _rule_create,
    copy:                  NULL,
    cmp:      (cmp_t)      NULL,
    free:     (free_t)     _rule_free,
    tostring: (tostring_t) NULL,
    parse:                 NULL,
    hash:                  NULL
  },
  {
    type:                  GETRuleEntry,
    typecode:              "N",
    new:      (new_t)      _rule_entry_new,
    copy:                  NULL,
    cmp:      (cmp_t)      NULL,
    free:     (free_t)     _rule_entry_free,
    tostring: (tostring_t) NULL,
    parse:                 NULL,
    hash:                  NULL
  },
};


/*
 * ge_t - static functions
 */

ge_t * _ge_create(grammar_t *grammar, ge_t *owner, grammar_element_type_t type, ...) {
  ge_t    *ret;
  va_list  args;

  ret = NEW(ge_t);
  ret -> type = type;
  ret -> grammar = grammar;
  ret -> owner = owner;
  ret -> initializer = NULL;
  ret -> finalizer = NULL;
  ret -> pushvalues = set_create((cmp_t) token_cmp);
  ret -> incrs = strset_create();
  ret -> variables = strtoken_dict_create();
  va_start(args, type);
  if (elements[type].new) {
    ret -> ptr = elements[type].new(ret, args);
  }
  va_end(args);
  return ret;
}

/*
 * ge_t - public functions
 */

void ge_free(ge_t *ge) {
  if (ge) {
    dict_free(ge -> variables);
    token_free(ge -> pushvalue);
    function_free(ge -> initializer);
    function_free(ge -> finalizer);
    if (elements[ge -> type].free) {
      elements[ge -> type].free(ge -> ptr);
    }
    free(ge);
  }
}

ge_t * ge_add_pushvalue(ge_t *ge, token_t *token) {
  set_add(ge -> pushvalues, token_copy(token))
  return ge;
}

ge_t * ge_add_incr(ge_t *ge, token_t *token) {
  set_add(ge -> incrs, strdup(token_token(token)));
  return ge;
}

ge_t * ge_set_initializer(ge_t *ge, function_t *fnc) {
  ge -> initializer = fnc;
  return ge;
}

function_t * ge_get_initializer(ge_t *ge) {
  return ge -> initializer;
}

ge_t * ge_set_finalizer(ge_t *ge, function_t *fnc) {
  ge -> finalizer = fnc;
  return ge;
}

function_t * ge_get_finalizer(ge_t *ge) {
  return ge -> finalizer;
}

ge_t * ge_set_option(ge_t *ge, char *name, token_t *val) {
  if (!strcmp(name, "init")) {
    ge_set_initializer(ge, grammar_resolve_function(ge -> grammar, token_token(val)));
  } else if (!strcmp(name, "done")) {
    ge_set_finalizer(ge, grammar_resolve_function(ge -> grammar, token_token(val)));
  } else if (!strcmp(name, "push")) {
    ge_add_pushvalue(ge, val);
  } else if (!strcmp(name, "incr")) {
    ge_add_incr(ge, val);
  } else if (ge -> set_option_delegate && ge -> set_option_delegate(ge, name, val)) {
    //
  } else {
    dict_put(ge -> variables, strdup(name), token_copy(val));
  }
  return ge;
}

/*
 * grammar_t static functions
 */

void _grammar_get_firsts_visitor(entry_t *entry) {
  nonterminal_t *nonterminal;

  nonterminal = (nonterminal_t *) entry -> value;
  if (grammar_debug) {
    debug("Building FIRST sets for rule %s", nonterminal -> name);
  }
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

  nonterminal = (nonterminal_t *) entry -> value;

  if (grammar_debug) {
    debug("Building FOLLOW sets for rule %s", nonterminal -> name);
  }
  follows = _nonterminal_get_follows(nonterminal);
  for (i = 0; i < array_size(nonterminal -> rules); i++) {
    rule = (rule_t *) array_get(nonterminal -> rules, i);
    for (j = 0; j < array_size(rule -> entries); j++) {
      rule_entry = (rule_entry_t *) array_get(rule -> entries, j);
      if (!rule_entry -> terminal) {
        next_firsts = intset_create();
        next = NULL;
        for (k = j + 1; k < array_size(rule -> entries); k++) {
          it = (rule_entry_t *) array_get(rule -> entries, k);
          if (!next) {
            next = it;
          }
          _rule_entry_get_firsts(it, next_firsts);
          if (!set_has_int(next_firsts, TokenCodeEmpty)) {
            break;
          }
        }
        nt = grammar_get_nonterminal(
               nonterminal_get_grammar(nt),
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
  *ok &= _nonterminal_check_LL1((nonterminal_t *) entry -> value);
  return ok;
}

void _grammar_build_parse_table_visitor(entry_t *entry) {
  _nonterminal_build_parse_table(entry -> value);
}

grammar_t * _grammar_create(ge_t *ge, va_list args) {
  grammar_t *ret;
  int        ix;

  ret = NEW(grammar_t);
  ret -> ge = ge;
  ret -> entrypoint = NULL;
  ret -> prefix = NULL;
  ret -> strategy = ParsingStrategyTopDown;

  ret -> keywords = intdict_create();
  dict_set_free_data(ret -> keywords, (free_t) token_free);

  ret -> nonterminals = strvoid_dict_create();
  dict_set_free_data(ret -> nonterminals, (free_t) nonterminal_free);

  ret -> lexer_options = array_create((int) LexerOptionLAST);
  for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
    grammar_set_lexer_option(ret, ix, 0L);
  }

  ge -> grammar = ret;
  ge -> owner = NULL;
  ge -> set_option_delegate = (set_option_t) _grammar_set_option;
  return ret;
}

void _grammar_free(grammar_t *grammar) {
  if (grammar) {
    dict_free(grammar -> nonterminals);
    dict_free(grammar -> keywords);
    array_free(grammar -> lexer_options);
    if (grammar -> prefix) {
      free(grammar -> prefix);
    }
    free(grammar);
  }
}

ge_t * _grammar_set_option(ge_t *ge, char *name, token_t *val) {
  int        b;
  grammar_t *g = ge -> grammar;
  char      *str;

  if (!strcmp(name, "lib")) {
    resolve_library(token_token(val));
  } else if (!strcmp(name, "prefix")) {
    g -> prefix = strdup(token_token(val));
  } else if (!strcmp(name, "strategy")) {
    str = token_token(val);
    if (!strncmp(str, "topdown", 7)|| !strncmp(str, "ll(1)", 5)) {
      grammar_set_parsing_strategy(g, ParsingStrategyTopDown);
    } else if (!strncmp(str, "bottomup", 8) || !strncmp(str, "lr(1)", 5)) {
      grammar_set_parsing_strategy(g, ParsingStrategyBottomUp);
    }
  } else if (!strcmp(name, "ignore_ws")) {
    grammar_set_lexer_option(g, LexerOptionIgnoreWhitespace,
                       atob(token_token(val)));
  } else if (!strcmp(name, "ignore_nl")) {
    grammar_set_lexer_option(g, LexerOptionIgnoreNewLines,
                       atob(token_token(val)));
  } else if (!strcmp(name, "case_sensitive")) {
    grammar_set_lexer_option(g, LexerOptionCaseSensitive,
                       atob(token_token(val)));
  } else if (!strcmp(name, "hashpling")) {
    grammar_set_lexer_option(g, LexerOptionHashPling,
                       atob(token_token(val)));
  } else {
    ge = NULL;
  }
  return ge;
}


/*
 * grammar_t public functions
 */

grammar_t * grammar_create() {
  ge_t *ge;

  ge = _ge_create(NULL, NULL, GETGrammar);
  return (grammar_t *) ge -> ptr;
}

void grammar_free(grammar_t *grammar) {
  ge_free(grammar -> ge);
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
  if (dict_size(grammar -> keywords)) {
    fprintf(stderr, "< ");
    dict_visit_values(grammar -> keywords, (visit_t) token_dump);
    fprintf(stderr, " >\n");
  }
  dict_visit_values(grammar -> nonterminals, (visit_t) rule_dump);
}

function_t * grammar_resolve_function(grammar_t *grammar, char *func_name) {
  char          *fname;
  int            len;
  voidptr_t      fnc;
  function_t *ret;

  if (grammar -> prefix) {
    len = strlen(grammar -> prefix) + strlen(func_name);
    fname = (char *) new(len + 1);
    strcpy(fname, grammar -> prefix);
    strcat(fname, func_name);
  } else {
    fname = func_name;
  }
  fnc = (voidptr_t) resolve_function(fname);
  assert(fnc);
  ret = function_create(fname, fnc);
  if (grammar -> prefix) {
    free(fname);
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

/*
 * nonterminal_t static functions
 */

nonterminal_t * _nonterminal_create(ge_t *ge, va_list args) {
  nonterminal_t *ret;
  char          *name;

  name = va_arg(args, char *);
  ret = NEW(nonterminal_t);
  ret -> firsts = NULL;
  ret -> follows = NULL;
  ret -> parse_table = NULL;
  ret -> name = strdup(name);
  ret -> rules = array_create(2);
  array_set_free(ret -> rules, (free_t) ge_free);
  ret -> state = strhash(name);
  dict_put(ge -> grammar -> nonterminals, strdup(ret -> name), ret);
  if (!ge -> grammar -> entrypoint) {
    ge -> grammar -> entrypoint = ret;
  }
  return ret;
}

void _nonterminal_free(nonterminal_t *nonterminal) {
  if (nonterminal) {
    free(nonterminal -> name);
    array_free(nonterminal -> rules);
    set_free(nonterminal -> firsts);
    set_free(nonterminal -> follows);
    dict_free(nonterminal -> parse_table);
    free(nonterminal);
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
  int            i;
  rule_t *option;

  if (!nonterminal -> firsts) {
    nonterminal -> firsts = intset_create();
    if (nonterminal -> firsts) {
      for (i = 0; i < array_size(nonterminal -> rules); i++) {
        option = array_get(nonterminal -> rules, i);
        set_union(nonterminal -> firsts, _rule_get_firsts(option));
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
  int            i;
  int            j;
  nonterminal_t  r;
  rule_t *option;
  rule_entry_t   *item;
  rule_entry_t   *next;

  if (!nonterminal -> follows) {
    nonterminal -> follows = intset_create();
    if (nonterminal == nonterminal -> ge -> grammar -> entrypoint) {
      set_add_int(nonterminal -> follows, TokenCodeEnd);
    }
  }
  return nonterminal -> follows;
}

int _nonterminal_check_LL1(nonterminal_t *nonterminal) {
  int     i, j, ret, ok;
  rule_t *r_i, *r_j;
  set_t  *f_i, *f_j, *follows;

  ret = 1;
  if (grammar_debug) {
    debug("Checking LL(1) conditions for rule %s [%d rules]",
          nonterminal -> name, array_size(nonterminal -> rules));
  }
  for (i = 0; i < array_size(nonterminal -> rules); i++) {
    r_i = array_get(nonterminal -> rules, i);
    f_i = _rule_get_firsts(r_i);
    for (j = i + 1; j < array_size(nonterminal -> rules); j++) {
      r_j = array_get(nonterminal -> rules, j);
      f_j = _rule_get_firsts(r_j);
      ok = set_disjoint(f_i, f_j);
      if (!ok) {
        error("Grammar not LL(1): non-terminal %s - Firsts for rules %d and %d not disjoint", nonterminal -> name, i, j);
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
  if (grammar_debug) {
    debug("Non-terminal %s checks %sOK for LL(1) conditions", nonterminal -> name, (ret) ? "": "NOT ");
  }
  return ret;
}

void _nonterminal_build_parse_table(nonterminal_t *nonterminal) {
  if (grammar_debug) {
    debug("Building parse table for non-terminal %s", nonterminal -> name);
  }
  nonterminal -> parse_table = intdict_create();
  if (nonterminal -> parse_table) {
    array_visit(nonterminal -> rules, (visit_t) _rule_build_parse_table);
  }
}

grammar_t * _nonterminal_dump_terminal(long code, grammar_t *grammar) {
  token_t *token;

  if (code < 200) {
    fprintf(stderr, " %s", token_code_name(code));
  } else {
    token = (token_t *) dict_get_int(grammar -> keywords, code);
    if (token) {
      fprintf(stderr, " \"%s\"", token_token(token));
    } else {
      fprintf(stderr, " [?%ld]", code);
    }
  }
  return grammar;
}

/*
 * nonterminal_t public functions
 */

nonterminal_t * nonterminal_create(grammar_t *grammar, char *name) {
  ge_t *ge;

  ge = _ge_create(grammar, grammar -> ge, GETNonTerminal, name);
  return (nonterminal_t *) ge -> ptr;
}

void nonterminal_free(nonterminal_t *nonterminal) {
  ge_free(nonterminal -> ge);
}

void nonterminal_dump(nonterminal_t *nonterminal) {
  int             i, j;
  rule_t  *option;
  list_t         *parse_table;
  listiterator_t *iter;
  entry_t        *entry;

  fprintf(stderr, "%s%s :=", (nonterminal -> ge -> grammar -> entrypoint == nonterminal) ? "(*) " : "", nonterminal -> name);
  array_visit(nonterminal -> rules, (visit_t) rule_dump);
  fprintf(stderr, "\b;\n  Firsts:\n");
  for (i = 0; i < array_size(nonterminal -> rules); i++) {
    option = (rule_t *) array_get(nonterminal -> rules, i);
    fprintf(stderr, "  [ ");
    set_reduce(option -> firsts, (reduce_t) _nonterminal_dump_terminal, nonterminal -> ge -> grammar);
    fprintf(stderr, " ]\n");
  }
  fprintf(stderr, "  Follows:\n");
  fprintf(stderr, "  [ ");
  set_reduce(nonterminal -> follows, (reduce_t) _nonterminal_dump_terminal, nonterminal -> ge -> grammar);
  fprintf(stderr, " ]\n  Parse Table:\n  [");
  dict_reduce_keys(nonterminal -> parse_table, (reduce_t) _nonterminal_dump_terminal, nonterminal -> ge -> grammar);
  fprintf(stderr, " ]\n\n");
}

/*
 * rule_t static functions
 */

rule_t * _rule_create(ge_t *ge, va_list args) {
  nonterminal_t *nonterminal;
  rule_t        *ret;

  nonterminal = (nonterminal_t *) (ge -> owner -> ptr);
  ret = NEW(rule_t);
  ret -> firsts = NULL;
  ret -> follows = NULL;
  ret -> entries = array_create(3);
  array_set_free(ret -> entries, (free_t) rule_entry_free);
  array_push(nonterminal -> rules, ret);
  ret -> ge = ge;
  return ret;
}

void _rule_free(rule_t *rule) {
  if (rule) {
    array_free(rule -> entries);
    set_free(rule -> firsts);
    set_free(rule -> follows);
    free(rule);
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
set_t * _rule_get_firsts(rule_t *rule) {
  rule_entry_t *e;
  int           j;

  if (!rule -> firsts) {
    rule -> firsts = intset_create();
    if (rule -> firsts) {
      set_add_int(rule -> firsts, TokenCodeEmpty);
      for (j = 0; set_has_int(rule -> firsts, TokenCodeEmpty) && (j < array_size(rule -> entries)); j++) {
        set_remove_int(rule -> firsts, TokenCodeEmpty);
        e = (rule_entry_t *) array_get(rule -> entries, j);
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
  ge_t *ge;

  ge = _ge_create(nonterminal_get_grammar(nonterminal), nonterminal -> ge, GETRule);
  return (rule_t *) ge -> ptr;
}

void rule_free(rule_t *rule) {
  ge_free(rule -> ge);
}

void rule_dump(rule_t *rule) {
  array_visit(rule -> entries, (visit_t) rule_entry_dump);
  fprintf(stderr, " |");
}

/*
 * rule_entry_t static functions
 */

rule_entry_t * _rule_entry_new(ge_t *ge, va_list args) {
  rule_t        *rule;
  rule_entry_t  *ret;
  int            terminal;
  void          *ptr;

  terminal = va_arg(args, int);
  ptr = va_arg(args, void *);
  rule = (rule_t *) (ge -> owner -> ptr);
  ret = NEW(rule_entry_t);
  ret -> terminal = terminal;
  if (terminal) {
    ret -> token =
        (ptr) ? token_copy((token_t *) ptr)
              : token_create(TokenCodeEmpty, "E");
  } else {
    ret -> nonterminal = strdup((char *) ptr);
  }
  array_push(rule -> entries, ret);
  ret -> ge = ge;
  return ret;
}

rule_entry_t * _rule_entry_create(rule_t *rule, int terminal, void *ptr) {
  ge_t *ge;

  ge = _ge_create(rule_get_grammar(rule), rule -> ge,
                  GETNonTerminal, terminal, ptr);
  return (rule_entry_t *) ge -> ptr;
}

void _rule_entry_free(rule_entry_t *entry) {
  if (entry) {
    if (entry -> terminal) {
      token_free(entry -> token);
    } else {
      free(entry -> nonterminal);
    }
    free(entry);
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

/*
 * rule_entry_t public functions
 */

void rule_entry_free(rule_entry_t *entry) {
  ge_free(entry -> ge);
}

rule_entry_t * rule_entry_non_terminal(rule_t *rule, char *nonterminal) {
  return _rule_entry_create(rule, 0, nonterminal);
}

rule_entry_t * rule_entry_terminal(rule_t *rule, token_t *token) {
  rule_entry_t *ret;
  int          code;
  char        *str;
  token_t     *t;

  code = token_code(token);
  str = token_token(token);
  if ((code == TokenCodeDQuotedStr) && strcmp(str, "\"")) {
    code = (int) strhash(str);
    token = token_create(code, str);
    dict_put_int(rule_get_grammar(rule) -> keywords, code, token);
  }
  return _rule_entry_create(rule, TRUE, token);
}

rule_entry_t * rule_entry_empty(rule_t *rule) {
  return _rule_entry_create(rule, 1, NULL);
}

void rule_entry_dump(rule_entry_t *entry) {
  int      code;
  token_t *kw;

  fprintf(stderr, " ");
  if (entry -> terminal) {
    code = token_code(entry -> token);
    if (code < 32) {
      fprintf(stderr, " %d", code);
    } else if (code < 200) {
      fprintf(stderr, " '%s'", token_token(entry -> token));
    } else {
      kw = (token_t *) dict_get_int(rule_entry_get_grammar(entry) -> keywords,
                                    code);
      fprintf(stderr, " \"%s\"", token_token(kw));
    }
  } else {
    fprintf(stderr, "%s", entry -> nonterminal);
  }
}

