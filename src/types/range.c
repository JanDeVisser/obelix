/*
 * /obelix/src/types/range.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
#include <string.h>

#include <data.h>
#include <exception.h>

/* ----------------------------------------------------------------------- */

typedef struct _range {
  data_t *from;
  data_t *to;
  data_t *next;
  int     direction;
} range_t;

static void          _range_init(void) __attribute__((constructor));
static data_t *      _range_new(data_t *, va_list);
static data_t *      _range_copy(data_t *, data_t *);
static void          _range_free(range_t *);
static int           _range_cmp(data_t *, data_t *);
static char *        _range_tostring(data_t *);
static data_t *      _range_cast(data_t *, int);
static unsigned int  _range_hash(data_t *);
static data_t *      _range_iter(data_t *);
static data_t *      _range_next(data_t *);
static data_t *      _range_has_next(data_t *);

static data_t *      _range_create(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_range[] = {
  { .id = FunctionNew,      .fnc = (void_t) _range_new },
  { .id = FunctionCopy,     .fnc = (void_t) _range_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _range_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _range_free },
  { .id = FunctionToString, .fnc = (void_t) _range_tostring },
  { .id = FunctionHash,     .fnc = (void_t) _range_hash },
  { .id = FunctionIter,     .fnc = (void_t) _range_iter },
  { .id = FunctionNext,     .fnc = (void_t) _range_next },
  { .id = FunctionHasNext,  .fnc = (void_t) _range_has_next },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_range =   {
  .type      = Range,
  .type_name = "range",
  .vtable    = _vtable_range
};

/* FIXME Add append, delete, head, tail, etc... */
static methoddescr_t _methoddescr_range[] = {
  { .type = Any,    .name = "range", .method = _range_create, .argtypes = { Any, Any, Any },        .minargs = 2, .varargs = 0 },
  { .type = Any,    .name = "~",     .method = _range_create, .argtypes = { Any, Any, Any },        .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,    .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

/* ----------------------------------------------------------------------- */

void _range_init(void) {
  typedescr_register(&_typedescr_range);
  typedescr_register_methods(_methoddescr_range);
}

data_t * _range_new(data_t *target, va_list args) {
  range_t     *range;
  data_t      *from;
  data_t      *to;
  typedescr_t *type;

  from = va_arg(args, data_t *);
  to = va_arg(args, data_t *);
  if (data_type(from) != data_type(to)) {
    return data_error(ErrorType, 
                      "Cannot build range: atoms '%s' and '%s' are of different type",
                      data_tostring(from), data_tostring(to));
  }
  type = data_typedescr(from);
  if (!typedescr_get_function(type, FunctionIncr) || 
      !typedescr_get_function(type, FunctionDecr)) {
    return data_error(ErrorType,
                      "Cannot build range: type '%s' is not incrementable",
                      typedescr_tostring(type));
  }
  range = NEW(range_t);
  range -> from = data_copy(from);
  range -> to = data_copy(to);
  range -> direction = (data_cmp(from, to) <= 0) ? FunctionIncr : FunctionDecr;
  range -> next = NULL;
  target -> ptrval = range;
  return target;
}

void _range_free(range_t *range) {
  if (range) {
    data_free(range -> from);
    data_free(range -> to);
    data_free(range -> next);
    free(range);
  }
}

int _range_cmp(data_t *d1, data_t *d2) {
  range_t *r1 = (range_t *) d1 -> ptrval;
  range_t *r2 = (range_t *) d2 -> ptrval;
  int      cmp;
  
  cmp = data_cmp(r1 -> from, r2 -> from);
  return (cmp) ? cmp : data_cmp(r1 -> to, r2 -> to);
}

data_t * _range_copy(data_t *dest, data_t *src) {
  range_t *dest_range;
  range_t *src_range = (range_t *) src -> ptrval;
  
  dest_range = NEW(range_t);
  dest_range -> from = data_copy(src_range -> from);
  dest_range -> to = data_copy(src_range -> to);
  dest_range -> next = NULL;
  dest_range -> direction = src_range -> direction;
  dest -> ptrval = dest_range;
  return dest;
}

char * _range_tostring(data_t *data) {
  range_t *r = (range_t *) data -> ptrval;
 
  asprintf(&data -> str, "%s ~ %s", data_tostring(r -> from), data_tostring(r -> to));
  return NULL;
}

unsigned int _range_hash(data_t *data) {
  range_t *r = (range_t *) data -> ptrval;
 
  return hashblend(data_hash(r -> from), data_hash(r -> to));
}

data_t * _range_iter(data_t *data) {
  range_t *r = (range_t *) data -> ptrval;
  
  r -> next = data_copy(r -> from);
  return data_copy(data);
}

data_t * _range_next(data_t *data) {
  range_t     *r = (range_t *) data -> ptrval;
  data_t      *ret = r -> next;
  typedescr_t *type;
  data_fnc_t   fnc;
  
  type = data_typedescr(ret);
  fnc = (data_fnc_t) typedescr_get_function(type, r -> direction);
  assert(fnc);
  r -> next = fnc(ret);
  return ret;
}

data_t * _range_has_next(data_t *data) {
  range_t *r = (range_t *) data -> ptrval;
  int      ret;
  int      cmp;
  
  ret = r -> next != NULL;
  if (ret) {
    cmp = data_cmp(r -> next, r -> to);
    ret = (r -> direction == FunctionIncr) ? (cmp < 0) : (cmp > 0);
  }
  return data_create(Bool, ret);
}

/* ----------------------------------------------------------------------- */

data_t * _range_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *from;
  data_t *to;
  int     ix;
  
  from = (!strcmp(name, "~")) ? self : data_array_get(args, 0);
  to = data_array_get(args, (!strcmp(name, "~")) ? 0 : 1);
  return data_create(Range, from, to);
}

