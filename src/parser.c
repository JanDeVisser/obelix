/*
 * /obelix/src/parser.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
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

#include <array.h>
#include <dict.h>
#include <expr.h>
#include <lexer.h>
#include <list.h>
#include <parser.h>
#include <resolve.h>
#include <set.h>

typedef enum _gp_state_ {
  GPStateStart,
  GPStateOptions,
  GPStateHeader,
  GPStateRule,
  GPStateRuleOption,
  GPStateItem,
} gp_state_t;

typedef struct _gp_state_rec {
  char     *name;
  reduce_t  handler;
} gp_state_rec_t;

typedef struct _grammar_parser {
  grammar_t     *grammar;
  gp_state_t     state;
  gp_state_t     old_state;
  char          *string;
  dict_t        *options;
  rule_t        *rule;
  rule_option_t *option;
  rule_item_t   *item;
} grammar_parser_t;

static grammar_parser_t * _grammar_parser_state_start(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_options(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_header(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_rule(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_rule_option(token_t *, grammar_parser_t *);
static grammar_parser_t * _grammar_parser_state_item(token_t *, grammar_parser_t *);

static gp_state_rec_t _gp_state_recs[] = {
    { "GPStateStart",      (reduce_t) _grammar_parser_state_start },
    { "GPStateOptions",    (reduce_t) _grammar_parser_state_options },
    { "GPStateHeader",     (reduce_t) _grammar_parser_state_header },
    { "GPStateRule",       (reduce_t) _grammar_parser_state_rule },
    { "GPStateRuleOption", (reduce_t) _grammar_parser_state_rule_option },
    { "GPStateItem",       (reduce_t) _grammar_parser_state_item }
};

static void            _grammar_parser_set_option(grammar_parser_t *, char *);

static set_t *         _rule_get_firsts(rule_t *);
static set_t *         _rule_get_follows(rule_t *);
static int             _rule_check_LL1(rule_t *);
static void            _rule_build_parse_table(rule_t *);

static set_t *         _rule_option_get_firsts(rule_option_t *);
static set_t *         _rule_option_get_follows(rule_option_t *);
static void            _rule_option_build_parse_table(rule_option_t *);
static rule_option_t * _rule_option_add_parse_table_entry(long, rule_option_t *);

static rule_item_t *   _rule_item_create(rule_option_t *, int);
static set_t *         _rule_item_get_firsts(rule_item_t *, set_t *);
static set_t *         _rule_item_get_follows(rule_item_t *, set_t *);

static void            _grammar_dump_rule(entry_t *);
static void            _grammar_get_firsts_visitor(entry_t *);
static int *           _grammar_check_LL1_reducer(entry_t *, int *);
static void            _grammar_build_parse_table_visitor(entry_t *);
static grammar_t *     _grammar_analyze(grammar_t *);

static void *          _parser_token_handler(token_t *, parser_t *);


/*
 * grammar_parser_t static functions
 */

/*
 * rule_t static functions
 */

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
  int            i;
  rule_option_t *option;

  if (!rule -> firsts) {
    rule -> firsts = intset_create();
    if (rule -> firsts) {
      for (i = 0; i < array_size(rule -> options); i++) {
        option = array_get(rule -> options, i);
        set_union(rule -> firsts, _rule_option_get_firsts(option));
      }
      if (set_empty(rule -> firsts)){
        set_add_int(rule -> firsts, TokenCodeEmpty);
      }
    }
  }
  return rule -> firsts;
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

set_t * _rule_get_follows(rule_t *rule) {
  int            i;
  int            j;
  rule_t         r;
  rule_option_t *option;
  rule_item_t   *item;
  rule_item_t   *next;

  if (!rule -> follows) {
    rule -> follows = intset_create();
    if (rule == rule -> grammar -> entrypoint) {
      set_add_int(rule -> follows, TokenCodeEnd);
    }
  }
  return rule -> follows;
}

