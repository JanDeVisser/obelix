/*
 * /obelix/src/data/data_int.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <error.h>

static void          _list_init(void) __attribute__((constructor));
static data_t *      _list_new(data_t *, va_list);
static data_t *      _list_copy(data_t *, data_t *);
static int           _list_cmp(data_t *, data_t *);
static char *        _list_tostring(data_t *);
static data_t *      _list_cast(data_t *, int);
static unsigned int  _list_hash(data_t *);

static data_t *      _list_create(data_t *, char *, array_t *, dict_t *);
static data_t *      _list_len(data_t *, char *, array_t *, dict_t *);
static data_t *      _list_at(data_t *, char *, array_t *, dict_t *);
static data_t *      _list_slice(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_list[] = {
  { .id = MethodNew,      .fnc = (void_t) _list_new },
  { .id = MethodCopy,     .fnc = (void_t) _list_copy },
  { .id = MethodCmp,      .fnc = (void_t) _list_cmp },
  { .id = MethodFree,     .fnc = (void_t) array_free },
  { .id = MethodToString, .fnc = (void_t) _list_tostring },
  { .id = MethodParse,    .fnc = NULL }, /* FIXME */
  { .id = MethodCast,     .fnc = (void_t) _list_cast },
  { .id = MethodHash,     .fnc = (void_t) _list_hash },
  { .id = MethodNone,     .fnc = NULL }
};

static typedescr_t _typedescr_list =   {
  .type      = List,
  .typecode  = "L",
  .type_name = "list",
  .vtable    = _vtable_list,
};

/* FIXME Add append, delete, head, tail, etc... */
static methoddescr_t _methoddescr_list[] = {
  { .type = Any,    .name = "list",  .method = _list_create,.argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = List,   .name = "len",   .method = _list_len,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = List,   .name = "at",    .method = _list_at,    .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = List,   .name = "slice", .method = _list_slice, .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,    .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

/*
 * --------------------------------------------------------------------------
 * List datatype functions
 * --------------------------------------------------------------------------
 */

void _list_init(void) {
  typedescr_register(_typedescr_list);
  typedescr_register_methods(_methoddescr_list);
}

data_t * _list_new(data_t *target, va_list arg) {
  array_t *array;
  int      count;
  int      ix;
  data_t  *elem;

  count = va_arg(arg, int);
  array = data_array_create((count > 0) ? count : 4);
  target -> ptrval = array;
  for (ix = 0; ix < count; ix++) {
    elem = va_arg(arg, data_t *);
    array_push(array, elem);
  }
  return target;
}

data_t * _list_cast(data_t *src, int totype) {
  array_t *array = data_arrayval(src);
  data_t  *ret = NULL;

  switch (totype) {
    case Bool:
      ret = data_create(Bool, array && array_size(array));
      break;
  }
  return ret;
}

int _list_cmp(data_t *d1, data_t *d2) {
  array_t *a1;
  array_t *a2;
  data_t  *e1;
  data_t  *e2;
  int      ix;
  int      cmp = 0;

  a1 = d1 -> ptrval;
  a2 = d2 -> ptrval;
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
  array_t *dest_arr;
  array_t *src_arr;
  data_t  *data;
  int      ix;

  dest_arr = dest -> ptrval;
  src_arr = src -> ptrval;
  for (ix = 0; ix < array_size(src_arr); ix++) {
    data = data_array_get(src_arr, ix);
    array_push(dest_arr, data_copy(data));
  }
  return dest;
}

char * _list_tostring(data_t *data) {
  array_t *array;
  str_t   *s;

  array = data -> ptrval;
  s = array_tostr(array);
  data -> str = strdup(str_chars(s));
  str_free(s);
  return NULL;
}

unsigned int _list_hash(data_t *data) {
  return array_hash(data -> ptrval);
}

/* ----------------------------------------------------------------------- */

data_t * data_create_list(array_t *array) {
  data_t  *ret;

  ret = data_create(List, 0);
  array_reduce(array, (reduce_t) data_add_all_reducer, data_arrayval(ret));
  return ret;
}

array_t * data_list_copy(data_t *list) {
  array_t *dest;
  
  dest = data_array_create(array_size(data_arrayval(list)));
  array_reduce(data_arrayval(list), (reduce_t) data_add_all_reducer, dest);
  return dest;
}

array_t * data_list_to_str_array(data_t *list) {
  array_t *dest;
  
  dest = str_array_create(array_size(data_arrayval(list)));
  array_reduce(data_arrayval(list), (reduce_t) data_add_strings_reducer, dest);
  return dest;
}

/* ----------------------------------------------------------------------- */

data_t * _list_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *ret = data_create(List, 0);
  
  if (self) {
    array_push(data_arrayval(ret), self);
  }
  if (args) {
    array_reduce(args, (reduce_t) data_add_all_reducer, data_arrayval(ret));
  }
  return ret;
}

data_t * _list_len(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, array_size((array_t *) self -> ptrval));
}

data_t * _list_at(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  /* TODO */
  return data_create(Int, list_size((list_t *) self -> ptrval));
}

data_t * _list_slice(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  /* TODO */
  return data_create(Int, list_size((list_t *) self -> ptrval));
}

 
