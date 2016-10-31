/*
 * /obelix/src/grammar/grammar_action.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

static grammar_action_t *  _ga_new(grammar_action_t *, va_list);
static char *              _ga_allocstring(grammar_action_t *);
static void                _ga_free(grammar_action_t *);
static grammar_action_t *  _ga_dump(grammar_action_t *, char *, char *);

static vtable_t _vtable_ga[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ga_new },
  { .id = FunctionFree,        .fnc = (void_t) _ga_free },
  { .id = FunctionCmp,         .fnc = (void_t) grammar_action_cmp },
  { .id = FunctionHash,        .fnc = (void_t) grammar_action_hash },
  { .id = FunctionAllocString, .fnc = (void_t) _ga_allocstring },
  { .id = FunctionUsr1,        .fnc = (void_t) _ga_dump },
  { .id = FunctionNone,        .fnc = NULL }
};

grammar_element_type_t GrammarAction = -1;

/* ------------------------------------------------------------------------ */

extern void grammar_action_register(void) {
  GrammarAction = typedescr_create_and_register(GrammarAction,
                                                "grammaraction",
                                                _vtable_ga,
                                                NULL);
  typedescr_set_size(GrammarAction, grammar_action_t);
}

/* -- G R A M M A R _ A C T I O N ----------------------------------------- */

grammar_action_t * _ga_new(grammar_action_t *ga, va_list args) {
  ga -> fnc = function_copy(va_arg(args, function_t *));
  ga -> data = data_copy(va_arg(args, data_t *));
  return ga;
}

void _ga_free(grammar_action_t *grammar_action) {
  if (grammar_action) {
    function_free(grammar_action -> fnc);
    data_free(grammar_action -> data);
  }
}

char * _ga_allocstring(grammar_action_t *grammar_action) {
  char *buf;

  if (grammar_action -> data) {
    asprintf(&buf, "%s [%s]",
	     function_tostring(grammar_action -> fnc),
	     data_tostring(grammar_action -> data));
  } else {
    asprintf(&buf, "%s",
	     function_tostring(grammar_action -> fnc));
  }
  return buf;
}

grammar_action_t * _ga_dump(grammar_action_t *ga, char *prefix, char *variable) {
  function_t *fnc = ga -> fnc;
  data_t     *d = ga -> data;
  char       *data = NULL;
  char       *encoded;

  if (d) {
    encoded = data_encode(d);
    asprintf(&data, "data_decode(\"%s\")", encoded);
    free(encoded);
  }

  printf("  ge_add_action((ge_t *) owner,\n"
         "    grammar_action_create(\n"
         "      grammar_resolve_function(grammar, \"%s\"), %s));\n",
         function_tostring(fnc), (data) ? data : "NULL");
  free(data);
  return ga;
}

/* ------------------------------------------------------------------------ */

grammar_action_t * grammar_action_create(function_t *fnc, data_t *data) {
  grammar_init();
  return (grammar_action_t *) data_create(GrammarAction, fnc, data);
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