int _rule_check_LL1(rule_t *rule) {
  int            i, j, ret, ok;
  rule_option_t *o_i, *o_j;
  set_t          *f_i, *f_j, *follows;

  ret = 1;
  debug("Checking LL(1) conditions for rule %s", rule -> name);
  for (i = 0; i < array_size(rule -> options); i++) {
    o_i = array_get(rule -> options, i);
    f_i = _rule_option_get_firsts(o_i);
    for (j = j + 1; j < array_size(rule -> options); j++) {
      o_j = array_get(rule -> options, i);
      f_j = _rule_option_get_firsts(o_j);
      ok = set_disjoint(f_i, f_j);
      if (!ok) {
        error("Grammar not LL(1): Rule %s - Firsts for option %d and %d not disjoint", rule -> name, i, j);
      }
      ret &= ok;
      if (set_has_int(f_j, TokenCodeEnd)) {
        ok = set_disjoint(f_i, follows);
        if (!ok) {
          error("Grammar not LL(1): Rule %s - Firsts for option %d follows not disjoint", rule -> name, i);
        }
        ret &= ok;
        ret = ret && set_disjoint(f_i, rule -> follows);
      }
    }
  }
  return ret;
}

void _rule_build_parse_table(rule_t *rule) {
  debug("Building parse table for rule %s", rule -> name);
  rule -> parse_table = intdict_create();
  if (rule -> parse_table) {
    array_visit(rule -> options, (visit_t) _rule_option_build_parse_table);
  }
}

/*
 * rule_t public functions
 */

