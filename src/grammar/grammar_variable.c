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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <data.h>
#include <grammar.h>

extern void                  grammar_init();
static grammar_variable_t *  _gv_new(grammar_variable_t *, va_list);
static void                  _gv_free(grammar_variable_t *);
static grammar_variable_t *  _gv_dump(grammar_variable_t *);

static vtable_t _vtable_gv[] = {
  { .id = FunctionNew,         .fnc = (void_t) _gv_new },
  { .id = FunctionFree,        .fnc = (void_t) _gv_free },
  { .id = FunctionUsr1,        .fnc = (void_t) _gv_dump },
  { .id = FunctionNone,        .fnc = NULL }
};

grammar_element_type_t GrammarVariable = -1;

/* ------------------------------------------------------------------------ */

extern void grammar_variable_register(void) {
  GrammarVariable = typedescr_create_and_register(GrammarVariable,
                                                "grammarvariable",
                                                _vtable_gv,
                                                NULL);
  typedescr_set_size(GrammarVariable, grammar_variable_t);
  typedescr_assign_inheritance(GrammarVariable, Token);
}

/* -- G R A M M A R _ A C T I O N ----------------------------------------- */

grammar_variable_t * _gv_new(grammar_variable_t *gv, va_list args) {
  va_arg(args, unsigned int);
  va_arg(args, char *);
  gv -> owner = data_copy(va_arg(args, data_t *));
  ((data_t * ) gv) -> str = strdup(va_arg(args, char *));
  return gv;
}

void _gv_free(grammar_variable_t *gv) {
  if (gv) {
    data_free(gv -> owner);
  }
}

grammar_variable_t * _gv_dump(grammar_variable_t *gv) {
  char *prefix;
  char *variable;

  prefix = data_typename(gv -> owner);
  variable = data_typename(gv -> owner);
  printf("  %s_set_variable(%s, \"%s\", token_create(%d, \"%s\"));\n",
         prefix, variable, data_tostring((data_t *) gv),
         token_code((token_t *) gv), token_token((token_t *) gv));
  return gv;
}

/* ------------------------------------------------------------------------ */

grammar_variable_t * grammar_variable_create(ge_t *owner, char *name, token_t *token) {
  grammar_init();
  return (grammar_variable_t *) data_create(
    GrammarVariable,
    token_code(token), token_token(token),
    owner, name);
}
