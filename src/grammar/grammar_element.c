/*
 * /obelix/src/grammar/grammar_element.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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
#include "libgrammar.h"

static ge_t *          _ge_new(ge_t *, va_list);
static void            _ge_free(ge_t *);
static ge_t *          _ge_set(ge_t *, char *, data_t *);
static data_t *        _ge_resolve(ge_t *, char *);

static ge_dump_ctx_t * _dump_child(data_t *, ge_dump_ctx_t *);
static ge_t *          _ge_dump_main(ge_dump_ctx_t *);
static ge_t *          _ge_dump_common(ge_dump_ctx_t *);

static vtable_t _vtable_ge[] = {
  { .id = FunctionNew,     .fnc = (void_t) _ge_new },
  { .id = FunctionFree,    .fnc = (void_t) _ge_free },
  { .id = FunctionSet,     .fnc = (void_t) _ge_set },
  { .id = FunctionResolve, .fnc = (void_t) _ge_resolve },
  { .id = FunctionUsr1,    .fnc = (void_t) _ge_dump_main },
  { .id = FunctionNone,    .fnc = NULL }
};

grammar_element_type_t GrammarElement = -1;

/* ------------------------------------------------------------------------ */

extern void grammar_element_register(void) {
  GrammarElement = typedescr_create_and_register(GrammarElement,
                                                 "grammarelement",
                                                 _vtable_ge,
                                                 NULL);
  typedescr_set_size(GrammarElement, ge_t);
}

/* -- G R A M M A R _ E L E M E N T --------------------------------------- */

ge_t * _ge_new(ge_t *ge, va_list args) {
  ge -> grammar = va_arg(args, grammar_t *);
  if (data_type(ge) == Grammar) {
    ge -> grammar = (grammar_t *) ge;
  }
  ge -> owner = va_arg(args, ge_t *);
  ge -> actions = strdata_dict_create();
  ge -> variables = strtoken_dict_create();
  return ge;
}

void _ge_free(ge_t *ge) {
  if (ge) {
    dict_free(ge -> variables);
    dict_free(ge -> actions);
  }
}

ge_t * _ge_set(ge_t *ge, char *name, data_t *val) {
  function_t *fnc;
  ge_t       *ret = NULL;

  if (grammar_debug) {
    debug("  Setting option %s on grammar element %s", name, ge_tostring(ge));
  }
  fnc = grammar_resolve_function(ge -> grammar, name);
  if (fnc) {
    ge_add_action(ge, grammar_action_create(fnc, val));
    function_free(fnc);
    ret = ge;
  } else if (data_hastype(val, Token)) {
    ge_set_variable(ge, name, (token_t *) val);
    ret = ge;
  }
  return ge;
}

data_t * _ge_resolve(ge_t *ge, char *name) {
  data_t *ret = NULL;

  if (grammar_debug) {
    debug("  Getting option %s from grammar element %s", name, ge_tostring(ge));
  }
  ret = (data_t *) dict_get(ge -> actions, name);
  if (!ret) {
    ret = (data_t *) dict_get(ge -> variables, name);
  }
  return ret;
}

/* ------------------------------------------------------------------------ */

ge_dump_ctx_t * _ge_dump_ctx_create(ge_dump_ctx_t *parent, data_t *obj) {
  ge_dump_ctx_t *ret;

  ret = NEW(ge_dump_ctx_t);
  ret -> obj = obj;
  ret -> parent = parent;
  ret -> stream = NULL;
  return ret;
}

void _ge_dump_ctx_free(ge_dump_ctx_t *ctx) {
  if (ctx) {
    free(ctx);
  }
}

ge_t * _ge_dump_main(ge_dump_ctx_t *ctx) {
  ge_dump_fnc_t  pre = (ge_dump_fnc_t) data_get_function(ctx -> obj, FunctionUsr2);
  ge_dump_fnc_t  post = (ge_dump_fnc_t) data_get_function(ctx -> obj, FunctionUsr4);

  if (pre) {
    pre(ctx);
    printf("\n");
  }
  _ge_dump_common(ctx);
  if (post) {
    post(ctx);
  }
  printf("\n");
  return ge;
}

ge_dump_ctx_t * _dump_child(data_t *child, ge_dump_ctx_t *ctx) {
  ge_dump_fnc_t  dump = (ge_dump_fnc_t) data_get_function(child, FunctionUsr1);
  ge_dump_ctx_t *child_ctx;

  child_ctx = _ge_dump_ctx_create(ctx, child);
  dump(child_ctx);
  _ge_dump_ctx_free(child_ctx);
  return ctx;
}

ge_t * _ge_dump_common(ge_dump_ctx_t *ctx) {
  ge_get_children_fnc_t  get_children = (ge_get_children_fnc_t) data_get_function((data_t *) ge, FunctionUsr3);
  list_t                *children;

  children = list_create();
  dict_reduce_values(ge -> variables, (reduce_t) ge_append_child, children);
  dict_reduce_values(ge -> actions, (reduce_t) ge_append_child, children);
  if (get_children) {
    get_children((data_t *) ctx -> ge, children);
  }
  list_reduce(children, _dump_child, ctx);
  list_free(children);
  printf("\n");
  return ge;
}

/* ------------------------------------------------------------------------ */

ge_t * ge_add_action(ge_t *ge, grammar_action_t *action) {
  dict_put(ge -> actions, strdup(name_tostring(action -> fnc -> name)), grammar_action_copy(action));
  return ge;
}

ge_t * ge_set_variable(ge_t *ge, char *name, token_t *value) {
  grammar_variable_t *gv;

  if (data_type((data_t *) value) == GrammarVariable) {
    gv = (grammar_variable_t *) value;
  } else {
    gv = grammar_variable_create(ge, name, value);
  }
  dict_put(ge -> variables, strdup(name), gv);
  if (gv != value) {
    grammar_variable_free(gv);
  }
  return ge;
}

grammar_variable_t * ge_get_variable(ge_t *ge, char *name) {
  return (grammar_variable_t *) data_get_attribute((data_t *) ge, name);
}

ge_t * ge_dump(ge_t *ge) {
  ge_dump_fnc_t  dump = (ge_dump_fnc_t) data_get_function((data_t *) ge, FunctionUsr1);
  ge_dump_ctx_t *ctx;

  ctx = _ge_dump_ctx_create(NULL, (data_t *) ge);
  dump(ctx);
  _ge_dump_ctx_free(ctx);
  return (ge_t *) dump(ctx);
}

ge_t * ge_set_option(ge_t *ge, token_t *name, token_t *val) {
  return (ge_t *) data_set_attribute((data_t *) ge, token_token(name), (data_t *) val);
}

list_t * ge_append_child(data_t *d, list_t *children) {
  list_append(children, d);
  return children;
}