rule_t * rule_create(grammar_t *grammar, char *name) {
  rule_t *ret;

  ret = NEW(rule_t);
  if (ret) {
    ret -> name = strdup(name);
    if (ret -> name) {
      ret -> options = array_create(2);
      if (!ret -> options) {
        free(ret -> name);
        ret -> name = NULL;
      } else {
        ret -> state = strhash(name);
        array_set_free(ret -> options, (free_t) rule_option_free);
        dict_put(grammar -> rules, ret -> name, ret);
        if (!grammar -> entrypoint) {
          grammar -> entrypoint = ret;
        }
        ret -> grammar = grammar;
      }
    }
    if (!ret -> name) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void rule_free(rule_t *rule) {
  if (rule) {
    free(rule -> name);
    array_free(rule -> options);
    free(rule);
  }
}

void rule_set_options(rule_t *rule, dict_t *options) {
  char *val;
  resolve_t *resolve;
  int b;

  val = dict_get(options, "init");
  if (val) {
    rule_set_initializer(rule, (parser_fnc_t) resolve_function(val));
  }
  val = dict_get(options, "done");
  if (val) {
    rule_set_finalizer(rule, (parser_fnc_t) resolve_function(val));
  }
  dict_clear(options);
}

parser_fnc_t rule_get_initializer(rule_t *rule) {
  return rule -> initializer;
}

rule_t * rule_set_initializer(rule_t *rule, parser_fnc_t fnc) {
  rule -> initializer = fnc;
  return rule;
}

parser_fnc_t rule_get_finalizer(rule_t *rule) {
  return rule -> finalizer;
}

rule_t * rule_set_finalizer(rule_t *rule, parser_fnc_t fnc) {
  rule -> finalizer = fnc;
  return rule;
}

static grammar_t * _rule_dump_terminal(long code, grammar_t *grammar) {
  token_t *token;

  if (code < 200) {
    fprintf(stderr, " %ld,", code);
  } else {
    token = (token_t *) dict_get_int(grammar -> keywords, code);
    fprintf(stderr, " \"%s\",", token_token(token));
  }
  return grammar;
}

void rule_dump(rule_t *rule) {
  int            i, j;
  rule_option_t *option;

  fprintf(stderr, "%s%s :=", (rule -> grammar -> entrypoint == rule) ? "(*) " : "", rule -> name);
  array_visit(rule -> options, (visit_t) rule_option_dump);
  fprintf(stderr, "\b;\nFirsts:\n");
  for (i = 0; i < array_size(rule -> options); i++) {
    option = (rule_option_t *) array_get(rule -> options, i);
    fprintf(stderr, "  [ ");
    set_reduce(option -> firsts, (reduce_t) _rule_dump_terminal, rule -> grammar);
    fprintf(stderr, " ]\n");
  }
  fprintf(stderr, "Follows:\n");
  fprintf(stderr, "  [ ");
  set_reduce(rule -> follows, (reduce_t) _rule_dump_terminal, rule -> grammar);
  fprintf(stderr, " ]\n");
}

/*
 * rule_option_t static functions
 */

set_t * _rule_option_get_firsts(rule_option_t *option) {
  rule_item_t *item;
  int j;

  if (!option -> firsts) {
    option -> firsts = intset_create();
    if (option -> firsts) {
      set_add_int(option -> firsts, TokenCodeEmpty);
      for (j = 0; set_has_int(option -> firsts, TokenCodeEmpty) && (j < array_size(option -> items)); j++) {
        set_remove_int(option -> firsts, TokenCodeEmpty);
        item = (rule_item_t *) array_get(option -> items, j);
        _rule_item_get_firsts(item, option -> firsts);
      }
    }
  }
  return option -> firsts;
}

set_t * _rule_option_get_follows(rule_option_t *option) {
  return option -> follows;
}

rule_option_t * _rule_option_add_parse_table_entry(long tokencode,
                                                   rule_option_t *option) {
  debug("  yy %ld", tokencode);
  if (tokencode != TokenCodeEmpty) {
    dict_put_int(option -> rule -> parse_table, (int) tokencode, option);
  } else {
    set_reduce(option -> rule -> follows,
               (reduce_t) _rule_option_add_parse_table_entry, option);
  }
  return option;
}

void _rule_option_build_parse_table(rule_option_t *option) {
  debug("  xx");
  if (option -> firsts) {
    set_reduce(option -> firsts, (reduce_t) _rule_option_add_parse_table_entry, option);
  }
}

/*
 * rule_option_t public functions
 */

rule_option_t * rule_option_create(rule_t *rule) {
  rule_option_t *ret;

  ret = NEW(rule_option_t);
  if (ret) {
    ret -> items = array_create(3);
    if (ret -> items) {
      ret -> rule = rule;
      array_set_free(ret -> items, (free_t) rule_item_free);
      if (!array_push(rule -> options, ret)) {
        array_free(ret -> items);
        ret -> items = NULL;
      }
    }
    if (!ret -> items) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void rule_option_free(rule_option_t *option) {
  if (option) {
    array_free(option -> items);
    free(option);
  }
}

void rule_option_dump(rule_option_t *option) {
  array_visit(option -> items, (visit_t) rule_item_dump);
  fprintf(stderr, " |");
}

/*
 * rule_item_t static functions
 */

rule_item_t * _rule_item_create(rule_option_t *option, int terminal) {
  rule_item_t *ret;

  ret = NEW(rule_item_t);
  if (ret) {
    ret -> terminal = terminal;
    ret -> option = option;
    if (!array_push(option -> items, ret)) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

set_t * _rule_item_get_firsts(rule_item_t *item, set_t *firsts) {
  rule_t *rule;

  if (item -> terminal) {
    set_add_int(firsts, token_code(item -> token));
  } else {
    rule = grammar_get_rule(item -> option -> rule -> grammar, item -> rule);
    assert(rule);
    set_union(firsts, _rule_get_firsts(rule));
  }
  return firsts;
}

set_t * _rule_item_get_follows(rule_item_t *item, set_t *follows) {
  return follows;
}

/*
 * rule_item_t public functions
 */

void rule_item_free(rule_item_t *item) {
  if (item) {
    if (item -> terminal) {
      token_free(item -> token);
    } else {
      free(item -> rule);
    }
    free(item);
  }
}

rule_item_t * rule_item_non_terminal(rule_option_t *option, char *rule) {
  rule_item_t *ret;

  ret = _rule_item_create(option, FALSE);
  if (ret) {
    ret -> rule = strdup(rule);
  }
  return ret;
}

rule_item_t * rule_item_terminal(rule_option_t *option, token_t *token) {
  rule_item_t *ret;
  int          code;
  char        *str;
  token_t     *terminal_token;

  ret = _rule_item_create(option, TRUE);
  if (ret) {
    code = token_code(token);
    str = token_token(token);
    if (code == TokenCodeDQuotedStr) {
      code = (int) strhash(str);
      ret -> token = token_create(code, str);
      dict_put_int(option -> rule -> grammar -> keywords, code, ret -> token);
    } else {
      ret -> token = token_copy(token);
    }
  }
  return ret;
}

rule_item_t * rule_item_empty(rule_option_t *option) {
  rule_item_t *ret;

  ret = _rule_item_create(option, TRUE);
  if (ret) {
    ret -> token = token_create(TokenCodeEmpty, "ε");
  }
  return ret;
}

parser_fnc_t rule_item_get_initializer(rule_item_t *item) {
  return item -> initializer;
}

rule_item_t * rule_item_set_initializer(rule_item_t *item, parser_fnc_t fnc) {
  item -> initializer = fnc;
  return item;
}

parser_fnc_t rule_item_get_finalizer(rule_item_t *item) {
  return item -> finalizer;
}

rule_item_t * rule_item_set_finalizer(rule_item_t *item, parser_fnc_t fnc) {
  item -> finalizer = fnc;
  return item;
}

void rule_item_set_options(rule_item_t *item, dict_t *options) {
  char *val;
  resolve_t *resolve;
  int b;

  val = dict_get(options, "init");
  if (val) {
    rule_item_set_initializer(item, (parser_fnc_t) resolve_function(val));
  }
  val = dict_get(options, "done");
  if (val) {
    rule_item_set_finalizer(item, (parser_fnc_t) resolve_function(val));
  }
  dict_clear(options);
}

void rule_item_dump(rule_item_t *item) {
  fprintf(stderr, " ");
  if (item -> terminal) {
    fprintf(stderr, "'%s' (%d)", token_token(item -> token), token_code(item -> token));
  } else {
    fprintf(stderr, "%s", item -> rule);
  }
}

/*
 * grammar_t static functions
 */

grammar_parser_t * _grammar_parser_state_start(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  char      *str, *state_str;
  char       terminal_str[2];
  grammar_t *g;
  gp_state_t state;

  g = grammar_parser -> grammar;
  state = grammar_parser -> state;
  code = token_code(token);
  str = token_token(token);
  switch (code) {
    case TokenCodeIdentifier:
      grammar_parser -> state = GPStateRule;
      grammar_parser -> rule = rule_create(grammar_parser -> grammar, str);
      grammar_parser -> option = NULL;
      grammar_parser -> item = NULL;
      break;
    case TokenCodeOpenBracket:
      grammar_parser -> old_state = GPStateRule;
      grammar_parser -> state = GPStateOptions;
      break;
    case TokenCodePercent:
      grammar_parser -> old_state = GPStateStart;
      grammar_parser -> state = GPStateOptions;
      break;
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_options(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  char      *str, *state_str;
  char       terminal_str[2];
  grammar_t *g;
  gp_state_t state;

  g = grammar_parser -> grammar;
  state = grammar_parser -> state;
  code = token_code(token);
  str = token_token(token);

  switch (code) {
    case TokenCodeIdentifier:
      if (!grammar_parser -> string) {
        grammar_parser -> string = strdup(str);
      } else {
        dict_put(grammar_parser -> options, grammar_parser -> string, strdup(str));
        grammar_parser -> string = NULL;
      }
      break;
    case TokenCodeColon:
      break;
    case TokenCodePercent:
      if (grammar_parser -> old_state == GPStateStart) {
        grammar_set_options(g, grammar_parser -> options);
        grammar_parser -> state = grammar_parser -> old_state;
      }
      break;
    case TokenCodeCloseBracket:
      if (grammar_parser -> old_state != GPStateStart) {
        grammar_parser -> state = grammar_parser -> old_state;
      }
      break;
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_header(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  char      *str, *state_str;
  char       terminal_str[2];
  grammar_t *g;
  gp_state_t state;

  g = grammar_parser -> grammar;
  state = grammar_parser -> state;
  code = token_code(token);
  str = token_token(token);
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_rule(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  char      *str, *state_str;
  char       terminal_str[2];
  grammar_t *g;
  gp_state_t state;

  g = grammar_parser -> grammar;
  state = grammar_parser -> state;
  code = token_code(token);
  str = token_token(token);

  switch (code) {
    case TokenCodeIdentifier:
      grammar_parser -> rule = rule_create(grammar_parser -> grammar, str);
      grammar_parser -> option = NULL;
      grammar_parser -> item = NULL;
      break;
    case TokenCodeOpenBracket:
      grammar_parser -> old_state = GPStateRule;
      grammar_parser -> state = GPStateOptions;
      break;
    case 200:
      if (grammar_parser -> rule) {
        rule_set_options(grammar_parser -> rule, grammar_parser -> options);
        grammar_parser -> option = rule_option_create(grammar_parser -> rule);
        grammar_parser -> state = GPStateRuleOption;
      }
      break;
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_rule_option(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  char      *str, *state_str;
  char       terminal_str[2];
  grammar_t *g;
  gp_state_t state;

  g = grammar_parser -> grammar;
  state = grammar_parser -> state;
  code = token_code(token);
  str = token_token(token);

  switch (code) {
    case TokenCodePipe:
      if ((grammar_parser -> state == GPStateItem) && (grammar_parser -> item)) {
        rule_item_set_options(grammar_parser -> item, grammar_parser -> options);
      }
      grammar_parser -> option = rule_option_create(grammar_parser -> rule);
      grammar_parser -> state = GPStateRuleOption;
      break;
    case TokenCodeSemiColon:
      if ((grammar_parser -> state == GPStateItem) && (grammar_parser -> item)) {
        rule_item_set_options(grammar_parser -> item, grammar_parser -> options);
      }
      grammar_parser -> rule = NULL;
      grammar_parser -> option = NULL;
      grammar_parser -> item = NULL;
      grammar_parser -> state = GPStateRule;
      break;
    case TokenCodeOpenBracket:
      if (grammar_parser -> state == GPStateItem) {
        grammar_parser -> old_state = grammar_parser -> state;
        grammar_parser -> state = GPStateOptions;
      }
      break;
    case TokenCodeIdentifier:
      grammar_parser -> item = rule_item_non_terminal(grammar_parser -> option, str);
      grammar_parser -> state = GPStateItem;
      break;
    case TokenCodeDQuotedStr:
      grammar_parser -> item = rule_item_terminal(grammar_parser -> option, token);
      grammar_parser -> state = GPStateItem;
      break;
    case TokenCodeSQuotedStr:
      if (strlen(str) == 1) {
        code = str[0];
        strcpy(terminal_str, str);
        token_free(token);
        token = token_create(code, terminal_str);
        grammar_parser -> item = rule_item_terminal(grammar_parser -> option, token);
      } else {
        assert(0);
      }
      grammar_parser -> state = GPStateItem;
      break;
    default:
      if ((code >= '!') && (code <= '~')) {
        grammar_parser -> item = rule_item_terminal(grammar_parser -> option, token);
        grammar_parser -> state = GPStateItem;
      } else {
        assert(0);
      }
      break;
  }
  return grammar_parser;
}

grammar_parser_t * _grammar_parser_state_item(token_t *token, grammar_parser_t *grammar_parser) {
  return _grammar_parser_state_rule_option(token, grammar_parser);
}

grammar_parser_t * _grammar_token_handler(token_t *token, grammar_parser_t *grammar_parser) {
  int        code;
  char      *str, *state_str;
  char       terminal_str[2];
  grammar_t *g;
  gp_state_t state;

  g = grammar_parser -> grammar;
  state = grammar_parser -> state;
  code = token_code(token);
  str = token_token(token);

  debug("%-18.18s [%3d] '%s'", _gp_state_recs[state].name, token_code(token), token_token(token));
  _gp_state_recs[state].handler(token, grammar_parser);
  return grammar_parser;
}

void _grammar_parser_set_option(grammar_parser_t *grammar_parser, char *str) {
  dict_t *o;

  o = grammar_parser -> options;
  if (!o) {
    grammar_parser -> options = dict_create((cmp_t) strcmp);
    o = grammar_parser -> options;
    dict_set_hash(o, (hash_t) *strhash);
    dict_set_free_key(o, (free_t) free);
    dict_set_free_data(o, (free_t) free);
  }
  dict_put(o, grammar_parser -> string, strdup(str));
  grammar_parser -> string = NULL;
}

void _grammar_dump_rule(entry_t *entry) {
  rule_dump((rule_t *) entry -> value);
}

void _grammar_get_firsts_visitor(entry_t *entry) {
  rule_t *rule;

  rule = (rule_t *) entry -> value;
  debug("Building FIRST sets for rule %s", rule -> name);
  _rule_get_firsts(rule);
}

void * _grammar_follows_reducer(entry_t *entry, int *current_sum) {
  rule_t        *rule, *r;
  int            i, j;
  rule_option_t *option;
  rule_item_t   *item, *next;
  set_t         *follows, *f, *next_firsts;

  rule = (rule_t *) entry -> value;

  follows = _rule_get_follows(rule);
  for (i = 0; i < array_size(rule -> options); i++) {
    option = (rule_option_t *) array_get(rule -> options, i);
    for (j = 0; j < array_size(option -> items); j++) {
      item = (rule_item_t *) array_get(option -> items, j);
      if (j < array_size(option -> items) - 1) {
        next = (rule_item_t *) array_get(option -> items, j + 1);
        next_firsts = intset_create();
        _rule_item_get_firsts(item, next_firsts);
      } else {
        next = NULL;
        next_firsts = NULL;
      }
      if (!item -> terminal) {
        r = grammar_get_rule(rule -> grammar, item -> rule);
        f = _rule_get_follows(r);
        if ((next == NULL) || set_has_int(next_firsts, TokenCodeEmpty)) {
          set_union(f, follows);
        } else if (next != NULL) {
          set_union(f, next_firsts);
          set_remove_int(f, TokenCodeEmpty);
        }
      }
    }
  }
  debug("Building FOLLOW sets for rule %s", rule -> name);
  if (rule -> follows) {
    set_dump(rule -> follows, stderr);
    fprintf(stderr, "\n");
  }
  *current_sum += set_size(follows);
  return current_sum;
}

int * _grammar_check_LL1_reducer(entry_t *entry, int *ok) {
  *ok &= _rule_check_LL1((rule_t *) entry -> value);
  return ok;
}

void _grammar_build_parse_table_visitor(entry_t *entry) {
  _rule_build_parse_table(entry -> value);
}

grammar_t * _grammar_analyze(grammar_t *grammar) {
  int sum, prev_sum, iter, ll_1;

  debug("Building FIRST sets");
  dict_visit(grammar -> rules, (visit_t) _grammar_get_firsts_visitor);

  debug("Building FOLLOW sets");
  sum = 0;
  iter = 1;
  do {
    prev_sum = sum;
    sum = 0;
    dict_reduce(grammar -> rules, (reduce_t) _grammar_follows_reducer, &sum);
    info("_grammar_analyze - build follows: iter: %d sum: %d", iter++, sum);
  } while (sum != prev_sum);

  debug("Checking grammar for LL(1)-ness");
  ll_1 = 1;
  dict_reduce(grammar -> rules, (reduce_t) _grammar_check_LL1_reducer, &ll_1);
  if (ll_1) {
    info("Grammar is LL(1)");
    dict_visit(grammar -> rules, (visit_t) _grammar_build_parse_table_visitor);
    debug("Built parse tables");
  } else {
    error("Grammar is not LL(1)");
  }
  return (ll_1) ? grammar : NULL;
}

/*
 * grammar_t public functions
 */

grammar_t * grammar_create() {
  grammar_t *ret;
  int ix;
  int ok = TRUE;

  ret = NEW(grammar_t);
  if (ret) {
    ret -> keywords = intdict_create();
    ok = (ret != NULL);
    if (ok) {
      dict_set_free_data(ret -> keywords, (free_t) token_free);
    }
    if (ok) {
      ret -> rules = dict_create((cmp_t) strcmp);
      ok = (ret -> rules != NULL);
      if (ok) {
        dict_set_hash(ret -> rules, (hash_t) strhash);
        dict_set_free_data(ret -> rules, (free_t) rule_free);
      }
    }
    if (ok) {
      ret -> lexer_options = array_create((int) LexerOptionLAST);
      if (ok == (ret -> lexer_options != NULL)) {
        for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
          grammar_set_option(ret, ix, 0L);
        }
      }
    }
    if (!ok) {
      grammar_free(ret);
      ret = NULL;
    }
  }
  return ret;
}

grammar_t * _grammar_read(reader_t *reader) {
  grammar_t *grammar;
  lexer_t *lexer;
  grammar_parser_t grammar_parser;

  grammar = grammar_create();
  if (grammar) {
    lexer = lexer_create(reader);
    if (lexer) {
      grammar_parser.options = dict_create((cmp_t) strcmp);
      if (grammar_parser.options) {
        dict_set_hash(grammar_parser.options, (hash_t) strhash);
        dict_set_free_data(grammar_parser.options, (free_t) free);
        dict_set_free_key(grammar_parser.options, (free_t) free);
        grammar_parser.grammar = grammar;
        grammar_parser.state = GPStateStart;
        grammar_parser.string = NULL;
        grammar_parser.rule = NULL;
        grammar_parser.option = NULL;
        grammar_parser.item = NULL;
        lexer_add_keyword(lexer, token_create(200, ":="));
        lexer_set_option(lexer, LexerOptionIgnoreWhitespace, TRUE);
        lexer_tokenize(lexer, _grammar_token_handler, &grammar_parser);
        if (_grammar_analyze(grammar)) {
          info("Grammar successfully analyzed");
        } else {
          error("Error(s) analyzing grammar");
        }
      }
      lexer_free(lexer);
    }
  }
  return grammar;
}

void grammar_free(grammar_t *grammar) {
  if (grammar) {
    dict_free(grammar -> rules);
    free(grammar);
  }
}

rule_t * grammar_get_rule(grammar_t *grammar, char *rule) {
  return (rule_t *) dict_get(grammar -> rules, rule);
}

grammar_t * grammar_set_initializer(grammar_t *grammar, parser_fnc_t init) {
  grammar -> initializer = init;
  return grammar;
}

parser_fnc_t grammar_get_initializer(grammar_t *grammar) {
  return grammar -> initializer;
}

grammar_t * grammar_set_finalizer(grammar_t *grammar, parser_fnc_t finalizer) {
  grammar -> initializer = finalizer;
  return grammar;
}

parser_fnc_t grammar_get_finalizer(grammar_t *grammar) {
  return grammar -> finalizer;
}

grammar_t * grammar_set_option(grammar_t *grammar, lexer_option_t option, long value) {
  array_set(grammar -> lexer_options, (int) option, (void *) value);
  return grammar;
}

long grammar_get_option(grammar_t *grammar, lexer_option_t option) {
  return (long) array_get(grammar -> lexer_options, (int) option);
}

void _grammar_token_dump(entry_t *entry) {
  token_dump((token_t *) entry -> value);
}

void grammar_dump(grammar_t *grammar) {
  if (dict_size(grammar -> keywords)) {
    fprintf(stderr, "< ");
    dict_visit(grammar -> keywords, (visit_t) _grammar_token_dump);
    fprintf(stderr, " >\n");
  }
  dict_visit(grammar -> rules, (visit_t) _grammar_dump_rule);
}

void grammar_set_options(grammar_t *grammar, dict_t *options) {
  char *val;
  resolve_t *resolve;
  int b;

  val = dict_get(options, "lib");
  if (val) {
    resolve_library(val);
  }
  val = dict_get(options, "init");
  if (val) {
    grammar_set_initializer(grammar, (parser_fnc_t) resolve_function(val));
  }
  val = dict_get(options, "done");
  if (val) {
    grammar_set_finalizer(grammar, (parser_fnc_t) resolve_function(val));
  }
  val = dict_get(options, "ignore_ws");
  if (val) {
    grammar_set_option(grammar, LexerOptionIgnoreWhitespace, atob(val));
  }
  val = dict_get(options, "ignore_nl");
  if (val) {
    grammar_set_option(grammar, LexerOptionIgnoreNewLines, atob(val));
  }
  val = dict_get(options, "case_sensitive");
  if (val) {
    grammar_set_option(grammar, LexerOptionCaseSensitive, atob(val));
  }
  dict_clear(options);
}

/*
 * parser_t static functions
 */

void * _parser_token_handler(token_t *token, parser_t *parser) {
  list_push(parser -> tokens, token_copy(token));
  return parser;
}

/*
 * parser_t public functions
 */

parser_t * parser_create(grammar_t *grammar) {
  parser_t *ret;

  if (ret) {
    ret -> grammar = grammar;
    ret -> tokens = list_create();
    if (ret -> tokens) {
      list_set_free(ret -> tokens, (free_t) token_free);
    } else {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

parser_t * _parser_read_grammar(reader_t *reader) {
  parser_t  *ret;
  grammar_t *grammar;

  grammar = grammar_read(reader);
  if (grammar) {
    ret = NEW(parser_t);
    if (ret) {
      ret -> grammar = grammar;
    } else {
      grammar_free(grammar);
    }
  }
  return ret;
}

lexer_t * _parser_set_keywords(entry_t *entry, lexer_t *lexer) {
  lexer_add_keyword(lexer, entry -> value);
  return lexer;
}

void _parser_parse(parser_t *parser, reader_t *reader) {
  lexer_t        *lexer;
  lexer_option_t  ix;

  lexer = lexer_create(reader);
  dict_reduce(parser -> grammar -> keywords, (reduce_t) _parser_set_keywords, lexer);
  for (ix = 0; ix < LexerOptionLAST; ix++) {
    lexer_set_option(lexer, ix, grammar_get_option(parser -> grammar, ix));
  }
  if (grammar_get_initializer(parser -> grammar)) {
    grammar_get_initializer(parser -> grammar)(parser);
  }
  lexer_tokenize(lexer, _parser_token_handler, parser);
  if (grammar_get_finalizer(parser -> grammar)) {
    grammar_get_finalizer(parser -> grammar)(parser);
  }
  lexer_free(lexer);
}

void parser_free(parser_t *parser) {
  if (parser) {
    list_free(parser -> tokens);
    grammar_free(parser -> grammar);
    free(parser);
  }
}
