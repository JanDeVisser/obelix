/*
 * /obelix/src/types/pointer.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <resolve.h>

static void          _ptr_init(void) __attribute__((constructor(101)));
static data_t *      _ptr_new(data_t *, va_list);
static int           _ptr_cmp(data_t *, data_t *);
static data_t *      _ptr_cast(data_t *, int);
static unsigned int  _ptr_hash(data_t *);
static char *        _ptr_tostring(data_t *);

static data_t *      _ptr_copy(data_t *, char *, array_t *, dict_t *);
static data_t *      _ptr_fill(data_t *, char *, array_t *, dict_t *);

static void          _fnc_init(void) __attribute__((constructor));
static data_t *      _fnc_new(data_t *, va_list);
static data_t *      _fnc_copy(data_t *, data_t *);
static int           _fnc_cmp(data_t *, data_t *);
static data_t *      _fnc_cast(data_t *, int);
static char *        _fnc_tostring(data_t *);
static data_t *      _fnc_parse(typedescr_t *, char *);
static unsigned int  _fnc_hash(data_t *);
static data_t *      _fnc_call(data_t *, array_t *, dict_t *);

static vtable_t _vtable_ptr[] = {
  { .id = FunctionNew,      .fnc = (void_t) _ptr_new },
  { .id = FunctionCmp,      .fnc = (void_t) _ptr_cmp },
  { .id = FunctionToString, .fnc = (void_t) _ptr_tostring },
  { .id = FunctionCast,     .fnc = (void_t) _ptr_cast },
  { .id = FunctionHash,     .fnc = (void_t) _ptr_hash },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_ptr = {
  .type =      Pointer,
  .type_name = "ptr",
  .vtable =    _vtable_ptr
};

static methoddescr_t _methoddescr_ptr[] = {
  { .type = Pointer, .name = "copy",  .method = _ptr_copy, .argtypes = { Pointer, NoType, NoType }, .minargs = 0, .varargs = 1  },
  { .type = Pointer, .name = "fill",  .method = _ptr_fill, .argtypes = { Pointer, NoType, NoType }, .minargs = 1, .varargs = 1  },
  { .type = NoType,  .name = NULL,    .method = NULL,      .argtypes = { NoType, NoType, NoType },  .minargs = 0, .varargs = 0  },
};

static vtable_t _vtable_fnc[] = {
  { .id = FunctionNew,      .fnc = (void_t) _fnc_new },
  { .id = FunctionCopy,     .fnc = (void_t) _fnc_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _fnc_cmp },
  { .id = FunctionFree,     .fnc = (void_t) function_free },
  { .id = FunctionToString, .fnc = (void_t) _fnc_tostring },
  { .id = FunctionParse,    .fnc = (void_t) _fnc_parse },
  { .id = FunctionCast,     .fnc = (void_t) _fnc_cast },
  { .id = FunctionHash,     .fnc = (void_t) _fnc_hash },
  { .id = FunctionCall,     .fnc = (void_t) _fnc_call },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_fnc = {
  .type =      Function,
  .type_name = "fnc",
  .vtable =    _vtable_fnc
};


/*
 * --------------------------------------------------------------------------
 * Pointer datatype functions
 * --------------------------------------------------------------------------
 */

static data_t _null;

void _ptr_init(void) {
  typedescr_register(&_typedescr_ptr);
  typedescr_register_methods(_methoddescr_ptr);
  data_settype(&_null, Pointer);
  _null.free_me = FALSE;
}

data_t * _ptr_new(data_t *target, va_list arg) {
  void      *ptr;
  int        size;
  pointer_t *p = NEW(pointer_t);

  p -> size = va_arg(arg, int);
  p -> ptr = va_arg(arg, void *);
  target -> ptrval = p;
  return target;
}

