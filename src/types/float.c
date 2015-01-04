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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <error.h>

static data_t *      _float_new(data_t *, va_list);
static int           _float_cmp(data_t *, data_t *);
static char *        _float_tostring(data_t *);
static unsigned int  _float_hash(data_t *);
static data_t *      _float_parse(char *);
static data_t *      _float_cast(data_t *, int);

static data_t *      _float_add(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_mult(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_div(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_abs(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_pow(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_sin(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_cos(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_tan(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_sqrt(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_minmax(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_round(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_trunc(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_floor(data_t *, char *, array_t *, dict_t *);
static data_t *      _float_ceil(data_t *, char *, array_t *, dict_t *);

typedescr_t typedescr_float =   {
  type:                  Float,
  typecode:              "F",
  typename:              "float",
  new:      (new_t)      _float_new,
  copy:                  NULL,
  cmp:      (cmp_t)      _float_cmp,
  free:                  NULL,
  tostring: (tostring_t) _float_tostring,
  parse:    (parse_t)    _float_parse,
  cast:                  _float_cast,
  hash:     (hash_t)     _float_hash,
  promote_to:            NoType
};

methoddescr_t methoddescr_float[] = {
  { type: Float,  name: "+",     method: _float_add,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "-",     method: _float_add,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "sum",   method: _float_add,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "*",     method: _float_mult,   argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "mult",  method: _float_mult,   argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "/",     method: _float_div,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Float,  name: "div",   method: _float_div,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Float,  name: "abs",   method: _float_abs,    argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "^",     method: _float_pow,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Float,  name: "pow",   method: _float_pow,    argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Float,  name: "sin",   method: _float_sin,    argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "cos",   method: _float_cos,    argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "tan",   method: _float_tan,    argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "sqrt",  method: _float_sqrt,   argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "min",   method: _float_minmax, argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "max",   method: _float_minmax, argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Float,  name: "round", method: _float_round,  argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "trunc", method: _float_trunc,  argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "floor", method: _float_floor,  argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: Float,  name: "ceil",  method: _float_ceil,   argtypes: { NoType,  NoType, NoType }, minargs: 0, varargs: 0 },
  { type: NoType, name: NULL,    method: NULL,          argtypes: { NoType, NoType, NoType }, minargs: 0, varargs: 0 }
};

/*
 * --------------------------------------------------------------------------
 * Float datatype functions
 * --------------------------------------------------------------------------
 */


data_t * _float_new(data_t *target, va_list arg) {
  double val;

  val = va_arg(arg, double);
  target -> dblval = val;
  return target;
}

unsigned int _float_hash(data_t *data) {
  return hash(&(data -> dblval), sizeof(double));
}

int _float_cmp(data_t *self, data_t *d2) {
  return (self -> dblval == d2 -> dblval)
      ? 0
      : (self -> dblval > d2 -> dblval) ? 1 : -1;
}

char * _float_tostring(data_t *data) {
  return dtoa(data -> dblval);
}

data_t * _float_parse(char *str) {
  char   *endptr;
  double  val;

  val = strtod(str, &endptr);
  return ((*endptr == 0) || (isspace(*endptr)))
    ? data_create(Float, val)
    : NULL;
}

data_t * _float_cast(data_t *data, int totype) {
  return (totype == Int) ? data_create(Int, (long) data -> dblval) : NULL;
}

/* ----------------------------------------------------------------------- */

data_t * _float_add(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  double  retval = data_dblval(self);
  double  val;
  int     plus = (name[0] == '+');

  retval = self -> dblval;
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    val = data_dblval(d);
    if (!plus) {
      val = -val;
    }
    retval += val;
  }
  return data_create(Float, retval);
}

data_t * _float_mult(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  double  retval = data_dblval(self);
  
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    retval *= data_dblval(d);
  }
  return data_create(Float, retval);
}

data_t * _float_div(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  return data_create(Float, data_dblval(self) / data_dblval(denom));
}

data_t * _float_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, fabs(data_dblval(self)));
}

data_t * _float_round(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) round(data_dblval(self)));
}

data_t * _float_trunc(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) trunc(data_dblval(self)));
}

data_t * _float_floor(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) floor(data_dblval(self)));
}

data_t * _float_ceil(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) ceil(data_dblval(self)));
}

/*
 * The functions below can be called with Int as self by _int_fallback
 */

data_t * _float_pow(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *exp;

  exp = (data_t *) array_get(args, 0);
  return data_create(Float, pow(data_dblval(self), data_dblval(exp)));
}

data_t * _float_sin(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, sin(data_dblval(self)));
}

data_t * _float_cos(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, cos(data_dblval(self)));
}

data_t * _float_tan(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, tan(data_dblval(self)));
}

data_t * _float_sqrt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, sqrt(data_dblval(self)));
}

data_t * _float_minmax(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     maxmin = name && !strcmp(name, "max");
  data_t *ret = self;
  data_t *d;
  int     ix;

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    ret = (maxmin)
      ? ((data_dblval(ret) > data_dblval(d)) ? ret : d)
      : ((data_dblval(ret) < data_dblval(d)) ? ret : d);
  }
  return data_copy(ret);
}

