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

#include "libcore.h"
#include <data.h>
#include <dictionary.h>
#include <str.h>

extern void          ptr_init(void);

static pointer_t *   _ptr_new(pointer_t *, va_list);
static int           _ptr_cmp(pointer_t *, pointer_t *);
static data_t *      _ptr_cast(pointer_t *, int);
static unsigned int  _ptr_hash(pointer_t *);
static char *        _ptr_allocstring(pointer_t *);
static pointer_t *   _ptr_parse(char *);
static data_t *      _ptr_serialize(pointer_t *);
static pointer_t *   _ptr_deserialize(data_t *);

static pointer_t *   _ptr_copy(pointer_t *, char *, arguments_t *);
static pointer_t *   _ptr_fill(pointer_t *, char *, arguments_t *);

static vtable_t _vtable_Pointer[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ptr_new },
  { .id = FunctionCmp,         .fnc = (void_t) _ptr_cmp },
  { .id = FunctionAllocString, .fnc = (void_t) _ptr_allocstring },
  { .id = FunctionCast,        .fnc = (void_t) _ptr_cast },
  { .id = FunctionHash,        .fnc = (void_t) _ptr_hash },
  { .id = FunctionParse,       .fnc = (void_t) _ptr_parse },
  { .id = FunctionSerialize,   .fnc = (void_t) _ptr_serialize },
  { .id = FunctionDeserialize, .fnc = (void_t) _ptr_deserialize },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Pointer[] = {
  { .type = Pointer, .name = "copy",  .method = (method_t) _ptr_copy, .argtypes = { Pointer, Int, NoType },    .minargs = 0, .varargs = 1  },
  { .type = Pointer, .name = "fill",  .method = (method_t) _ptr_fill, .argtypes = { Int, NoType, NoType },     .minargs = 1, .varargs = 0  },
  { .type = NoType,  .name = NULL,    .method = NULL,                 .argtypes = { NoType, NoType, NoType },  .minargs = 0, .varargs = 0  },
};

/*
 * --------------------------------------------------------------------------
 * Pointer datatype functions
 * --------------------------------------------------------------------------
 */

static pointer_t * _null = NULL;

void ptr_init(void) {
  builtin_typedescr_register(Pointer, "ptr", pointer_t);
  _null = ptr_create(0, NULL);
  _null -> _d.data_semantics = DataSemanticsConstant;
}

pointer_t * _ptr_new(pointer_t *ptr, va_list args) {
  if (_null && !ptr) {
    ptr = _null;
  } else {
    ptr -> size = va_arg(args, size_t);
    ptr -> ptr = va_arg(args, void *);
  }
  return ptr;
}

data_t * _ptr_cast(pointer_t *src, int totype) {
  data_t *ret = NULL;

  if (totype == Bool) {
    ret = int_as_bool(src -> ptr != NULL);
  } else if (totype == Int) {
    ret = int_to_data((intptr_t) src -> ptr);
  }
  return ret;
}

int _ptr_cmp(pointer_t *p1, pointer_t *p2) {
  if (p1 -> ptr == p2 -> ptr) {
    return 0;
  } else if (p1 -> size != p2 -> size) {
    return p1 -> size - p2 -> size;
  } else {
    return memcmp(p1 -> ptr, p2 -> ptr, p1 -> size);
  }
}

char * _ptr_allocstring(pointer_t *p) {
  char *buf;

  if (!p || p == _null) {
    buf = strdup("null");
  } else {
    asprintf(&buf, "%p", p -> ptr);
  }
  return buf;
}

pointer_t * _ptr_parse(char *str) {
  long l;

  if (!strcasecmp(str, "null")) {
    return (pointer_t *) data_null();
  } else {
    if (strtoint(str, &l)) {
      return (pointer_t *) ptr_to_data(0, (void *) (intptr_t) l);
    } else {
      return (pointer_t *) data_null();
    }
  }
}

pointer_t * _ptr_deserialize(data_t *data) {
  pointer_t    *ret = NULL;
  data_t       *value;

  if (data_is_pointer(data)) {
    ret = data_as_pointer(data);
    if (data_isnull((data_t *) ret)) {
      ret = NULL;
    }
  } else if (data_is_dictionary(data)) {
    value = dictionary_get(data_as_dictionary(data), "value");
    ret = _ptr_parse(data_tostring(value));
    data_free(value);
  } else if (data) {
    ret = ptr_create(data_typedescr(data) -> size, data);
  }
  return ret;
}

data_t * _ptr_serialize(pointer_t *ptr) {
  dictionary_t *ret;

  if (data_isnull((data_t *) ptr)) {
    return (data_t *) ptr;
  } else {
    ret = dictionary_create(NULL);
    dictionary_set(ret, "value",
        data_uncopy((data_t *) str_copy_chars(data_tostring(ptr))));
    return (data_t *) ret;
  }
}

unsigned int _ptr_hash(pointer_t *data) {
  return hash(data -> ptr, data -> size);
}

/* ----------------------------------------------------------------------- */

pointer_t * _ptr_copy(pointer_t *p, char *name, arguments_t *args) {
  void      *newbuf;

  newbuf = new(p -> size);
  memcpy(newbuf, p -> ptr, p -> size);
  return ptr_create(p -> size, newbuf);
}

pointer_t * _ptr_fill(pointer_t *p, char *name, arguments_t *args) {
  data_t    *fillchar = arguments_get_arg(args, 0);

  memset(p -> ptr, data_intval(fillchar), p -> size);
  return pointer_copy(p);
}

/* ----------------------------------------------------------------------- */

data_t * data_null(void) {
  return (data_t *) _null;
}

/* ----------------------------------------------------------------------- */
