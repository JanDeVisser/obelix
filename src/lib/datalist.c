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
#include <dictionary.h>
#include <exception.h>

typedef struct _datalist_iter {
  data_t   _d;
  array_t *array;
  int      ix;
} datalist_iter_t;

extern void              list_init(void);

static datalist_t *      _list_new(int, va_list);
static void              _list_free(datalist_t *);
static datalist_t *      _list_copy(datalist_t *, datalist_t *);
static int               _list_cmp(datalist_t *, datalist_t *);
static char *            _list_tostring(datalist_t *);
static data_t *          _list_cast(datalist_t *, int);
static unsigned int      _list_hash(datalist_t *);
static int               _list_len(datalist_t *);
static data_t *          _list_resolve(datalist_t *, char *);
static datalist_iter_t * _list_iter(datalist_t *);
static char *            _list_encode(pointer_t *);
static datalist_t *      _list_serialize(datalist_t *);
static datalist_t *      _list_deserialize(datalist_t *);
static void *            _list_reduce_children(datalist_t *, reduce_t, void *ctx);

static datalist_t *      _list_create(data_t *, char *, arguments_t *);
//static data_t *          _list_range(datalist_t *, char *, arguments_t *);
static data_t *          _list_at(datalist_t *, char *, arguments_t *);
static datalist_t *      _list_slice(datalist_t *, char *, arguments_t *);

