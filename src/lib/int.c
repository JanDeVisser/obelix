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

extern void          int_init(void);

static data_t *      _int_new(int, va_list);
static int           _int_cmp(int_t *, int_t *);
static char *        _int_allocstring(int_t *);
static data_t *      _int_cast(int_t *, int);
static unsigned int  _int_hash(int_t *);
static data_t *      _int_parse(char *);
static data_t *      _int_incr(int_t *);
static data_t *      _int_decr(int_t *);
static double        _int_fltvalue(int_t *);
static int           _int_intvalue(int_t *);

static data_t *      _int_add(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_mult(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_div(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_mod(data_t *, char *, array_t *, dict_t *);
static data_t *      _int_abs(data_t *, char *, array_t *, dict_t *);

static data_t *      _bool_new(int, va_list);
static char *        _bool_tostring(int_t *);
static data_t *      _bool_parse(char *);
static data_t *      _bool_cast(int_t *, int);

static vtable_t _vtable_int[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _int_new },
  { .id = FunctionCmp,         .fnc = (void_t) _int_cmp },
  { .id = FunctionAllocString, .fnc = (void_t) _int_allocstring },
  { .id = FunctionParse,       .fnc = (void_t) _int_parse },
  { .id = FunctionCast,        .fnc = (void_t) _int_cast },
  { .id = FunctionHash,        .fnc = (void_t) _int_hash },
  { .id = FunctionFltValue,    .fnc = (void_t) _int_fltvalue },
  { .id = FunctionIntValue,    .fnc = (void_t) _int_intvalue },
  { .id = FunctionDecr,        .fnc = (void_t) _int_decr },
  { .id = FunctionIncr,        .fnc = (void_t) _int_incr },
  { .id = FunctionNone,        .fnc = NULL }
};

static typedescr_t _typedescr_int = {
  .type          = Int,
  .type_name     = "int",
  .promote_to    = Float,
  .vtable        = _vtable_int
};

static vtable_t _vtable_bool[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _bool_new },
  { .id = FunctionCmp,         .fnc = (void_t) _int_cmp },
  { .id = FunctionToString,    .fnc = (void_t) _bool_tostring },
  { .id = FunctionParse,       .fnc = (void_t) _bool_parse },
  { .id = FunctionCast,        .fnc = (void_t) _bool_cast },
  { .id = FunctionHash,        .fnc = (void_t) _int_hash },
  { .id = FunctionNone,        .fnc = NULL }
};

static typedescr_t _typedescr_bool = {
  .type          = Bool,
  .type_name     = "bool",
  .inherits      = { Int, NoType, NoType },
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

dict_t * _ints = NULL;
int_t *  bool_true = NULL;
int_t *  bool_false = NULL;

/*
 * --------------------------------------------------------------------------
 * Int datatype functions
 * --------------------------------------------------------------------------
 */

void int_init(void) {
  typedescr_register(&_typedescr_int);
  typedescr_register(&_typedescr_bool);
  typedescr_register_methods(_methoddescr_int);
  _ints = intdata_dict_create();
  bool_true = (int_t *) data_create(Bool, 1);
  bool_true -> _d.free_me = Constant;
  bool_false = (int_t *) data_create(Bool, 0);
  bool_false -> _d.free_me = Constant;
}

data_t * _int_new(int type, va_list arg) {
  return (data_t *) int_create(va_arg(arg, long));
}

unsigned int _int_hash(int_t *data) {
  return hash(&(data -> i), sizeof(long));
}

int _int_cmp(int_t *self, int_t *other) {
  return self -> i - other -> i;
}

char * _int_allocstring(int_t *data) {
  char *buf;

  asprintf(&buf, "%ld", data -> i);
  return buf;
}

data_t * _int_cast(int_t *data, int totype) {
  switch (totype) {
    case Float:
      return flt_to_data((double) data -> i);
    case Bool:
      return int_as_bool(data -> i);
    default:
      return NULL;
  }
}

data_t * _int_parse(char *str) {
  long  val;

  if (!strtoint(str, &val)) {
    return int_to_data((long) val);
  } else {
    return NULL;
  }
}

data_t * _int_incr(int_t *self) {
  return int_to_data(self -> i + 1);
}

data_t * _int_decr(int_t *self) {
  return int_to_data(self -> i - 1);
}

double _int_fltvalue(int_t *data) {
  return (double) data -> i;
}

int _int_intvalue(int_t *data) {
  return data -> i;
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
    intret = ((int_t *) self) -> i;
    return int_to_data((minus) ? -1 * intret : intret);
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
  ret = (type == Int) ? (data_t *) int_create(intret) : (data_t *) flt_create(fltret);
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
  ret = (type == Int) ? (data_t *) int_create(intret) : (data_t *) flt_create(fltret);
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
      ret = int_to_data(intret);
      break;
    case Float:
      fltret = data_floatval(self) / data_floatval(denom);
      ret = flt_to_data(fltret);
      break;
  }
  return ret;
}

data_t * _int_mod(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *denom;

  denom = (data_t *) array_get(args, 0);
  return int_to_data(data_intval(self) % data_intval(denom));
}

data_t * _int_abs(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return int_to_data(labs(data_intval(self)));
}

/*
 * --------------------------------------------------------------------------
 * Bool datatype functions
 * --------------------------------------------------------------------------
 */

data_t * _bool_new(int type, va_list arg) {
  long   val;
  int_t *ret;

  val = va_arg(arg, long);
  if (bool_true && bool_false) {
    return (data_t *) bool_get(val);
  } else {
    ret = data_new(Bool, int_t);
    ret -> i = (val != 0);
    return (data_t *) ret;
  }
}

char * _bool_tostring(int_t *data) {
  return btoa(data -> i);
}

data_t * _bool_parse(char *str) {
  data_t *i = _int_parse(str);

  if (i) {
    return int_as_bool(data_intval(i));
  } else {
    return int_as_bool(atob(str));
  }
}

data_t * _bool_cast(int_t *data, int totype) {
  switch (totype) {
    case Int:
      return int_to_data(data -> i);
    default:
      return NULL;
  }
}

/* ----------------------------------------------------------------------- */

int_t * int_create(intptr_t val) {
  int_t *ret;

  ret = (int_t *) data_dict_get(_ints, (void *) val);
  if (!ret) {
    ret = data_new(Int, int_t);
    ret -> i = val;
    ret -> _d.free_me = Constant;
    dict_put(_ints, (void *) val, ret);
  }
  return ret;
}

int_t * bool_get(long value) {
  return (value) ? bool_true : bool_false;
}

/* ----------------------------------------------------------------------- */
