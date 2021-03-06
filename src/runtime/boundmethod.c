/*
 * /obelix/src/parser/boundmethod.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libruntime.h"

/* ------------------------------------------------------------------------ */

static inline void      _bound_method_init(void);
static bound_method_t * _bound_method_new(bound_method_t *, va_list);
static void             _bound_method_free(bound_method_t *);
static char *           _bound_method_allocstring(bound_method_t *);

static vtable_t _vtable_BoundMethod[] = {
  { .id = FunctionNew,         .fnc = (void_t) _bound_method_new },
  { .id = FunctionCmp,         .fnc = (void_t) bound_method_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _bound_method_free },
  { .id = FunctionAllocString, .fnc = (void_t) _bound_method_allocstring },
  { .id = FunctionCall,        .fnc = (void_t) bound_method_execute },
  { .id = FunctionNone,        .fnc = NULL }
};

int BoundMethod = -1;

/* ------------------------------------------------------------------------ */

void _bound_method_init(void) {
  typedescr_register(BoundMethod, bound_method_t);
}

/* -- B O U N D  M E T H O D  S T A T I C  F U N C T I O N S -------------- */

bound_method_t * _bound_method_new(bound_method_t *bm, va_list args) {
  bm -> script = script_copy(va_arg(args, script_t *));
  bm -> self = object_copy(va_arg(args, object_t *));
  bm -> closure = NULL;
  return bm;
}

void _bound_method_free(bound_method_t *bm) {
  if (bm) {
    script_free(bm -> script);
    object_free(bm -> self);
    closure_free(bm -> closure);
  }
}

char * _bound_method_allocstring(bound_method_t *bm) {
  char *buf;

  if (bm -> script) {
    asprintf(&buf, "%s (bound)",
             script_tostring(bm -> script));
  } else {
    buf = strdup("uninitialized");
  }
  return buf;
}

/* -- B O U N D  M E T H O D  P U B L I C  F U N C T I O N S   ------------ */

bound_method_t * bound_method_create(script_t *script, object_t *self) {
  _bound_method_init();
  return (bound_method_t *) data_create(BoundMethod, script, self);
}

int bound_method_cmp(bound_method_t *bm1, bound_method_t *bm2) {
  int cmp = object_cmp(bm1 -> self, bm2 -> self);

  return (!cmp)
    ? script_cmp(bm1 -> script, bm2 -> script)
    : cmp;
}

closure_t * bound_method_get_closure(bound_method_t *bm) {
  data_t *self;

  self = (data_t *) ((bm -> self) ? object_copy(bm -> self) : NULL);
  return closure_create(bm -> script, bm -> closure, self);
}

data_t * bound_method_execute(bound_method_t *bm, arguments_t *args) {
  closure_t *closure;
  data_t    *ret;

  closure = bound_method_get_closure(bm);
  ret = closure_execute(closure, args);
  closure_free(closure);
  return ret;
}
