/*
 * /obelix/src/lib/range.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
  data_t  _d;
  data_t *from;
  data_t *to;
  data_t *next;
  int     direction;
} range_t;

static void          _range_init(void) __attribute__((constructor));
static data_t *      _range_new(data_t *, va_list);
static void          _range_free(range_t *);
static char *        _range_tostring(range_t *);

static data_t *      _range_create(data_t *, char *, array_t *, dict_t *);

extern data_t *      range_create(data_t *from, data_t *to);
extern int           range_cmp(range_t *, range_t *);
extern unsigned int  range_hash(range_t *);
extern data_t *      range_iter(range_t *);
extern data_t *      range_next(range_t *);
extern data_t *      range_has_next(range_t *);

#define data_is_range(d)  ((d) && (data_hastype((d), Name)))
#define data_as_range(d)  ((range_t *) (data_is_range((d)) ? (d) : NULL))
#define range_free(n)     (data_free((data_t *) (n)))
#define range_tostring(n) (data_tostring((data_t *) (n)))
#define range_copy(n)     ((range_t *) data_copy((data_t *) (n)))

static vtable_t _vtable_range[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) range_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _range_free },
  { .id = FunctionToString, .fnc = (void_t) _range_tostring },
  { .id = FunctionHash,     .fnc = (void_t) range_hash },
  { .id = FunctionIter,     .fnc = (void_t) range_iter },
  { .id = FunctionNext,     .fnc = (void_t) range_next },
  { .id = FunctionHasNext,  .fnc = (void_t) range_has_next },
  { .id = FunctionNone,     .fnc = NULL }
};

/* FIXME Add append, delete, head, tail, etc... */
static methoddescr_t _methoddescr_range[] = {
  { .type = Any,           .name = "range", .method = _range_create, .argtypes = { Incrementable, Incrementable, Any }, .minargs = 2, .varargs = 0 },
  { .type = Incrementable, .name = "~",     .method = _range_create, .argtypes = { Incrementable, Any, Any },           .minargs = 1, .varargs = 0 },
  { .type = NoType,        .name = NULL,    .method = NULL,          .argtypes = { NoType, NoType, NoType },            .minargs = 0, .varargs = 0 },
};

int Range = -1;

/* ----------------------------------------------------------------------- */

void _range_init(void) {
  Range = typedescr_create_and_register(Range, "range", _vtable_range, _methoddescr_range);
}

void _range_free(range_t *range) {
  if (range) {
    data_free(range -> from);
    data_free(range -> to);
    data_free(range -> next);
    free(range);
  }
}

char * _range_tostring(range_t *r) {
  if (!r -> _d.str) {
    asprintf(&r -> _d.str, "%s ~ %s", data_tostring(r -> from), data_tostring(r -> to));
  }
  return NULL;
}

/* -- R A N G E _ T  P U B L I C  F U N C T I O N S ----------------------- */

data_t * range_create(data_t *from, data_t *to) {
  range_t     *range;
  typedescr_t *type;

  if (data_type(from) != data_type(to)) {
    return data_exception(ErrorType,
                          "Cannot build range: atoms '%s' and '%s' are of different type",
                          data_tostring(from), data_tostring(to));
  }
  type = data_typedescr(from);
  if (!typedescr_get_function(type, FunctionIncr) ||
      !typedescr_get_function(type, FunctionDecr)) {
    return data_exception(ErrorType,
                          "Cannot build range: type '%s' is not incrementable",
                          typedescr_tostring(type));
  }
  range = data_new(Range, range_t);
  range -> from = data_copy(from);
  range -> to = data_copy(to);
  range -> direction = (data_cmp(from, to) <= 0) ? FunctionIncr : FunctionDecr;
  range -> next = NULL;
  return (data_t *) range;
}

int range_cmp(range_t *r1, range_t *r2) {
  int      cmp;

  cmp = data_cmp(r1 -> from, r2 -> from);
  return (cmp) ? cmp : data_cmp(r1 -> to, r2 -> to);
}

unsigned int range_hash(range_t *r) {
  return hashblend(data_hash(r -> from), data_hash(r -> to));
}

data_t * range_iter(range_t *r) {
  r -> next = data_copy(r -> from);
  return data_copy((data_t *) r);
}

data_t * range_next(range_t *r) {
  data_t      *ret = r -> next;
  typedescr_t *type;
  data_fnc_t   fnc;

  type = data_typedescr(ret);
  fnc = (data_fnc_t) typedescr_get_function(type, r -> direction);
  assert(fnc);
  r -> next = fnc(ret);
  return ret;
}

data_t * range_has_next(range_t *r) {
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
  int     infix = !strcmp(name, "~");

  from = (infix) ? self : data_array_get(args, 0);
  to = data_array_get(args, (infix) ? 0 : 1);
  return data_create(Range, from, to);
}
