/*
 * /obelix/src/types/list.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <core.h>
#include <data.h>
#include <exception.h>

extern void          list_init(void);

static data_t *      _list_new(int, va_list);
static void          _list_free(pointer_t *p);
static data_t *      _list_copy(data_t *, data_t *);
static int           _list_cmp(data_t *, data_t *);
static char *        _list_tostring(data_t *);
static data_t *      _list_cast(data_t *, int);
static unsigned int  _list_hash(data_t *);
static int           _list_len(data_t *);
static data_t *      _list_resolve(data_t *, char *);
static data_t *      _list_iter(data_t *);

static data_t *      _list_create(data_t *, char *, array_t *, dict_t *);
//static data_t *      _list_range(data_t *, char *, array_t *, dict_t *);
static data_t *      _list_at(data_t *, char *, array_t *, dict_t *);
static data_t *      _list_slice(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_List[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _list_new },
  { .id = FunctionCopy,     .fnc = (void_t) _list_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _list_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _list_free },
  { .id = FunctionToString, .fnc = (void_t) _list_tostring },
  { .id = FunctionCast,     .fnc = (void_t) _list_cast },
  { .id = FunctionHash,     .fnc = (void_t) _list_hash },
  { .id = FunctionLen,      .fnc = (void_t) _list_len },
  { .id = FunctionResolve,  .fnc = (void_t) _list_resolve },
  { .id = FunctionIter,     .fnc = (void_t) _list_iter },
  { .id = FunctionNone,     .fnc = NULL }
};

/* FIXME Add append, delete, head, tail, etc... */
static methoddescr_t _methods_List[] = {
  { .type = Any,    .name = "list",  .method = _list_create,.argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = List,   .name = "at",    .method = _list_at,    .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = List,   .name = "slice", .method = _list_slice, .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,    .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

typedef struct _datalist_iter {
  array_t *array;
  int      ix;
} datalist_iter_t;

static datalist_iter_t * _datalist_iter_new(datalist_iter_t *, va_list);
static void              _datalist_iter_free(datalist_iter_t *);
static data_t *          _datalist_iter_next(datalist_iter_t *);
static data_t *          _datalist_iter_has_next(datalist_iter_t *);

static vtable_t _vtable_ListIterator[] = {
  { .id = FunctionNew,      .fnc = (void_t) _datalist_iter_new },
  { .id = FunctionFree,     .fnc = (void_t) _datalist_iter_free },
  { .id = FunctionNext,     .fnc = (void_t) _datalist_iter_next },
  { .id = FunctionHasNext,  .fnc = (void_t) _datalist_iter_has_next },
  { .id = FunctionNone,     .fnc = NULL }
};

int ListIterator;

/*
 * --------------------------------------------------------------------------
 * List datatype functions
 * --------------------------------------------------------------------------
 */

void list_init(void) {
  builtin_typedescr_register(List, "list", pointer_t);
  typedescr_register(ListIterator, datalist_iter_t);
}

data_t * _list_new(int type, va_list arg) {
  pointer_t *p;
  array_t   *array;
  int        count;
  int        ix;
  data_t    *elem;

  count = va_arg(arg, int);
  array = data_array_create((count > 0) ? count : 4);
  for (ix = 0; ix < count; ix++) {
    elem = va_arg(arg, data_t *);
    array_push(array, elem);
  }
  p = data_new(List, pointer_t);
  p -> ptr = array;
  return (data_t *) p;
}

void _list_free(pointer_t *p) {
  if (p) {
    array_free((array_t *) p -> ptr);
  }
}

data_t * _list_cast(data_t *src, int totype) {
  array_t *array = data_as_array(src);
  data_t  *ret = NULL;

  if (totype == Bool) {
    ret = int_as_bool(array && array_size(array));
  }
  return ret;
}

int _list_cmp(data_t *d1, data_t *d2) {
  array_t *a2 = data_as_array(d1);
  array_t *a1 = data_as_array(d2);
  data_t  *e1;
  data_t  *e2;
  int      ix;
  int      cmp = 0;

  if (array_size(a1) != array_size(a2)) {
    return array_size(a1) - array_size(a2);
  }
  for (ix = 0; !cmp && ix < array_size(a1); ix++) {
    e1 = data_array_get(a1, ix);
    e2 = data_array_get(a2, ix);
    cmp = data_cmp(e1, e2);
  }
  return cmp;
}

data_t * _list_copy(data_t *dest, data_t *src) {
  array_t *dest_arr = data_as_array(dest);
  array_t *src_arr = data_as_array(src);
  data_t  *data;
  int      ix;

  for (ix = 0; ix < array_size(src_arr); ix++) {
    data = data_array_get(src_arr, ix);
    array_push(dest_arr, data_copy(data));
  }
  return dest;
}

char * _list_tostring(data_t *data) {
  return array_tostring(data_as_array(data));
}

unsigned int _list_hash(data_t *data) {
  return array_hash(data_as_array(data));
}

int _list_len(data_t *self) {
  return array_size(data_as_array(self));
}

data_t * _list_resolve(data_t *self, char *name) {
  long     ix;

  if (!strtoint(name, &ix)) {
    return data_list_get(self, ix);
  } else {
    return NULL;
  }
}

data_t * _list_iter(data_t *data) {
  return data_create(ListIterator, data_as_array(data));
}

/* ----------------------------------------------------------------------- */

data_t * data_create_list(array_t *array) {
  data_t  *ret;

  ret = data_create(List, 0);
  if (array) {
    array_reduce(array, (reduce_t) data_add_all_reducer, data_as_array(ret));
  }
  return ret;
}

array_t * data_list_copy(data_t *list) {
  array_t *dest;

  dest = data_array_create(array_size(data_as_array(list)));
  array_reduce(data_as_array(list), (reduce_t) data_add_all_reducer, dest);
  return dest;
}

array_t * data_list_to_str_array(data_t *list) {
  array_t *dest;

  dest = str_array_create(array_size(data_as_array(list)));
  array_reduce(data_as_array(list), (reduce_t) data_add_strings_reducer, dest);
  return dest;
}

data_t * data_str_array_to_list(array_t *src) {
  data_t  *ret;

  ret = data_create(List, 0);
  array_reduce(src, (reduce_t) data_add_all_as_data_reducer, data_as_array(ret));
  return ret;
}

data_t * data_list_push(data_t *list, data_t *value) {
  array_push(data_as_array(list), value);
  return list;
}

data_t * data_list_get(data_t *data, int ix) {
  array_t *list = data_as_array(data);
  int      sz = array_size(list);

  if ((ix >= sz) || (ix < -sz)) {
    return data_exception(ErrorRange,
                          "Index %d is not in range %d ~ %d",
                          ix, -sz, sz - 1);
  } else {
    return data_copy(data_array_get(list, ix));
  }
}

int data_list_size(data_t *list) {
  return array_size(data_as_array(list));
}

/* ----------------------------------------------------------------------- */

data_t * _list_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *ret = data_create(List, 0);

  if (args) {
    array_reduce(args, (reduce_t) data_add_all_reducer, data_as_array(ret));
  }
  return ret;
}

data_t * _list_at(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) kwargs;
  return _list_resolve(self, data_tostring(data_array_get(args, 0)));
}

data_t * _list_slice(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  array_t *slice = array_slice(data_as_array(self),
                               data_intval(data_array_get(args, 1)),
                               data_intval(data_array_get(args, 1)));
  data_t  *ret = data_create_list(slice);

  array_free(slice);
  return ret;
}

/* ----------------------------------------------------------------------- */

datalist_iter_t * _datalist_iter_new(datalist_iter_t *iter, va_list args) {
  iter -> array = array_copy(va_arg(args, array_t *));
  iter -> ix = (array_size(iter -> array)) ? 0 : -1;
  return iter;
}

void _datalist_iter_free(datalist_iter_t *iter) {
  if (iter) {
    array_free(iter -> array);
  }
}

data_t * _datalist_iter_next(datalist_iter_t *iter) {
  return data_copy(data_array_get(iter -> array, iter -> ix++));
}

data_t * _datalist_iter_has_next(datalist_iter_t *iter) {
  return int_as_bool((iter -> ix >= 0) &&
                     (iter -> ix < array_size(iter -> array)));
}
