/*
 * /obelix/src/grammar/grammar_variable.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

extern void                  grammar_init();
static grammar_variable_t *  _gv_new(grammar_variable_t *, va_list);
static void                  _gv_free(grammar_variable_t *);
static grammar_variable_t *  _gv_dump(grammar_variable_t *);

static vtable_t _vtable_GrammarVariable[] = {
  { .id = FunctionNew,         .fnc = (void_t) _gv_new },
  { .id = FunctionFree,        .fnc = (void_t) _gv_free },
  { .id = FunctionUsr1,        .fnc = (void_t) _gv_dump },
  { .id = FunctionNone,        .fnc = NULL }
};

int GrammarVariable = -1;

/* ------------------------------------------------------------------------ */

extern void grammar_variable_register(void) {
  typedescr_register(GrammarVariable, grammar_variable_t);
}

/* -- G R A M M A R _ V A R I A B L E ------------------------------------- */

grammar_variable_t * _gv_new(grammar_variable_t *gv, va_list args) {
  gv -> owner = data_copy(va_arg(args, data_t *));
  ((data_t * ) gv) -> str = strdup(va_arg(args, char *));
  gv -> value = data_copy(va_arg(args, data_t *));
  return gv;
}

void _gv_free(grammar_variable_t *gv) {
  if (gv) {
    data_free(gv -> owner);
    data_free(gv -> value);
  }
}

grammar_variable_t * _gv_dump(grammar_variable_t *gv) {
  data_t *v = gv -> value;
  char   *buf;

  printf("  ge_set_variable((ge_t *) owner, \"%s\", ", grammar_variable_tostring(gv));
  buf = data_encode(v);
  printf("data_decode(\"%s:%s\"));\n", data_typename(v), buf);
  free(buf);
  return gv;
}

/* ------------------------------------------------------------------------ */

grammar_variable_t * grammar_variable_create(ge_t *owner, char *name, data_t *value) {
  grammar_init();
  return (grammar_variable_t *) data_create(
    GrammarVariable, owner, name, value);
}
