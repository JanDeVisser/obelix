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


#include "libcore.h"

#include <stdio.h>

#include <data.h>
#include <threadonce.h>

extern void          int_init(void);

static data_t *      _int_new(int, va_list);
static int           _int_cmp(int_t *, int_t *);
static char *        _int_allocstring(int_t *);
static data_t *      _int_cast(int_t *, int);
static unsigned int  _int_hash(int_t *);
static data_t *      _int_incr(int_t *);
static data_t *      _int_decr(int_t *);
static double        _int_fltvalue(int_t *);
static int           _int_intvalue(int_t *);

static data_t *      _int_add(data_t *, char *, arguments_t *);
static data_t *      _int_mult(data_t *, char *, arguments_t *);
static data_t *      _int_div(data_t *, char *, arguments_t *);
static data_t *      _int_mod(data_t *, char *, arguments_t *);
static data_t *      _int_abs(data_t *, char *, arguments_t *);

static data_t *      _bool_new(int, va_list);
static char *        _bool_tostring(int_t *);
static data_t *      _bool_parse(char *);
static data_t *      _bool_cast(int_t *, int);

static vtable_t _vtable_Int[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _int_new },
  { .id = FunctionCmp,         .fnc = (void_t) _int_cmp },
  { .id = FunctionAllocString, .fnc = (void_t) _int_allocstring },
  { .id = FunctionParse,       .fnc = (void_t) int_parse },
  { .id = FunctionCast,        .fnc = (void_t) _int_cast },
  { .id = FunctionHash,        .fnc = (void_t) _int_hash },
  { .id = FunctionFltValue,    .fnc = (void_t) _int_fltvalue },
  { .id = FunctionIntValue,    .fnc = (void_t) _int_intvalue },
  { .id = FunctionDecr,        .fnc = (void_t) _int_decr },
  { .id = FunctionIncr,        .fnc = (void_t) _int_incr },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Int[] = {
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

static vtable_t _vtable_Bool[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _bool_new },
  { .id = FunctionCmp,         .fnc = (void_t) _int_cmp },
  { .id = FunctionToString,    .fnc = (void_t) _bool_tostring },
  { .id = FunctionParse,       .fnc = (void_t) _bool_parse },
  { .id = FunctionCast,        .fnc = (void_t) _bool_cast },
  { .id = FunctionHash,        .fnc = (void_t) _int_hash },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t * _methods_Bool = NULL;

int_t *  bool_true = NULL;
int_t *  bool_false = NULL;

/*
 * --------------------------------------------------------------------------
 * Int datatype functions
 * --------------------------------------------------------------------------
 */

void int_init(void) {
  builtin_typedescr_register(Int, "int", int_t);
  typedescr_get(Int) -> promote_to = Float;
  typedescr_set_size(Int, int_t);
  builtin_typedescr_register(Bool, "bool", int_t);
  typedescr_get(Bool) -> promote_to = Int;
  typedescr_set_size(Bool, int_t);
  typedescr_assign_inheritance(Bool, Int);
  bool_true = (int_t *) data_create(Bool, 1);
  bool_true -> _d.data_semantics = DataSemanticsConstant;
  bool_false = (int_t *) data_create(Bool, 0);
  bool_false -> _d.data_semantics = DataSemanticsConstant;
}

data_t * _int_new(int _unused_ type, va_list arg) {
  return (data_t *) int_create(va_arg(arg, intptr_t));
}

unsigned int _int_hash(int_t *data) {
  return hash(&(data -> i), sizeof(long));
}

int _int_cmp(int_t *self, int_t *other) {
  return (int) (self -> i - other -> i);
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
  return (int) data -> i;
}

/* ----------------------------------------------------------------------- */

data_t * _int_add(data_t *self, char *name, arguments_t *args) {
  data_t  *d;
  data_t  *ret;
  int      type = Int;
  int      ix;
  long     intret = 0;
  double   fltret = 0.0;
  double   dblval;
  long     longval;
  int      minus = name && (name[0] == '-');

  if (!args || !arguments_args_size(args)) {
    intret = ((int_t *) self) -> i;
    return int_to_data((minus) ? -1 * intret : intret);
  }

  for (ix = 0; ix < arguments_args_size(args); ix++) {
    d = data_uncopy(arguments_get_arg(args, ix));
    if (data_hastype(d, Float)) {
      type = Float;
      break;
    }
  }
  if (type == Float) {
    fltret = data_floatval(self);
  } else {
    intret = data_intval(self);
  }
  for (ix = 0; ix < arguments_args_size(args); ix++) {
    d = data_uncopy(arguments_get_arg(args, ix));
    if (type == Int) {
      /* Type of d must be Int, can't be Float */
      longval = data_intval(d);
      if (minus) {
        longval = -longval;
      }
      intret += longval;
    } else {
      dblval = data_floatval(d);
      if (minus) {
        dblval = -dblval;
      }
      fltret += dblval;
    }
  }
  ret = (type == Int) ? (data_t *) int_create(intret) : (data_t *) float_create(fltret);
  return ret;
}

data_t * _int_mult(data_t *self, char _unused_ *name, arguments_t *args) {
  data_t *d;
  data_t *ret;
  int     type = Int;
  int     ix;
  double  fltret = 0.0;
  long    intret = 0;

  for (ix = 0; ix < arguments_args_size(args); ix++) {
    d = data_uncopy(arguments_get_arg(args, ix));
    if (data_hastype(d, Float)) {
      type = Float;
      break;
    }
  }
  if (type == Float) {
    fltret = data_floatval(self);
  } else {
    intret = data_intval(self);
  }
  for (ix = 0; ix < arguments_args_size(args); ix++) {
    d = data_uncopy(arguments_get_arg(args, ix));
    if (type == Int) {
      intret *= data_intval(d);
    } else {
      fltret *= data_floatval(d);
    }
  }
  ret = (type == Int) ? (data_t *) int_create(intret) : (data_t *) float_create(fltret);
  return ret;
}

data_t * _int_div(data_t *self, char _unused_ *name, arguments_t *args) {
  data_t *denom;
  data_t *ret;
  long    intret = 0;
  double  fltret;

  denom = data_uncopy(arguments_get_arg(args, 0));
  if (data_hastype(denom, Int)) {
    intret = data_intval(self) / data_intval(denom);
    ret = int_to_data(intret);
  } else {
    fltret = data_floatval(self) / data_floatval(denom);
    ret = flt_to_data(fltret);
  }
  return ret;
}

data_t * _int_mod(data_t *self, char _unused_ *name, arguments_t *args) {
  data_t *denom;

  denom = arguments_get_arg(args, 0);
  return int_to_data(data_intval(self) % data_intval(denom));
}

data_t * _int_abs(data_t *self, char _unused_ *name, arguments_t _unused_ *args) {
  return int_to_data(labs(data_intval(self)));
}

/*
 * --------------------------------------------------------------------------
 * Bool datatype functions
 * --------------------------------------------------------------------------
 */

data_t * _bool_new(int _unused_ type, va_list arg) {
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
  data_t *i = (data_t *) int_parse(str);

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

static void _integer_cache_init(void);

#define INTEGER_CACHE_SIZE     256
static int_t                  *_integer_cache[INTEGER_CACHE_SIZE];
static dict_t *                _ints = NULL;
static int_t *                 _zero;
static int_t *                 _one;
static int_t *                 _minusone;
static int_t *                 _two;
THREAD_ONCE(_integer_cache_once);

static int_t * _int_make(intptr_t i) {
  int_t *ret;

  ret = data_new(Int, int_t);
  ret -> i = i;
  ret -> _d.data_semantics = DataSemanticsConstant;
  return ret;
}

static void _integer_cache_init(void) {
  for (int ix = 0; ix < INTEGER_CACHE_SIZE; ix++) {
    _integer_cache[ix] = NULL;
  }
  _ints = intdata_dict_create();
  _zero = _int_make(0);
  _one = _int_make(1);
  _minusone = _int_make(-1);
  _two = _int_make(2);
}

int_t * int_create(intptr_t val) {
  int_t *ret;

  ONCE(_integer_cache_once, _integer_cache_init);
  if (!val) {
    ret = _zero;
  } else if (val == 1) {
    ret = _one;
  } else if (val == -1) {
    ret = _minusone;
  } else if (val == 2) {
    ret = _two;
  } else if (val < INTEGER_CACHE_SIZE) {
    if (!(ret = _integer_cache[val])) {
      ret = _int_make(val);
      _integer_cache[val] = ret;
    }
  } else {
    if (!(ret = (int_t *) data_dict_get(_ints, (void *) val))) {
      ret = _int_make(val);
      dict_put(_ints, (void *) val, ret);
    }
  }
  return ret;
}

int_t * int_parse(char *str) {
  long  val;

  if (!strtoint(str, &val)) {
    return int_create(val);
  } else {
    return NULL;
  }
}

int_t * bool_get(long value) {
  return (value) ? bool_true : bool_false;
}

/* ----------------------------------------------------------------------- */
