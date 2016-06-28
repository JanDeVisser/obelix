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
#include <range.h>

/* ----------------------------------------------------------------------- */

static inline void   _range_init(void);
static data_t *      _range_new(data_t *, va_list);
static void          _range_free(range_t *);
static char *        _range_allocstring(range_t *);

static vtable_t _vtable_range[] = {
  { .id = FunctionCmp,         .fnc = (void_t) range_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _range_free },
  { .id = FunctionAllocString, .fnc = (void_t) _range_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) range_hash },
  { .id = FunctionIter,        .fnc = (void_t) range_iter },
  { .id = FunctionNext,        .fnc = (void_t) range_next },
  { .id = FunctionHasNext,     .fnc = (void_t) range_has_next },
  { .id = FunctionNone,        .fnc = NULL }
};

int Range = -1;

/* ----------------------------------------------------------------------- */

void _range_init(void) {
  if (Range < 0) {
    Range = typedescr_create_and_register(Range, "range", _vtable_range, /* _methoddescr_range */ NULL);
  }
}

void _range_free(range_t *range) {
  if (range) {
    data_free(range -> from);
    data_free(range -> to);
    data_free(range -> next);
  }
}

char * _range_allocstring(range_t *r) {
  char *buf;
  
  asprintf(&buf, "%s ~ %s", data_tostring(r -> from), data_tostring(r -> to));
  return buf;
}

/* -- R A N G E _ T  P U B L I C  F U N C T I O N S ----------------------- */

data_t * range_create(data_t *from, data_t *to) {
  range_t     *range;
  typedescr_t *type;

  _range_init();
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
  return int_as_bool(ret);
}

/* ----------------------------------------------------------------------- */

