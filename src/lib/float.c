/*
 * /obelix/src/types/float.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcore.h"
#include <data.h>
#include <exception.h>

extern  void          float_init(void);

static data_t *      _float_new(int, va_list);
static int           _float_cmp(flt_t *, flt_t *);
static char *        _float_allocstring(flt_t *);
static unsigned int  _float_hash(flt_t *);
static data_t *      _float_parse(char *);
static data_t *      _float_cast(flt_t *, int);
static double        _float_fltvalue(flt_t *);
static int           _float_intvalue(flt_t *);

static data_t *      _number_add(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_mult(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_div(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_abs(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_pow(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_sin(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_cos(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_tan(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_sqrt(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_minmax(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_round(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_trunc(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_floor(data_t *, char *, array_t *, dict_t *);
static data_t *      _number_ceil(data_t *, char *, array_t *, dict_t *);

static methoddescr_t _methoddescr_number[] = {
  { .type = Number,  .name = "+",     .method = _number_add,    .argtypes = { NoType, NoType, NoType },  .minargs = 0, .varargs = 1 },
  { .type = Number,  .name = "-",     .method = _number_add,    .argtypes = { NoType, NoType, NoType },  .minargs = 0, .varargs = 1 },
  { .type = Number,  .name = "sum",   .method = _number_add,    .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 1 },
  { .type = Number,  .name = "*",     .method = _number_mult,   .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 1 },
  { .type = Number,  .name = "mult",  .method = _number_mult,   .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 1 },
  { .type = Number,  .name = "/",     .method = _number_div,    .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 0 },
  { .type = Number,  .name = "div",   .method = _number_div,    .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 0 },
  { .type = Number,  .name = "abs",   .method = _number_abs,    .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "^",     .method = _number_pow,    .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 0 },
  { .type = Number,  .name = "pow",   .method = _number_pow,    .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 0 },
  { .type = Number,  .name = "sin",   .method = _number_sin,    .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "cos",   .method = _number_cos,    .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "tan",   .method = _number_tan,    .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "sqrt",  .method = _number_sqrt,   .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "min",   .method = _number_minmax, .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 1 },
  { .type = Number,  .name = "max",   .method = _number_minmax, .argtypes = { Number, NoType, NoType },  .minargs = 1, .varargs = 1 },
  { .type = Number,  .name = "round", .method = _number_round,  .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "trunc", .method = _number_trunc,  .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "floor", .method = _number_floor,  .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Number,  .name = "ceil",  .method = _number_ceil,   .argtypes = { NoType,  NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType,  .name = NULL,    .method = NULL,           .argtypes = { NoType, NoType, NoType },  .minargs = 0, .varargs = 0 }
};

static vtable_t _vtable_Float[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _float_new },
  { .id = FunctionCmp,         .fnc = (void_t) _float_cmp },
  { .id = FunctionAllocString, .fnc = (void_t) _float_allocstring },
  { .id = FunctionParse,       .fnc = (void_t) _float_parse },
  { .id = FunctionCast,        .fnc = (void_t) _float_cast },
  { .id = FunctionHash,        .fnc = (void_t) _float_hash },
  { .id = FunctionFltValue,    .fnc = (void_t) _float_fltvalue },
  { .id = FunctionIntValue,    .fnc = (void_t) _float_intvalue },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t * _methods_Float = NULL;

/*
 * --------------------------------------------------------------------------
 * Float datatype functions
 * --------------------------------------------------------------------------
 */

void float_init(void) {
  builtin_interface_register(Number, 2, FunctionFltValue, FunctionIntValue);
  builtin_typedescr_register(Float, "float", flt_t);
  typedescr_register_methods(Number, _methoddescr_number);
}

data_t * _float_new(int type, va_list arg) {
  return (data_t *) flt_create(va_arg(arg, double));
}

unsigned int _float_hash(flt_t *data) {
  return hash(&(data -> dbl), sizeof(double));
}

int _float_cmp(flt_t *self, flt_t *d2) {
  return (self -> dbl == d2 -> dbl)
      ? 0
      : (self -> dbl > d2 -> dbl) ? 1 : -1;
}

char * _float_allocstring(flt_t *data) {
  char *buf;

  asprintf(&buf, "%f", data -> dbl);
  return buf;
}

data_t * _float_parse(char *str) {
  char   *endptr;
  double  val;

  val = strtod(str, &endptr);
  return ((*endptr == 0) || (isspace(*endptr)))
    ? flt_to_data(val)
    : NULL;
}

data_t * _float_cast(flt_t *data, int totype) {
  data_t *ret = NULL;

  switch (totype) {
    case Int:
      ret = (data_t *) int_create((long) data -> dbl);
      break;
    case Bool:
      ret = int_as_bool(data -> dbl != 0);
      break;
  }
  return ret;
}

double _float_fltvalue(flt_t *data) {
  return data -> dbl;
}

int _float_intvalue(flt_t *data) {
  return (int) data -> dbl;
}

/* ----------------------------------------------------------------------- */

data_t * _number_add(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  double  retval;
  double  val;
  int     plus = (name[0] == '+') || !strcmp(name, "sum");

  if (!args || !array_size(args)) {
    return (plus) ? data_copy(self) : flt_to_data(-1.0 * data_floatval(self));
  }
  retval = ((flt_t *) self) -> dbl;
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    val = data_floatval(d);
    if (!plus) {
      val = -val;
    }
    retval += val;
  }
  return flt_to_data(retval);
}

data_t * _number_mult(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  double  retval = data_floatval(self);

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    retval *= data_floatval(d);
  }
  return flt_to_data(retval);
}

data_t * _number_div(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  return flt_to_data(data_floatval(self) / data_floatval(denom));
}

data_t * _number_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return flt_to_data(fabs(data_floatval(self)));
}

data_t * _number_round(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return int_to_data((long) round(data_floatval(self)));
}

data_t * _number_trunc(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return int_to_data((long) trunc(data_floatval(self)));
}

data_t * _number_floor(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return int_to_data((long) floor(data_floatval(self)));
}

data_t * _number_ceil(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return int_to_data((long) ceil(data_floatval(self)));
}

/*
 * The functions below can be called with Int as self by _int_fallback
 */

data_t * _number_pow(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *exp;

  exp = (data_t *) array_get(args, 0);
  return flt_to_data(pow(data_floatval(self), data_floatval(exp)));
}

data_t * _number_sin(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return flt_to_data(sin(data_floatval(self)));
}

data_t * _number_cos(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return flt_to_data(cos(data_floatval(self)));
}\

data_t * _number_tan(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return flt_to_data(tan(data_floatval(self)));
}

data_t * _number_sqrt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return flt_to_data(sqrt(data_floatval(self)));
}

data_t * _number_minmax(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     maxmin = name && !strcmp(name, "max");
  data_t *ret = self;
  data_t *d;
  int     ix;

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    ret = (maxmin)
      ? ((data_floatval(ret) > data_floatval(d)) ? ret : d)
      : ((data_floatval(ret) < data_floatval(d)) ? ret : d);
  }
  return data_copy(ret);
}

/* ----------------------------------------------------------------------- */

flt_t * flt_create(double val) {
  flt_t *ret = data_new(Float, flt_t);

  ret -> dbl = val;
  return ret;
}

/* ----------------------------------------------------------------------- */
