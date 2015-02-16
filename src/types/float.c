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
#include <stdint.h>

static void          _float_init(void) __attribute__((constructor));
static data_t *      _float_new(data_t *, va_list);
static int           _float_cmp(data_t *, data_t *);
static char *        _float_tostring(data_t *);
static unsigned int  _float_hash(data_t *);
static data_t *      _float_parse(char *);
static data_t *      _float_cast(data_t *, int);
static double        _float_fltvalue(data_t *);
static int           _float_intvalue(data_t *);

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

static typedescr_t _typedescr_number =   {
  .type           = Number,
  .type_name      = "number",
  .inherits_size  = 0,
  .vtable         = NULL,
  .promote_to     = NoType
};

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

static int _inherits_float[] = { Number };

static vtable_t _vtable_float[] = {
  { .id = MethodNew,      .fnc = (void_t) _float_new },
  { .id = MethodCmp,      .fnc = (void_t) _float_cmp },
  { .id = MethodToString, .fnc = (void_t) _float_tostring },
  { .id = MethodParse,    .fnc = (void_t) _float_parse },
  { .id = MethodCast,     .fnc = (void_t) _float_cast },
  { .id = MethodHash,     .fnc = (void_t) _float_hash },
  { .id = MethodFltValue, .fnc = (void_t) _float_fltvalue },
  { .id = MethodIntValue, .fnc = (void_t) _float_intvalue },
  { .id = MethodNone,     .fnc = NULL }
};

static typedescr_t _typedescr_float =   {
  .type           = Float,
  .type_name      = "float",
  .inherits_size  = 1,
  .inherits       = _inherits_float,
  .vtable         = _vtable_float,
  .promote_to     = NoType
};

/*
 * --------------------------------------------------------------------------
 * Float datatype functions
 * --------------------------------------------------------------------------
 */

double data_floatval(data_t *data) {
  double (*fltvalue)(data_t *);
  int    (*intvalue)(data_t *);
  data_t  *flt;
  double   ret;
  
  if (data_type(data) == Float) {
    return data -> dblval;
  } else {
    fltvalue = (double (*)(data_t *)) data_get_function(data, MethodFltValue);
    if (fltvalue) {
      return fltvalue(data);
    } else {
      intvalue = (int (*)(data_t *)) data_get_function(data, MethodIntValue);
      if (intvalue) {
        return (double) intvalue(data);
      } else {
        flt = data_cast(data, Float);
        if (flt && !data_is_error(flt)) {
          ret = flt -> dblval;
          data_free(flt);
          return ret;
        } else {
          return nan("Can't convert atom to float");
        }
      }
    }
  }
}

int data_intval(data_t *data) {
  int    (*intvalue)(data_t *);
  double (*fltvalue)(data_t *);
  data_t  *i;
  int      ret;
  
  if ((data_type(data) == Int) || (data_type(data) == Bool)) {
    return data -> intval;
  } else {
    intvalue = (int (*)(data_t *)) data_get_function(data, MethodIntValue);
    if (intvalue) {
      return intvalue(data);
    } else {
      fltvalue = (double (*)(data_t *)) data_get_function(data, MethodFltValue);
      if (fltvalue) {
        return (int) fltvalue(data);
      } else {
        i = data_cast(data, Int);
        if (i && !data_is_error(i)) {
          ret = i -> intval;
          data_free(i);
          return ret;
        } else {
          return 0;
        }
      }
    }
  }
}

void _float_init(void) {
  typedescr_register(&_typedescr_number);
  typedescr_register(&_typedescr_float);
  typedescr_register_methods(_methoddescr_number);
}

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
  data_t *ret = NULL;

  switch (totype) {
    case Int:
      ret = data_create(Int, (long) data -> dblval);
      break;
    case Bool:
      ret = data_create(Bool, data -> dblval != 0);
      break;
  }
  return ret;
}

double _float_fltvalue(data_t *data) {
  return data -> dblval;
}

int _float_intvalue(data_t *data) {
  return (int) data -> dblval;
}

/* ----------------------------------------------------------------------- */

data_t * _number_add(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  double  retval = data_floatval(self);
  double  val;
  int     plus = (name[0] == '+');

  if (!array_size(args)) {
    return data_create(Int, (plus) ? data_floatval(self) : -1.0 * data_floatval(self));
  }
  retval = self -> dblval;
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    val = data_floatval(d);
    if (!plus) {
      val = -val;
    }
    retval += val;
  }
  return data_create(Float, retval);
}

data_t * _number_mult(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  double  retval = data_floatval(self);
  
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    retval *= data_floatval(d);
  }
  return data_create(Float, retval);
}

data_t * _number_div(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  return data_create(Float, data_floatval(self) / data_floatval(denom));
}

data_t * _number_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, fabs(data_floatval(self)));
}

data_t * _number_round(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) round(data_floatval(self)));
}

data_t * _number_trunc(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) trunc(data_floatval(self)));
}

data_t * _number_floor(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) floor(data_floatval(self)));
}

data_t * _number_ceil(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, (long) ceil(data_floatval(self)));
}

/*
 * The functions below can be called with Int as self by _int_fallback
 */

data_t * _number_pow(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *exp;

  exp = (data_t *) array_get(args, 0);
  return data_create(Float, pow(data_floatval(self), data_floatval(exp)));
}

data_t * _number_sin(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, sin(data_floatval(self)));
}

data_t * _number_cos(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, cos(data_floatval(self)));
}

data_t * _number_tan(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, tan(data_floatval(self)));
}

data_t * _number_sqrt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Float, sqrt(data_floatval(self)));
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

