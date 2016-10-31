/*
 * /obelix/src/grammar/rule_entry.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

static rule_entry_t * _rule_entry_new(rule_entry_t *, va_list);
static void           _rule_entry_free(rule_entry_t *);
static char *         _rule_entry_allocstring(rule_entry_t *);
static rule_entry_t * _rule_entry_dump_pre(rule_entry_t *);

static rule_entry_t * _rule_entry_create(rule_t *, int, void *);

static vtable_t _vtable_rule_entry[] = {
  { .id = FunctionNew,         .fnc = (void_t) _rule_entry_new },
  { .id = FunctionFree,        .fnc = (void_t) _rule_entry_free },
  { .id = FunctionAllocString, .fnc = (void_t) _rule_entry_allocstring },
  { .id = FunctionUsr2,        .fnc = (void_t) _rule_entry_dump_pre },
  { .id = FunctionNone,        .fnc = NULL }
};

grammar_element_type_t RuleEntry = -1;

/* ------------------------------------------------------------------------ */

extern void rule_entry_register(void) {
  RuleEntry = typedescr_create_and_register(
      RuleEntry, "rule_entry", _vtable_rule_entry, NULL);
  typedescr_set_size(RuleEntry, rule_entry_t);
  typedescr_assign_inheritance(RuleEntry, GrammarElement);
}

/* -- R U L E _ E N T R Y  S T A T I C  F U N C T I O N S ----------------- */

rule_entry_t * _rule_entry_new(rule_entry_t *entry, va_list args) {
  rule_t *rule;
  int     terminal;
  void   *ptr;

  va_arg(args, grammar_t *);
  rule = va_arg(args, rule_t *);
  terminal = va_arg(args, int);
  ptr = va_arg(args, void *);
  entry -> terminal = terminal;
  if (terminal) {
    entry -> token = (ptr) ? token_copy((token_t *) ptr)
                           : token_create(TokenCodeEmpty, "E");
  } else {
    entry -> nonterminal = strdup((char *) ptr);
  }
  array_push(rule -> entries, entry);
  return entry;
}

char * _rule_entry_allocstring(rule_entry_t *entry) {
  char *buf;

  if (entry -> terminal) {
    asprintf(&buf, "'%s'", token_token(entry -> token));
  } else {
    buf = strdup(entry -> nonterminal);
  }
  return buf;
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

rule_entry_t * _rule_entry_dump_pre(rule_entry_t *entry) {
  if (entry -> terminal) {
    printf("  ge = (ge_t *) rule_entry_terminal((rule_t *) owner, token_create(%d, \"%s\"));\n",
           token_code(entry -> token),
           (token_code(entry -> token) != 34) ? token_token(entry -> token) : "\\\"");
  } else {
    printf("  ge = (ge_t *) rule_entry_non_terminal((rule_t *) owner, \"%s\");\n",
           entry -> nonterminal);
  }
  return entry;
}

/* ----------------------------------------------------------------------- */

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

rule_entry_t * _rule_entry_create(rule_t *rule, int terminal, void *ptr) {
  grammar_init();
  return (rule_entry_t *) data_create(RuleEntry,
                            rule_get_grammar(rule), rule, terminal, ptr);
}

/* -- R U L E _ E N T R Y  P U B L I C  F U N C T I O N S ----------------- */

rule_entry_t * rule_entry_non_terminal(rule_t *rule, char *nonterminal) {
  return _rule_entry_create(rule, FALSE, nonterminal);
}

rule_entry_t * rule_entry_terminal(rule_t *rule, token_t *token) {
  unsigned int  code;
  char         *str;

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