data_t * _ptr_cast(data_t *src, int totype) {
  data_t *ret = NULL;

  if (totype == Bool) {
    ret = data_create(Bool, data_pointerval(src) -> ptr != NULL);
  } else if (totype == Int) {
    ret = data_create(Int, (long) data_pointerval(src) -> ptr);
  }
  return ret;
}

int _ptr_cmp(data_t *d1, data_t *d2) {
  pointer_t *p1 = data_pointerval(d1);
  pointer_t *p2 = data_pointerval(d2);
  
  if (p1 -> ptr == p2 -> ptr) {
    return 0;
  } else if (p1 -> size != p2 -> size) {
    return p1 -> size - p2 -> size;
  } else {
    return memcmp(p1 -> ptr, p2 -> ptr, p1 -> size);
  }
}

char * _ptr_tostring(data_t *data) {
  pointer_t   *p = data_pointerval(data);
  static char  buf[32];

  if (data -> ptrval) {
    snprintf(buf, 32, "%p", p -> ptr);
    return buf;
  } else {
    return "Null";
  }
}

unsigned int _ptr_hash(data_t *data) {
  return hash(data_pointerval(data) -> ptr, data_pointerval(data) -> size);
}

data_t * data_null(void) {
  return &_null;
}

/* ----------------------------------------------------------------------- */

data_t * _ptr_copy(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  void      *newbuf;
  pointer_t *p = data_pointerval(self);
  
  newbuf = (void *) new(p -> size);
  memcpy(newbuf, p -> ptr, p -> size);
  return data_create(Pointer, p -> size, newbuf);
}

data_t * _ptr_fill(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  pointer_t *p = data_pointerval(self);
  data_t    *fillchar = (data_t *) array_get(args, 0);
  
  memset(p -> ptr, data_intval(fillchar), p -> size);
  return data_copy(self);
}

/*
 * --------------------------------------------------------------------------
 * Function datatype functions
 * --------------------------------------------------------------------------
 */

void _fnc_init(void) {
  typedescr_register(&_typedescr_fnc);
}

data_t * _fnc_new(data_t *target, va_list arg) {
  function_t *fnc;

  fnc = va_arg(arg, function_t *);
  target -> ptrval = function_copy(fnc);
  return target;
}

data_t * _fnc_copy(data_t *target, data_t *src) {
  target -> ptrval = function_copy(src -> ptrval);
  return target;
}

int _fnc_cmp(data_t *d1, data_t *d2) {
  function_t *fnc1;
  function_t *fnc2;

  fnc1 = d1 -> ptrval;
  fnc2 = d2 -> ptrval;
  return (int) ((long) fnc1 -> fnc) - ((long) fnc2 -> fnc);
}

char * _fnc_tostring(data_t *data) {
  function_t     *fnc;

  fnc = data -> ptrval;
  return fnc -> name;
}

data_t * _fnc_parse(typedescr_t *type, char *str) {
  void_t      f = resolve_function(str);
  function_t *fnc;
  data_t     *ret = NULL;

  (void) type;
  if (f) {
    fnc = function_create(str, (voidptr_t) f);
    ret = data_create(Function, fnc);
    function_free(fnc);
  }
  return ret;
}

data_t * _fnc_cast(data_t *src, int totype) {
  data_t     *ret = NULL;
  function_t *fnc = (function_t *) src -> ptrval;

  switch (totype) {
    case Bool:
      ret = data_create(Bool, fnc -> fnc != NULL);
      break;
    case Int:
      ret = data_create(Int, (long) fnc -> fnc);
      break;
  }
  return ret;
}

unsigned int _fnc_hash(data_t *data) {
  function_t *fnc;

  fnc = data -> ptrval;
  return hashptr(fnc -> fnc);
}

data_t * _fnc_call(data_t *data, array_t *args, dict_t *kwargs) {
  /* FIXME is there a better way? */
  function_t *fnc = (function_t *) data -> ptrval;
  return ((data_t * (*)(array_t *)) fnc -> fnc)(args);
}
