/*
 * /obelix/src/types/wrapper.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <exception.h>
#include <wrapper.h>

static data_t *     _wrapper_new(data_t *, va_list);
static void         _wrapper_free(data_t *);
static data_t *     _wrapper_copy(data_t *, data_t *);
static int          _wrapper_cmp(data_t *, data_t *);
static unsigned int _wrapper_hash(data_t *);
static char *       _wrapper_tostring(data_t *);
static data_t *     _wrapper_call(data_t *, array_t *, dict_t *);
static data_t *     _wrapper_resolve(data_t *, char *);
static data_t *     _wrapper_set(data_t *, char *, data_t *);

static vtable_t _vtable_wrapper[] = {
  { .id = FunctionNew,      .fnc = (void_t) _wrapper_new },
  { .id = FunctionCopy,     .fnc = (void_t) _wrapper_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _wrapper_cmp },
  { .id = FunctionHash,     .fnc = (void_t) _wrapper_hash },
  { .id = FunctionFreeData, .fnc = (void_t) _wrapper_free },
  { .id = FunctionToString, .fnc = (void_t) _wrapper_tostring },
  { .id = FunctionResolve,  .fnc = (void_t) _wrapper_resolve },
  { .id = FunctionCall,     .fnc = (void_t) _wrapper_call },
  { .id = FunctionSet,      .fnc = (void_t) _wrapper_set },
  { .id = FunctionNone,     .fnc = NULL }
};

#define wrapper_function(type, fnc_id) (((type) -> ptr) \
  ? vtable_get((vtable_t *) (type) -> ptr, (fnc_id)) \
  : NULL)

/* -- W R A P P E R  S T A T I C  F U N C T I O N S ----------------------- */

data_t * _wrapper_new(data_t *ret, va_list arg) {
  typedescr_t *type = data_typedescr(ret);
  void_t       fnc;
  void        *src;
  
  fnc = wrapper_function(type, FunctionFactory);
  if (fnc) {
    ret -> ptrval = ((vcreate_t) fnc)(arg);
  } else {
    src = va_arg(arg, void *);
    fnc = wrapper_function(type, FunctionCopy);
    if (fnc) {
      ret -> ptrval = ((voidptrvoidptr_t) fnc)(src);
    } else {
      ret -> ptrval = src;
    }
  }
  return ret;
}

void _wrapper_free(data_t *data) {
  typedescr_t *type = data_typedescr(data);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionFree);
  if (fnc) {
    ((free_t) fnc)(data -> ptrval);
  } else {
    warning("No free method defined for wrapper type '%s'", type -> type_name);
  }
}

data_t * _wrapper_copy(data_t *target, data_t *src) {
  typedescr_t *type = data_typedescr(src);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionCopy);
  if (fnc) {
    target -> ptrval = ((voidptrvoidptr_t) fnc)(src -> ptrval);
  } else {
    target -> ptrval = src -> ptrval;
  }
  return target;
}

int _wrapper_cmp(data_t *d1, data_t *d2) {
  typedescr_t *type = data_typedescr(d1);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionCmp);
  if (fnc) {
    return ((cmp_t) fnc)(d1 -> ptrval, d2 -> ptrval);
  } else {
    return (int) ((unsigned long) d1 -> ptrval) - ((unsigned long) d2 -> ptrval);
  }
}

unsigned int _wrapper_hash(data_t *data) {
  typedescr_t *type = data_typedescr(data);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionHash);
  if (fnc) {
    return ((hash_t) fnc)(data -> ptrval);
  } else {
    return hashptr(data -> ptrval);
  }
}

char * _wrapper_tostring(data_t *data) {
  typedescr_t *type = data_typedescr(data);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionToString);
  if (fnc) {
    return ((tostring_t) fnc)(data -> ptrval);
  } else {
    return (char *) data -> ptrval;
  }
}

data_t * _wrapper_call(data_t *self, array_t *params, dict_t *kwargs) {
  typedescr_t *type = data_typedescr(self);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionCall);
  if (!fnc) {
    return data_error(ErrorInternalError,
                      "No 'call' method defined for wrapper type '%s'", 
                      type -> type_name);
  } else {
    return ((call_t) fnc)(self -> ptrval, params, kwargs);
  }
}

data_t * _wrapper_resolve(data_t *data, char *name) {
  typedescr_t *type = data_typedescr(data);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionResolve);
  if (!fnc) {
    return data_error(ErrorInternalError,
                      "No 'resolve' method defined for wrapper type '%s'", 
                      type -> type_name);
  } else {
    return ((resolve_name_t) fnc)(data -> ptrval, name);
  }
}

data_t * _wrapper_set(data_t *data, char *name, data_t *value) {
  typedescr_t *type = data_typedescr(data);
  void_t       fnc;
  
  fnc = wrapper_function(type, FunctionResolve);
  if (!fnc) {
    return data_error(ErrorInternalError,
                      "No 'set' method defined for wrapper type '%s'", 
                      type -> type_name);
  } else {
    return ((setvalue_t) fnc)(data -> ptrval, name, value);
  }
}

/* -- W R A P P E R  P U B L I C  F U N C T I O N S ----------------------- */

int wrapper_register(int type, char *name, vtable_t *vtable) {
  typedescr_t    descr;
  
  descr.type = type;
  descr.type_name = name;
  descr.ptr = vtable;
  descr.vtable = _vtable_wrapper;
  return typedescr_register(&descr);
}

