/*
 * /obelix/src/grammar/rule.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

static rule_t *            _rule_new(rule_t *, va_list);
static void                _rule_free(rule_t *);
static void *              _rule_reduce_children(rule_t *, reduce_t, void *);
static char *              _rule_tostring(rule_t *);
static rule_t *            _rule_dump_pre(rule_t *rule);
static list_t *            _rule_dump_get_children(rule_t *rule, list_t *);

static vtable_t _vtable_Rule[] = {
  { .id = FunctionNew,      .fnc = (void_t) _rule_new },
  { .id = FunctionFree,     .fnc = (void_t) _rule_free },
  { .id = FunctionReduce,   .fnc = (void_t) _rule_reduce_children },
  { .id = FunctionToString, .fnc = (void_t) _rule_tostring },
  { .id = FunctionUsr2,     .fnc = (void_t) _rule_dump_pre },
  { .id = FunctionUsr3,     .fnc = (void_t) _rule_dump_get_children },
  { .id = FunctionNone,     .fnc = NULL }
};

int Rule = -1;

/* ------------------------------------------------------------------------ */

extern void rule_register(void) {
  typedescr_register(Rule, rule_t);
  typedescr_assign_inheritance(Rule, GrammarElement);
}

/* -- R U L E  S T A T I C  F U N C T I O N S ----------------------------- */

rule_t * _rule_new(rule_t *rule, va_list args) {
  nonterminal_t *nonterminal;

  va_arg(args, grammar_t *);
  nonterminal = va_arg(args, nonterminal_t *);
  rule -> firsts = NULL;
  rule -> follows = NULL;
  rule -> entries = datalist_create(NULL);
  datalist_push(nonterminal -> rules, rule);
  return rule;
}

void _rule_free(rule_t *rule) {
  if (rule) {
    set_free(rule -> firsts);
    set_free(rule -> follows);
  }
}

void * _rule_reduce_children(rule_t *rule, reduce_t reducer, void *ctx) {
  return reducer(rule->entries, ctx);
}

char * _rule_tostring(rule_t *rule) {
  return datalist_tostring(rule -> entries);
}

rule_t * _rule_dump_pre(rule_t *rule) {
  printf("  ge = (ge_t *) rule_create((nonterminal_t *) owner);\n");
  return rule;
}

list_t * _rule_dump_get_children(rule_t *rule, list_t *children) {
  return datalist_reduce(rule -> entries, (reduce_t) ge_append_child, children);
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
set_t * _rule_get_firsts(rule_t *rule) {
  rule_entry_t *e;
  int           j;

  if (!rule -> firsts) {
    rule -> firsts = intset_create();
    if (rule -> firsts) {
      set_add_int(rule -> firsts, TokenCodeEmpty);
      for (j = 0; set_has_int(rule -> firsts, TokenCodeEmpty) && (j < datalist_size(rule -> entries)); j++) {
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

/* -- R U L E  P U B L I C  F U N C T I O N S ----------------------------- */

rule_t * rule_create(nonterminal_t *nonterminal) {
  grammar_init();
  return (rule_t *) data_create(Rule, nonterminal_get_grammar(nonterminal), nonterminal);
}

rule_entry_t * rule_get_entry(rule_t *rule, int ix) {
  return (rule_entry_t *) datalist_get(rule->entries, ix);
}