static vtable_t _vtable_List[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _list_new },
  { .id = FunctionCopy,        .fnc = (void_t) _list_copy },
  { .id = FunctionCmp,         .fnc = (void_t) _list_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _list_free },
  { .id = FunctionToString,    .fnc = (void_t) _list_tostring },
  { .id = FunctionCast,        .fnc = (void_t) _list_cast },
  { .id = FunctionHash,        .fnc = (void_t) _list_hash },
  { .id = FunctionLen,         .fnc = (void_t) _list_len },
  { .id = FunctionResolve,     .fnc = (void_t) _list_resolve },
  { .id = FunctionIter,        .fnc = (void_t) _list_iter },
  { .id = FunctionEncode,      .fnc = (void_t) _list_encode },
  { .id = FunctionSerialize,   .fnc = (void_t) _list_serialize },
  { .id = FunctionDeserialize, .fnc = (void_t) _list_deserialize },
  { .id = FunctionPush,        .fnc = (void_t) _datalist_push },
  { .id = FunctionPop,         .fnc = (void_t) datalist_pop },
  { .id = FunctionReduce,      .fnc = (void_t) _list_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

/* FIXME Add append, delete, head, tail, etc... */
static methoddescr_t _methods_List[] = {
  { .type = Any,    .name = "list",  .method = (method_t) _list_create,  .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = List,   .name = "at",    .method = (method_t) _list_at,      .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = List,   .name = "slice", .method = (method_t) _list_slice,   .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,    .method = NULL,                     .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

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

void datalist_init(void) {
  builtin_typedescr_register(List, "list", pointer_t);
  typedescr_register(ListIterator, datalist_iter_t);
}

datalist_t * _list_new(int type, va_list arg) {
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
  return (datalist_t *) p;
}

void _list_free(pointer_t *p) {
  if (p) {
    array_free((array_t *) p -> ptr);
  }
}

data_t * _list_cast(datalist_t *src, int totype) {
  array_t *array = data_as_array(src);
  data_t  *ret = NULL;

  if (totype == Bool) {
    ret = int_as_bool(array && array_size(array));
  }
  return ret;
}

int _list_cmp(datalist_t *d1, datalist_t *d2) {
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

datalist_t * _list_copy(datalist_t *dest, datalist_t *src) {
  array_t *dest_arr = data_as_array(dest);
  array_t *src_arr = data_as_array(src);
  data_t  *data;
  int      ix;

  for (ix = 0; ix < array_size(src_arr); ix++) {
    data = data_array_get(src_arr, ix);
    array_push(dest_arr, data);
  }
  return dest;
}

char * _list_tostring(datalist_t *data) {
  return array_tostring(data_as_array(data));
}

unsigned int _list_hash(datalist_t *data) {
  return array_hash(data_as_array(data));
}

int _list_len(datalist_t *self) {
  return array_size(data_as_array(self));
}

data_t * _list_resolve(datalist_t *self, char *name) {
  long     ix;

  if (!strtoint(name, &ix)) {
    return datalist_get(self, ix);
  } else {
    return NULL;
  }
}

datalist_iter_t * _list_iter(datalist_t *data) {
  return (datalist_iter_t *) data_create(ListIterator, data_as_array(data));
}

char * _list_encode(pointer_t *data) {
  array_t *array = data_as_array(data);
  list_t  *encoded = str_list_create();
  int      ix;
  int      bufsz = 4; /* for '[ ' and ' ]' */
  char    *entry;
  char    *ret;
  char    *ptr;

  for (ix = 0; ix < array_size(array); ix++) {
    list_push(encoded, (entry = data_encode(data_array_get(array, ix))));
    bufsz += strlen(entry);
    if (ix > 0) {
      bufsz += 2; /* for ', ' */
    }
  }
  ret = ptr = stralloc(bufsz);
  strcpy(ptr, "[ ");
  ptr += 2;
  ix = 0;
  while (list_notempty(encoded)) {
    if (ix++ > 0) {
      strcpy(ptr, ", ");
      ptr += 2;
    }
    entry = (char *) list_shift(encoded);
    strcpy(ptr, entry);
    ptr += strlen(entry);
    free(entry);
  }
  strcpy(ptr, " ]");
  list_free(encoded);
  return ret;
}

datalist_t * _list_serialize(datalist_t *list) {
  datalist_t *ret = datalist_create(NULL);

  for (int ix = 0; ix < datalist_size(list); ix++) {
    datalist_push(ret, data_serialize(datalist_get(list, ix)));
  }
  return ret;
}

datalist_t * _list_deserialize(datalist_t *list) {
  datalist_t   *ret = datalist_create(NULL);
  data_t       *elem;

  for (int ix = 0; ix < datalist_size(list); ix++) {
    elem = datalist_get(list, ix);
    datalist_push(ret, data_deserialize(elem));
    data_free(elem);
  }
  return ret;
}

void * _list_reduce_children(datalist_t *list, reduce_t action, void *ctx) {
  data_t *elem;

  for (int ix = 0; ix < datalist_size(list); ix++) {
    elem = datalist_get(list, ix);
    action(elem, ctx);
  }
}

/* ----------------------------------------------------------------------- */

datalist_t * datalist_create(array_t *array) {
  datalist_t  *ret;

  ret = (datalist_t *) data_create(List, 0);
  if (ret && array) {
    array_reduce(array, (reduce_t) data_add_all_reducer, data_as_array(ret));
  }
  return ret;
}

array_t * datalist_to_array(datalist_t *list) {
  array_t *src = data_as_array(list);
  array_t *dest;

  dest = data_array_create(array_size(src));
  array_reduce(src, (reduce_t) data_add_all_reducer, dest);
  return dest;
}

array_t * datalist_to_str_array(datalist_t *list) {
  array_t *dest;

  dest = str_array_create(array_size(data_as_array(list)));
  array_reduce(data_as_array(list), (reduce_t) data_add_strings_reducer, dest);
  return dest;
}

datalist_t * str_array_to_datalist(array_t *src) {
  datalist_t  *ret;

  ret = (datalist_t *) data_create(List, 0);
  array_reduce(src, (reduce_t) data_add_all_as_data_reducer, data_as_array(ret));
  return ret;
}

datalist_t * _datalist_set(datalist_t *list, int ix, data_t *value) {
  array_set(data_as_array(list), ix, value);
  return list;
}

datalist_t * _datalist_push(datalist_t *list, data_t *value) {
  array_push(data_as_array(list), value);
  return list;
}

data_t * datalist_pop(datalist_t *list) {
  return (data_t *) array_pop(data_as_array(list));
}

void * datalist_reduce(datalist_t *list, reduce_t reducer, void *ctx) {
  return array_reduce(data_as_array(list), reducer, ctx);
}

/* ----------------------------------------------------------------------- */

datalist_t * _list_create(data_t _unused_ *self, char _unused_ *name, arguments_t *args) {
  datalist_t *ret = (datalist_t *) data_create(List, 0);

  if (args) {
    array_reduce(data_as_array(args -> args), (reduce_t) data_add_all_reducer, data_as_array(ret));
  }
  return ret;
}

data_t * _list_at(datalist_t *self, char _unused_ *name, arguments_t *args) {
  return _list_resolve(self, data_tostring(arguments_get_arg(args, 0)));
}

datalist_t * _list_slice(datalist_t *self, char _unused_ *name, arguments_t *args) {
  array_t *slice = array_slice(data_as_array(self),
                               data_intval(arguments_get_arg(args, 0)),
                               data_intval(arguments_get_arg(args, 1)));
  datalist_t  *ret = datalist_create(slice);

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
  return data_array_get(iter -> array, iter -> ix++);
}

data_t * _datalist_iter_has_next(datalist_iter_t *iter) {
  return int_as_bool((iter -> ix >= 0) &&
                     (iter -> ix < array_size(iter -> array)));
}
