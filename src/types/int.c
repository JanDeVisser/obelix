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

extern typedescr_t typedescr_int =   {
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
  fallback:              _int_fallback
};

extern typedescr_t typedescr_bool =   {
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
  hash:     (hash_t)     _int_hash
};

extern methoddescr_t methoddescr_int[] = {
  { type: Int, name: "+",    method: _int_add,  min_args: 2, max_args: -1 },
  { type: Int, name: "-",    method: _int_add,  min_args: 2, max_args: -1 },
  { type: Int, name: "sum",  method: _int_add,  min_args: 2, max_args: -1 },
  { type: Int, name: "*",    method: _int_mult, min_args: 2, max_args: -1 },
  { type: Int, name: "mult", method: _int_mult, min_args: 2, max_args: -1 },
  { type: Int, name: "/",    method: _int_div,  min_args: 2, max_args: 2  },
  { type: Int, name: "div",  method: _int_div,  min_args: 2, max_args: 2  },
  { type: Int, name: "%",    method: _int_mod,  min_args: 2, max_args: 2  },
  { type: Int, name: "mod",  method: _int_mod,  min_args: 2, max_args: 2  },
  { type: Int, name: "abs",  method: _int_abs,  min_args: 1, max_args: 1  },
  { type: -1,  name: NULL,   method: NULL,      min_args: 0, max_args: 0  },
};

extern methoddescr_t methoddescr_bool[] = {
  { type: Bool, name: "&&",  method: _bool_and, min_args: 2, max_args: 2  },
  { type: Bool, name: "and", method: _bool_and, min_args: 2, max_args: 2  },
  { type: Bool, name: "||",  method: _bool_or,  min_args: 2, max_args: 2  },
  { type: Bool, name: "or",  method: _bool_or,  min_args: 2, max_args: 2  },
  { type: Bool, name: "!",   method: _bool_not, min_args: 1, max_args: 1  },
  { type: Bool, name: "not", method: _bool_not, min_args: 1, max_args: 1  },
  { type: -1,  name: NULL,   method: NULL,      min_args: 0, max_args: 0  },
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
      return data_create_int((int) val);
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

data_t * _int_add(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     type = Int;
  int     ix;
  long    intret = 0;
  double  fltret = 0.0;
  double  val;
  int     minus = name && (name[0] == '-');

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (!data_is_numeric(d)) {
      type = data_type(d);
      break;
    } else if (data_type(d) == Float) {
      type = Float;
    }
  }
  if ((type != Int) && (type != Float)) {
    ret = data_error(ErrorType,
                     ((minus)
                       ? "Cannot subtract integers and objects of type '%s'"
                       : "Cannot add integers and objects of type '%s'"),
                     typedescr_get(type) -> typecode);
  } else {
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
          intret += (minus) ? (-(d -> intval)) : d -> intval;
          break;
        case Float:
          /* Here type of d can be Int or Float */
          val = (data_type(d) == Int) ? (double) d -> intval : d -> dblval;
          fltret += (minus) ? -val : val;
          break;
      }
    }
    ret = data_create(type, (type == Int) ? intret : fltret);
  }
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

  if (!args || !array_size(args)) {
    return data_error(ErrorArgCount, "int(*) requires at least two parameters");
  }
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (!data_is_numeric(d)) {
      type = data_type(d);
      break;
    } else if (data_type(d) == Float) {
      type = Float;
    }
  }
  if ((type != Int) && (type != Float)) {
    ret = data_error(ErrorType,
                     "Cannot multiply integers and objects of type '%s'",
                     typedescr_get(type) -> typename);
  } else {
    for (ix = 0; ix < array_size(args); ix++) {
      d = (data_t *) array_get(args, ix);
      switch(type) {
        case Int:
          /* Type of d must be Int, can't be Float */
          intret *= d -> intval;
          break;
        case Float:
          /* Here type of d can be Int or Float */
          fltret *= (data_type(d) == Int) ? (double) d -> intval : d -> dblval;
          break;
      }
    }
    ret = data_create(type, (type == Int) ? intret : fltret);
  }
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
      intret = self -> intval / denom -> intval;
      ret = data_create(Int, intret);
      break;
    case Float:
      fltret = ((double) self -> intval) / denom -> dblval;
      ret = data_create(Float, fltret);
      break;
    default:
      ret = data_error(ErrorType,
                       "Cannot divide an integer and an object of type '%s'",
                       typedescr_get(data_type(denom)) -> typename);
      break;
  }
  return ret;
}

data_t * _int_mod(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  if (data_type(denom) != Int) {
    return data_error(ErrorType,
                      "Cannot calculate the remainder of an integer and an object of type '%s'",
                      typedescr_get(data_type(denom)) -> typename);
  } else {
    return data_create(Int, self -> intval % denom -> intval);
  }
}

data_t * _int_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, labs(self -> intval));
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

data_t * _bool_and(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *other;

  other = (data_t *) array_get(args, 0);
  if (data_type(other) != Bool) {
    return data_error(ErrorType,
                      "Cannot AND a boolean and an object of type '%s'",
                      typedescr_get(data_type(other)) -> typename);
  } else {
    return data_create(Bool, self -> intval && other -> intval);
  }
}

data_t * _bool_or(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *other;

  other = (data_t *) array_get(args, 0);
  if (data_type(other) != Bool) {
    return data_error(ErrorType,
                      "Cannot OR a boolean and an object of type '%s'",
                      typedescr_get(data_type(other)) -> typename);
  } else {
    return data_create(Bool, self -> intval || other -> intval);
  }
}

data_t * _bool_not(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Bool, !(self -> intval));
}

