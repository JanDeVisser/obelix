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

#include "libcore.h"
#include <data.h>
#include <exception.h>
#include <range.h>

/* ----------------------------------------------------------------------- */

static inline void _range_init(void);
static data_t *    _range_new(range_t *, va_list);
static void *      _range_reduce_children(range_t *, reduce_t, void *);

static vtable_t _vtable_Range[] = {
  { .id = FunctionNew,         .fnc = (void_t) _range_new },
  { .id = FunctionCmp,         .fnc = (void_t) range_cmp },
  { .id = FunctionHash,        .fnc = (void_t) range_hash },
  { .id = FunctionIter,        .fnc = (void_t) range_iter },
  { .id = FunctionNext,        .fnc = (void_t) range_next },
  { .id = FunctionHasNext,     .fnc = (void_t) range_has_next },
  { .id = FunctionReduce,      .fnc = (void_t) _range_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

int Range = -1;

/* ----------------------------------------------------------------------- */

void _range_init(void) {
  if (Range < 1) {
    typedescr_register(Range, range_t);
  }
}

data_t * _range_new(range_t *range, va_list args) {
  data_t      *from = va_arg(args, data_t *);
  data_t      *to = va_arg(args, data_t *);
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
  range -> from = from;
  range -> to = to;
  range -> direction = (data_cmp(from, to) <= 0) ? FunctionIncr : FunctionDecr;
  range -> next = NULL;
  asprintf(&range->_d.str, "%s ~ %s", data_tostring(from), data_tostring(to));
  data_set_string_semantics(range, StrSemanticsStatic);
  return (data_t *) range;
}

void * _range_reduce_children(range_t *range, reduce_t reducer, void *ctx) {
  ctx = reducer(range->from, ctx);
  ctx = reducer(range->to, ctx);
  return reducer(range->next, ctx);
}


/* -- R A N G E _ T  P U B L I C  F U N C T I O N S ----------------------- */

data_t * range_create(data_t *from, data_t *to) {
  _range_init();
  return (data_t *) data_create(Range, from, to);
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
  r -> next = r -> from;
  return data_as_data(r);
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
  return int_as_bool(ret);
}

/* ----------------------------------------------------------------------- */
