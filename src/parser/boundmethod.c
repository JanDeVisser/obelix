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

#include <stdio.h>

#include <boundmethod.h>
#include <wrapper.h>

/* ------------------------------------------------------------------------ */

static void _bound_method_init(void) __attribute__((constructor(102)));

static vtable_t _wrapper_vtable_bound_method[] = {
  { .id = FunctionCopy,     .fnc = (void_t) bound_method_copy },
  { .id = FunctionCmp,      .fnc = (void_t) bound_method_cmp },
  { .id = FunctionFree,     .fnc = (void_t) bound_method_free },
  { .id = FunctionToString, .fnc = (void_t) bound_method_tostring },
  { .id = FunctionCall,     .fnc = (void_t) bound_method_execute },
  { .id = FunctionNone,     .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _bound_method_init(void) {
  wrapper_register(BoundMethod, "boundmethod", _wrapper_vtable_bound_method);
}

/* -- B O U N D  M E T H O D  F U N C T I O N S   ------------------------- */

bound_method_t * bound_method_create(script_t *script, object_t *self) {
  bound_method_t *ret = NEW(bound_method_t);
  
  ret -> script = script_copy(script);
  ret -> self = object_copy(self);
  ret -> closure = NULL;
  ret -> str = NULL;
  ret -> refs = 1;
  return ret;
}

void bound_method_free(bound_method_t *bm) {
  if (bm && ((--bm -> refs) <= 0)) {
    script_free(bm -> script);
    object_free(bm -> self);
    closure_free(bm -> closure);
    free(bm -> str);
    free(bm);
  }
}

bound_method_t * bound_method_copy(bound_method_t *bm) {
  if (bm) {
    bm -> refs++;
  }
  return bm;
}

int bound_method_cmp(bound_method_t *bm1, bound_method_t *bm2) {
  return (!object_cmp(bm1 -> self, bm2 -> self)) 
  ? script_cmp(bm1 -> script, bm2 -> script)
  : 0;
}

char * bound_method_tostring(bound_method_t *bm) {
  if (bm -> script) {
    asprintf(&bm -> str, "%s (bound)", 
             script_tostring(bm -> script));
  } else {
    bm -> str = strdup("uninitialized");
  }
  return bm -> str;
}

data_t * bound_method_execute(bound_method_t *bm, array_t *params, dict_t *kwparams) {
  closure_t *closure;
  data_t    *self;
  data_t    *ret;
  
  self = (bm -> self) ? data_create(Object, bm -> self) : NULL;
  closure = closure_create(bm -> script, bm -> closure, self);
  ret = closure_execute(closure, params, kwparams);
  closure_free(closure);
  return ret;  
}

