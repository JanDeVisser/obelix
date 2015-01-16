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


#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <error.h>

static data_t *      _int_new(data_t *, va_list);
static int           _int_cmp(data_t *, data_t *);
static char *        _int_tostring(data_t *);
static data_t *      _int_cast(data_t *, int);
static unsigned int  _int_hash(data_t *);
static data_t *      _int_parse(char *);
static data_t *      _int_fallback(data_t *, char *, array_t *, dict_t *);

static data_t *      _int_add(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_mult(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_div(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_mod(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_abs(data_t *, char *, array_t *, dict_t *);

static data_t *      _bool_new(data_t *, va_list);
static char *        _bool_tostring(data_t *);
static data_t *      _bool_parse(char *);
static data_t *      _bool_cast(data_t *, int);

static data_t *      _bool_and(data_t *, char *, array_t *, dict_t *);
static data_t *      _bool_or(data_t *, char *, array_t *, dict_t *);
static data_t *      _bool_not(data_t *, char *, array_t *, dict_t *);

typedescr_t typedescr_int = {
  type:                  Int,
  typecode:              "I",
  typename:              "int",
  new:      (new_t)      _int_new,
  copy:                  NULL,
  cmp:      (cmp_t)      _int_cmp,
  free:                  NULL,
  tostring: (tostring_t) _int_tostring,
  parse:    (parse_t)    _int_parse,
  cast:                  _int_cast,
  hash:     (hash_t)     _int_hash,
  promote_to:            Float,
  fallback:              _int_fallback
};

typedescr_t typedescr_bool = {
  type:                  Bool,
  typecode:              "B",
  typename:              "bool",
  new:      (new_t)      _bool_new,
  copy:                  NULL,
  cmp:      (cmp_t)      _int_cmp,
  free:                  NULL,
  tostring: (tostring_t) _bool_tostring,
  parse:    (parse_t)    _bool_parse,
  cast:                  _bool_cast,
  hash:     (hash_t)     _int_hash,
  promote_to:            Int
};

methoddescr_t methoddescr_int[] = {
  { type: Int,    name: "+",    method: _int_add,  argtypes: { Numeric, NoType, NoType }, minargs: 0, varargs: 1 },
  { type: Int,    name: "-",    method: _int_add,  argtypes: { Numeric, NoType, NoType }, minargs: 0, varargs: 1 },
  { type: Int,    name: "sum",  method: _int_add,  argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Int,    name: "*",    method: _int_mult, argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Int,    name: "mult", method: _int_mult, argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 1 },
  { type: Int,    name: "/",    method: _int_div,  argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Int,    name: "div",  method: _int_div,  argtypes: { Numeric, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Int,    name: "%",    method: _int_mod,  argtypes: { Int, NoType, NoType },     minargs: 1, varargs: 0 },
  { type: Int,    name: "mod",  method: _int_mod,  argtypes: { Int, NoType, NoType },     minargs: 1, varargs: 0 },
  { type: Int,    name: "abs",  method: _int_abs,  argtypes: { NoType, NoType, NoType },  minargs: 0, varargs: 0 },
  { type: NoType, name: NULL,   method: NULL,      argtypes: { NoType, NoType, NoType },  minargs: 0, varargs: 0 }
};

methoddescr_t methoddescr_bool[] = {
  { type: Bool,   name: "&&",  method: _bool_and,  argtypes: { Int, NoType, NoType },     minargs: 1, varargs: 1 },
  { type: Bool,   name: "and", method: _bool_and,  argtypes: { Int, NoType, NoType },     minargs: 1, varargs: 1 },
  { type: Bool,   name: "||",  method: _bool_or,   argtypes: { Int, NoType, NoType },     minargs: 1, varargs: 1 },
  { type: Bool,   name: "or",  method: _bool_or,   argtypes: { Int, NoType, NoType },     minargs: 1, varargs: 1 },
  { type: Bool,   name: "!",   method: _bool_not,  argtypes: { NoType, NoType, NoType },  minargs: 0, varargs: 0 },
  { type: Bool,   name: "not", method: _bool_not,  argtypes: { NoType, NoType, NoType },  minargs: 0, varargs: 0 },
  { type: NoType, name: NULL,  method: NULL,       argtypes: { NoType, NoType, NoType },  minargs: 0, varargs: 0 }
};

/*
 * --------------------------------------------------------------------------
 * Int datatype functions
 * --------------------------------------------------------------------------
 */

data_t * _int_new(data_t *target, va_list arg) {
  long val;

  val = va_arg(arg, long);
  target -> intval = val;
  return target;
}

unsigned int _int_hash(data_t *data) {
  return hash(&(data -> intval), sizeof(long));
}

int _int_cmp(data_t *self, data_t *other) {
  return self -> intval - other -> intval;
}

char * _int_tostring(data_t *data) {
  return itoa(data -> intval);
}

data_t * _int_cast(data_t *data, int totype) {
  switch (totype) {
    case Float:
      return data_create(Float, (double) data -> intval);
    case Bool:
      return data_create(Bool, data -> intval);
    default:
      return NULL;
  }
}

data_t * _int_parse(char *str) {
  char *endptr;
  char *ptr;
  long  val;

  /*
   * Using strtod here instead of strtol. strtol parses a number with a
   * leading '0' as octal, which nobody wants these days. Also less chance
   * of overflows.
   *
   * After a successful parse we check that the number is within bounds
   * and that it's actually an long, i.e. that it doesn't have a decimal
   * point or exponent.
   */
  val = strtol(str, &endptr, 0);
  if ((*endptr == 0) || (isspace(*endptr))) {
    if ((val < LONG_MIN) || (val > LONG_MAX)) {
      return NULL;
    }
    ptr = strpbrk(str, ".eE");
    if (ptr && (ptr < endptr)) {
      return NULL;
    } else {
      return data_create(Int, (long) val);
    }
  } else {
    return NULL;
  }
}

data_t * _int_fallback(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  methoddescr_t  *float_method;

  float_method = typedescr_get_method(typedescr_get(Float), name);
  if (float_method) {
    return data_execute_method(self, float_method, args, kwargs);
  } else {
    return NULL;
  }
}

/* ----------------------------------------------------------------------- */

data_t * _int_add(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t  *d;
  data_t  *ret;
  int      type = Int;
  int      ix;
  long     intret = 0;
  double   fltret = 0.0;
  double   dblval;
  long     longval;
  int      minus = name && (name[0] == '-');
  
  if (!array_size(args)) {
    return data_create(Int, (minus) ? -1 * self -> intval : self -> intval);
  }

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (data_type(d) == Float) {
      type = Float;
      break;
    }
  }
  if (type == Float) {
    fltret = (double) self -> intval;
  } else {
    intret = self -> intval;
  }
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    switch(type) {
      case Int:
        /* Type of d must be Int, can't be Float */
        longval = data_longval(d);
        if (minus) {
          longval = -longval;
        }
        intret += longval;
        break;
      case Float:
        dblval = data_dblval(d);
        if (minus) {
          dblval = -dblval;
        }
        fltret += dblval;
        break;
    }
  }
  ret = (type == Int) 
    ? data_create(type, intret)
    : data_create(type, fltret); 
  /* FIXME Why? Why not data_create(type, (type == Int) ? ...: ...)?
      Whyt does that not work (gives garbage intval) */
  return ret;
}

data_t * _int_mult(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     type = Int;
  int     ix;
  long    intret = 0;
  double  fltret = 0.0;
  int     plus = (name[0] == '+');

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (data_type(d) == Float) {
      type = Float;
      break;
    }
  }
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    switch(type) {
      case Int:
        /* Type of d must be Int, can't be Float */
        intret *= data_longval(d);
        break;
      case Float:
        fltret *= data_dblval(d);
        break;
    }
  }
  ret = (type == Int) 
    ? data_create(type, intret)
    : data_create(type, fltret);
  return ret;
}

data_t * _int_div(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;
  data_t *ret;
  long    intret = 0;
  double  fltret = 0.0;

  denom = (data_t *) array_get(args, 0);
  switch (data_type(denom)) {
    case Int:
      intret = data_longval(self) / data_longval(denom);
      ret = data_create(Int, intret);
      break;
    case Float:
      fltret = data_dblval(self) / data_dblval(denom);
      ret = data_create(Float, fltret);
      break;
  }
  return ret;
}

data_t * _int_mod(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  return data_create(Int, data_longval(self) % data_longval(denom));
}

data_t * _int_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, labs(data_longval(self)));
}

/*
 * --------------------------------------------------------------------------
 * Bool datatype functions
 * --------------------------------------------------------------------------
 */

data_t * _bool_new(data_t *target, va_list arg) {
  long val;

  val = va_arg(arg, long);
  target -> intval = val ? 1 : 0;
  return target;
}

char * _bool_tostring(data_t *data) {
  return btoa(data -> intval);
}

data_t * _bool_parse(char *str) {
  return data_create(Bool, atob(str));
}

data_t * _bool_cast(data_t *data, int totype) {
  switch (totype) {
    case Int:
      return data_create(Int, data -> intval);
    default:
      return NULL;
  }
}

/* ----------------------------------------------------------------------- */

data_t * _bool_and(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  int     retval = data_longval(self);

  for (ix = 0; retval && (ix < array_size(args)); ix++) {
    d = (data_t *) array_get(args, ix);
    retval = retval && data_longval(d);
  }
  return data_create(Bool, retval);
}

data_t * _bool_or(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  int     retval = data_longval(self);

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    retval = retval || data_longval(d);
  }
  return data_create(Bool, retval);
}

data_t * _bool_not(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Bool, !data_longval(self));
}

