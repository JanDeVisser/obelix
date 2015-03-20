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

static void          _int_init(void) __attribute__((constructor));
static data_t *      _int_new(data_t *, va_list);
static int           _int_cmp(data_t *, data_t *);
static char *        _int_tostring(data_t *);
static data_t *      _int_cast(data_t *, int);
static unsigned int  _int_hash(data_t *);
static data_t *      _int_parse(char *);
static double        _int_fltvalue(data_t *);
static int           _int_intvalue(data_t *);

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

static vtable_t _vtable_int[] = {
  { .id = FunctionNew,      .fnc = (void_t) _int_new },
  { .id = FunctionCmp,      .fnc = (void_t) _int_cmp },
  { .id = FunctionToString, .fnc = (void_t) _int_tostring },
  { .id = FunctionParse,    .fnc = (void_t) _int_parse },
  { .id = FunctionCast,     .fnc = (void_t) _int_cast },
  { .id = FunctionHash,     .fnc = (void_t) _int_hash },
  { .id = FunctionFltValue, .fnc = (void_t) _int_fltvalue },
  { .id = FunctionIntValue, .fnc = (void_t) _int_intvalue },
  { .id = FunctionNone,     .fnc = NULL }
};

static int _inherits_int[] = { Number };

static typedescr_t _typedescr_int = {
  .type          = Int,
  .type_name     = "int",
  .inherits_size = 1,
  .inherits      = _inherits_int,
  .promote_to    = Float,
  .vtable        = _vtable_int
};

static vtable_t _vtable_bool[] = {
  { .id = FunctionNew,      .fnc = (void_t) _bool_new },
  { .id = FunctionCmp,      .fnc = (void_t) _int_cmp },
  { .id = FunctionToString, .fnc = (void_t) _bool_tostring },
  { .id = FunctionParse,    .fnc = (void_t) _bool_parse },
  { .id = FunctionCast,     .fnc = (void_t) _bool_cast },
  { .id = FunctionHash,     .fnc = (void_t) _int_hash },
  { .id = FunctionNone,     .fnc = NULL }
};

static int _inherits_bool[] = { Int };

static typedescr_t _typedescr_bool = {
  .type          = Bool,
  .type_name     = "bool",
  .inherits_size = 1,
  .inherits      = _inherits_bool,
  .vtable        = _vtable_bool,
  .promote_to    = Int
};

static methoddescr_t _methoddescr_int[] = {
  { .type = Int,    .name = "+",    .method = _int_add,  .argtypes = { Number, NoType, NoType }, .minargs = 0, .varargs = 1 },
  { .type = Int,    .name = "-",    .method = _int_add,  .argtypes = { Number, NoType, NoType }, .minargs = 0, .varargs = 1 },
  { .type = Int,    .name = "sum",  .method = _int_add,  .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = Int,    .name = "*",    .method = _int_mult, .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = Int,    .name = "mult", .method = _int_mult, .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = Int,    .name = "/",    .method = _int_div,  .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Int,    .name = "div",  .method = _int_div,  .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Int,    .name = "%",    .method = _int_mod,  .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Int,    .name = "mod",  .method = _int_mod,  .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Int,    .name = "abs",  .method = _int_abs,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,   .method = NULL,      .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static methoddescr_t _methoddescr_bool[] = {
  { .type = Bool,   .name = "&&",  .method = _bool_and,  .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Bool,   .name = "and", .method = _bool_and,  .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Bool,   .name = "||",  .method = _bool_or,   .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Bool,   .name = "or",  .method = _bool_or,   .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = Bool,   .name = "!",   .method = _bool_not,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = Bool,   .name = "not", .method = _bool_not,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,  .method = NULL,       .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/*
 * --------------------------------------------------------------------------
 * Int datatype functions
 * --------------------------------------------------------------------------
 */

void _int_init(void) {
  typedescr_register(&_typedescr_int);
  typedescr_register(&_typedescr_bool);
  typedescr_register_methods(_methoddescr_int);
  typedescr_register_methods(_methoddescr_bool);
}

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
  
  if (!strtoint(str, &val)) {
    return data_create(Int, (long) val);
  } else {
    return NULL;
  }
}

double _int_fltvalue(data_t *data) {
  return (double) data -> intval;
}

int _int_intvalue(data_t *data) {
  return data -> intval;
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
  
  if (!args || !array_size(args)) {
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
    fltret = data_floatval(self);
  } else {
    intret = data_intval(self);
  }
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    switch(type) {
      case Int:
        /* Type of d must be Int, can't be Float */
        longval = data_intval(d);
        if (minus) {
          longval = -longval;
        }
        intret += longval;
        break;
      case Float:
        dblval = data_floatval(d);
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
      Why does that not work (gives garbage intval) */
  return ret;
}

data_t * _int_mult(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     type = Int;
  int     ix;
  double  fltret;
  long    intret;

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    if (data_type(d) == Float) {
      type = Float;
      break;
    }
  }
  if (type == Float) {
    fltret = data_floatval(self);
  } else {
    intret = data_intval(self);
  }
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    switch(type) {
      case Int:
        /* Type of d must be Int, can't be Float */
        intret *= data_intval(d);
        break;
      case Float:
        fltret *= data_floatval(d);
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
      intret = data_intval(self) / data_intval(denom);
      ret = data_create(Int, intret);
      break;
    case Float:
      fltret = data_floatval(self) / data_floatval(denom);
      ret = data_create(Float, fltret);
      break;
  }
  return ret;
}

data_t * _int_mod(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  return data_create(Int, data_intval(self) % data_intval(denom));
}

data_t * _int_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, labs(data_intval(self)));
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
  data_t *i = _int_parse(str);
  
  if (i) {
    return data_create(Bool, data_intval(i));
  } else {
    return data_create(Bool, atob(str));
  }
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
  int     retval = data_intval(self);

  for (ix = 0; retval && (ix < array_size(args)); ix++) {
    d = (data_t *) array_get(args, ix);
    retval = retval && data_intval(d);
  }
  return data_create(Bool, retval);
}

data_t * _bool_or(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  int     ix;
  int     retval = data_intval(self);

  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    retval = retval || data_intval(d);
  }
  return data_create(Bool, retval);
}

data_t * _bool_not(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Bool, !data_intval(self));
}

