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
  hash:     (hash_t)     _float_hash
};

methoddescr_t methoddescr_float[] = {
  { type: Float, name: "+",     method: _float_add,    min_args: 2, max_args: -1 },
  { type: Float, name: "-",     method: _float_add,    min_args: 2, max_args: -1 },
  { type: Float, name: "sum",   method: _float_add,    min_args: 2, max_args: -1 },
  { type: Float, name: "*",     method: _float_mult,   min_args: 2, max_args: -1 },
  { type: Float, name: "mult",  method: _float_mult,   min_args: 2, max_args: -1 },
  { type: Float, name: "/",     method: _float_div,    min_args: 2, max_args: 2  },
  { type: Float, name: "div",   method: _float_div,    min_args: 2, max_args: 2  },
  { type: Float, name: "abs",   method: _float_abs,    min_args: 1, max_args: 1  },
  { type: Float, name: "^",     method: _float_pow,    min_args: 2, max_args: 2  },
  { type: Float, name: "pow",   method: _float_pow,    min_args: 2, max_args: 2  },
  { type: Float, name: "sin",   method: _float_sin,    min_args: 1, max_args: 1  },
  { type: Float, name: "cos",   method: _float_cos,    min_args: 1, max_args: 1  },
  { type: Float, name: "tan",   method: _float_tan,    min_args: 1, max_args: 1  },
  { type: Float, name: "sqrt",  method: _float_sqrt,   min_args: 1, max_args: 1  },
  { type: Float, name: "min",   method: _float_minmax, min_args: 1, max_args: -1 },
  { type: Float, name: "max",   method: _float_minmax, min_args: 1, max_args: -1 },
  { type: Float, name: "round", method: _float_round,  min_args: 1, max_args: 1  },
  { type: Float, name: "trunc", method: _float_trunc,  min_args: 1, max_args: 1  },
  { type: Float, name: "floor", method: _float_floor,  min_args: 1, max_args: 1  },
  { type: Float, name: "ceil",  method: _float_ceil,   min_args: 1, max_args: 1  },
  { type: -1,    name: NULL,    method: NULL,          min_args: 0, max_args: 0  },
};

/*
 * --------------------------------------------------------------------------
 * Float datatype functions
 * --------------------------------------------------------------------------
 */

#define DBLVAL(d)  ((data_type((d)) == Float) ? (double)((d) -> intval) : (d) -> dblval)
#define INTVAL(d)  ((data_type((d)) == Int) ? (d) -> intval : (long)((d) -> dblval))

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

data_t * _float_add(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     ix;
  int     type = Float;
  double  retval = 0.0;
  double  val;
  int     plus = (name[0] == '+');

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (!data_is_numeric(d)) {
      type = data_type(d);
      break;
    }
  }
  if (type != Float) {
    ret = data_error(ErrorType,
                     ((plus)
                       ? "Cannot add floats and objects of type '%s'"
                       : "Cannot subtract floats and objects of type '%s'"),
                     typedescr_get(type) -> typename);
  } else {
    retval = self -> dblval;
    for (ix = 0; ix < array_size(args); ix++) {
      d = (data_t *) array_get(args, ix);
      val = DBLVAL(d);
      retval += (plus) ? val : -val;
    }
    ret = data_create(Float, retval);
  }
  return ret;
}

data_t * _float_mult(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     ix;
  int     type = Float;
  double  retval = 0.0;

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (!data_is_numeric(d)) {
      type = data_type(d);
      break;
    }
  }
  if (type != Float) {
    ret = data_error(ErrorType,
                     "Cannot multiply floats and objects of type '%s'",
                     typedescr_get(type) -> typename);
  } else {
    retval = self -> dblval;
    for (ix = 0; ix < array_size(args); ix++) {
      d = (data_t *) array_get(args, ix);
      retval *= DBLVAL(d);
    }
    ret = data_create(type, retval);
  }
  return ret;
}

data_t * _float_div(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;
  data_t *ret;
  double  retval = 0.0;

  denom = (data_t *) array_get(args, 0);
  if (!data_is_numeric(denom)) {
    ret = data_error(ErrorType,
                     "Cannot divide an float by an object of type '%s'",
                     typedescr_get(data_type(denom)) -> typename);
  }
  retval = DBLVAL(self) / DBLVAL(denom);
  ret = data_create(Float, retval);
  return ret;
}

data_t * _float_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, fabs(DBLVAL(self)));
}

/*
 * The functions below can be called with Int as self by _int_fallback
 */

data_t * _float_pow(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *exp;

  if (!args || (array_size(args) != 1)) {
    return data_error(ErrorArgCount, "pow(x,y) requires exactly two arguments");
  }
  exp = (data_t *) array_get(args, 0);
  if (!data_is_numeric(exp)) {
    return data_error(ErrorType,
                     "Cannot a number to the power of an object of type '%s'",
                     typedescr_get(data_type(exp)) -> typename);
  } else {
    return data_create(Float, pow(DBLVAL(self), DBLVAL(exp)));
  }
}

data_t * _float_sin(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, sin(DBLVAL(self)));
}

data_t * _float_cos(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, cos(DBLVAL(self)));
}

data_t * _float_tan(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, tan(DBLVAL(self)));
}

data_t * _float_sqrt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, sqrt(DBLVAL(self)));
}

data_t * _float_minmax(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     maxmin = name && !strcmp(name, "max");
  data_t *ret = self;
  data_t *d;
  int     ix;

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (!data_is_numeric(d)) {
      return data_error(ErrorType,
                       "Object of type '%s' cannot be included in %s() operations",
                       typedescr_get(data_type(d)) -> typename, name);
    }
    ret = (maxmin)
      ? ((DBLVAL(ret) > DBLVAL(d)) ? ret : d)
      : ((DBLVAL(ret) < DBLVAL(d)) ? ret : d);
  }
  return data_copy(ret);
}

data_t * _float_round(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, round(DBLVAL(self)));
}

data_t * _float_trunc(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, round(DBLVAL(self)));
}

data_t * _float_floor(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, floor(DBLVAL(self)));
}

data_t * _float_ceil(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, ceil(DBLVAL(self)));
}

