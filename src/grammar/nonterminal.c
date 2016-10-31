/*
 * /obelix/src/grammar/nonterminal.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>

#include "libgrammar.h"

static nonterminal_t * _nonterminal_new(nonterminal_t *, va_list);
static void            _nonterminal_free(nonterminal_t *);
extern char *          _nonterminal_tostring(nonterminal_t *);
static nonterminal_t * _nonterminal_dump_pre(nonterminal_t *);
static nonterminal_t * _nonterminal_dump_get_children(nonterminal_t *, list_t *);

static vtable_t _vtable_nonterminal[] = {
  { .id = FunctionNew,      .fnc = (void_t) _nonterminal_new },
  { .id = FunctionFree,     .fnc = (void_t) _nonterminal_free },
  { .id = FunctionToString, .fnc = (void_t) _nonterminal_tostring },
  { .id = FunctionUsr2,     .fnc = (void_t) _nonterminal_dump_pre },
  { .id = FunctionUsr3,     .fnc = (void_t) _nonterminal_dump_get_children },
  { .id = FunctionNone,     .fnc = NULL }
};

grammar_element_type_t NonTerminal = -1;

/* ------------------------------------------------------------------------ */

extern void nonterminal_register(void) {
  NonTerminal = typedescr_create_and_register(
      NonTerminal, "nonterminal", _vtable_nonterminal, NULL);
  typedescr_set_size(NonTerminal, nonterminal_t);
  typedescr_assign_inheritance(NonTerminal, GrammarElement);
}

/* -- N O N T E R M I N A L  F U N C T I O N S --------------------------- */

nonterminal_t * _nonterminal_new(nonterminal_t *nonterminal, va_list args) {
  grammar_t *grammar = va_arg(args, grammar_t *); // grammar

  va_arg(args, ge_t *); // owner
  nonterminal -> firsts = NULL;
  nonterminal -> follows = NULL;
  nonterminal -> parse_table = NULL;
  nonterminal -> name = strdup(va_arg(args, char *));
  nonterminal -> rules = data_array_create(2);
  nonterminal -> state = strhash(nonterminal -> name);
  dict_put(grammar -> nonterminals, strdup(nonterminal -> name), nonterminal);
  if (!grammar -> entrypoint) {
    grammar -> entrypoint = nonterminal;
  }
  return nonterminal;
}

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

nonterminal_t * _nonterminal_dump_pre(nonterminal_t *nonterminal) {
  printf("  ge = (ge_t *) nonterminal_create(grammar, \"%s\");\n", nonterminal -> name);
  return nonterminal;
}

nonterminal_t * _nonterminal_dump_get_children(nonterminal_t *nt, list_t *children) {
  array_reduce(nt -> rules, (reduce_t) ge_append_child, children);
  return nt;
}

/* ------------------------------------------------------------------------ */

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
  set_t  *f_i, *f_j;

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
        ok = set_disjoint(f_i, _rule_get_follows(r_i));
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

/*
 * nonterminal_t public functions
 */

nonterminal_t * nonterminal_create(grammar_t *grammar, char *name) {
  grammar_init();
  return (nonterminal_t *) data_create(NonTerminal, grammar, grammar, name);
}

rule_t * nonterminal_get_rule(nonterminal_t *nonterminal, int ix) {
  assert(ix >= 0);
  assert(ix < array_size(nonterminal -> rules));
  return (rule_t *) array_get(nonterminal -> rules, ix);
}
