/*
 * /obelix/src/data.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <error.h>
#include <list.h>

static data_t *      _data_new_error(data_t *, va_list);
static unsigned int  _data_hash_error(data_t *);
static data_t *      _data_copy_error(data_t *, data_t *);
static int           _data_cmp_error(data_t *, data_t *);
static char *        _data_tostring_error(data_t *);

static data_t *      _data_new_pointer(data_t *, va_list);
static int           _data_cmp_pointer(data_t *, data_t *);
static unsigned int  _data_hash_pointer(data_t *);
static char *        _data_tostring_pointer(data_t *);

static data_t *      _data_new_string(data_t *, va_list);
static data_t *      _data_copy_string(data_t *, data_t *);
static int           _data_cmp_string(data_t *, data_t *);
static unsigned int  _data_hash_string(data_t *);
static char *        _data_tostring_string(data_t *);

static data_t *      _data_new_int(data_t *, va_list);
static int           _data_cmp_int(data_t *, data_t *);
static char *        _data_tostring_int(data_t *);
static unsigned int  _data_hash_int(data_t *);
static data_t *      _data_parse_int(char *);
static data_t *      _data_add_int(data_t *, char *, array_t *, dict_t *);

static data_t *      _data_new_float(data_t *, va_list);
static int           _data_cmp_float(data_t *, data_t *);
static char *        _data_tostring_float(data_t *);
static unsigned int  _data_hash_float(data_t *);
static data_t *      _data_parse_float(char *);

static char *        _data_tostring_bool(data_t *);
static data_t *      _data_parse_bool(char *);

static data_t *      _data_new_list(data_t *, va_list);
static data_t *      _data_copy_list(data_t *, data_t *);
static int           _data_cmp_list(data_t *, data_t *);
static char *        _data_tostring_list(data_t *);
static unsigned int  _data_hash_list(data_t *);

static data_t *      _data_list_len(data_t *, char *, array_t *, dict_t *);


static data_t *      _data_new_function(data_t *, va_list);
static data_t *      _data_copy_function(data_t *, data_t *);
static int           _data_cmp_function(data_t *, data_t *);
static char *        _data_tostring_function(data_t *);
/* static data_t *      _data_parse_function(char *); */
static unsigned int  _data_hash_function(data_t *);

static void          _data_initialize_types(void);
static data_t *      _data_create(datatype_t);

// -----------------------
// Data conversion support

int data_numtypes = Function + 1;
static typedescr_t builtins[] = {
  {
    type:                  Error,
    typecode:              "E",
    new:      (new_t)      _data_new_error,
    copy:     (copydata_t) _data_copy_error,
    cmp:      (cmp_t)      _data_cmp_error,
    free:     (free_t)     error_free,
    tostring: (tostring_t) _data_tostring_error,
    parse:                 NULL,
    hash:     (hash_t)     _data_hash_error
  },
  {
    type:                  Pointer,
    typecode:              "P",
    new:      (new_t)      _data_new_pointer,
    copy:                  NULL,
    cmp:      (cmp_t)      _data_cmp_pointer,
    free:                  NULL,
    tostring: (tostring_t) _data_tostring_pointer,
    parse:                 NULL,
    hash:     (hash_t)     _data_hash_pointer
  },
  {
    type:                  String,
    typecode:              "C",
    new:      (new_t)      _data_new_string,
    copy:     (copydata_t) _data_copy_string,
    cmp:      (cmp_t)      _data_cmp_string,
    free:     (free_t)     free,
    tostring: (tostring_t) _data_tostring_string,
    parse:    (parse_t)    data_create_string,
    hash:     (hash_t)     _data_hash_string
  },
  {
    type:                  Int,
    typecode:              "I",
    new:      (new_t)      _data_new_int,
    copy:                  NULL,
    cmp:      (cmp_t)      _data_cmp_int,
    free:                  NULL,
    tostring: (tostring_t) _data_tostring_int,
    parse:    (parse_t)    _data_parse_int,
    hash:     (hash_t)     _data_hash_int
  },
  {
    type:                  Float,
    typecode:              "F",
    new:      (new_t)      _data_new_float,
    copy:                  NULL,
    cmp:      (cmp_t)      _data_cmp_float,
    free:                  NULL,
    tostring: (tostring_t) _data_tostring_float,
    parse:    (parse_t)     _data_parse_float,
    hash:     (hash_t)     _data_hash_float
  },
  {
    type:                  Bool,
    typecode:              "B",
    new:      (new_t)      _data_new_int,
    copy:                  NULL,
    cmp:      (cmp_t)      _data_cmp_int,
    free:                  NULL,
    tostring: (tostring_t) _data_tostring_bool,
    parse:    (parse_t)    _data_parse_bool,
    hash:     (hash_t)     _data_hash_int
  },
  {
    type:                  List,
    typecode:              "L",
    new:      (new_t)      _data_new_list,
    copy:     (copydata_t) _data_copy_list,
    cmp:      (cmp_t)      _data_cmp_list,
    free:     (free_t)     list_free,
    tostring: (tostring_t) _data_tostring_list,
    parse:                 NULL,
    hash:     (hash_t)     _data_hash_list
  },
  {
    type:                  Function,
    typecode:              "U",
    new:      (new_t)      _data_new_function,
    copy:     (copydata_t) _data_copy_function,
    cmp:      (cmp_t)      _data_cmp_function,
    free:     (free_t)     function_free,
    tostring: (tostring_t) _data_tostring_function,
    parse:                 /* _data_parse_function, */ NULL,
    hash:     (hash_t)     _data_hash_function
  }
};

static typedescr_t *descriptors = builtins;

int data_count = 0;

/*
 * --------------------------------------------------------------------------
 * Error
 * --------------------------------------------------------------------------
 */

data_t * _data_new_error(data_t *target, va_list args) {
  int      code;
  error_t *e;

  code = va_arg(args, int);
  e = error_vcreate(code, args);
  target -> ptrval = e;
  return target;
}

unsigned int _data_hash_error(data_t *data) {
  return error_hash((error_t *) data -> ptrval);
}

data_t * _data_copy_error(data_t *target, data_t *src) {
  error_t *e;

  e = error_copy((error_t *) src -> ptrval);
  target -> ptrval = e;
  return target;
}

int _data_cmp_error(data_t *d1, data_t *d2) {
  return error_cmp((error_t *) d1 -> ptrval, (error_t *) d2 -> ptrval);
}

char * _data_tostring_error(data_t *data) {
  return error_tostring((error_t *) data -> ptrval);
}

/*
 * --------------------------------------------------------------------------
 * Pointer
 * --------------------------------------------------------------------------
 */

data_t * _data_new_pointer(data_t *target, va_list arg) {
  void *ptr;

  ptr = va_arg(arg, void *);
  target -> ptrval = ptr;
  return target;
}

int _data_cmp_pointer(data_t *d1, data_t *d2) {
  return (int)((long) d1 -> ptrval) - ((long) d2 -> ptrval);
}

char * _data_tostring_pointer(data_t *data) {
  static char buf[32];

  if (data -> ptrval) {
    snprintf(buf, 32, "%p", (void *) data -> ptrval);
    return buf;
  } else {
    return "Null";
  }
}

unsigned int _data_hash_pointer(data_t *data) {
  return hash(&(data -> ptrval), sizeof(void *));
}


/*
 * --------------------------------------------------------------------------
 * String
 * --------------------------------------------------------------------------
 */

data_t * _data_new_string(data_t *target, va_list arg) {
  char *str;

  str = va_arg(arg, char *);
  target -> ptrval = str ? strdup(str) : NULL;
  return target;
}

unsigned int _data_hash_string(data_t *data) {
  return strhash(data -> ptrval);
}

data_t * _data_copy_string(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval ? strdup(src -> ptrval) : NULL;
  return target;
}

int _data_cmp_string(data_t *d1, data_t *d2) {
  return strcmp((char *) d1 -> ptrval, (char *) d2 -> ptrval);
}

char * _data_tostring_string(data_t *data) {
  return (char *) data -> ptrval;
}

/*
 * --------------------------------------------------------------------------
 * Int
 * --------------------------------------------------------------------------
 */

data_t * _data_new_int(data_t *target, va_list arg) {
  long val;

  val = va_arg(arg, long);
  target -> intval = val;
  return target;
}

unsigned int _data_hash_int(data_t *data) {
  return hash(&(data -> intval), sizeof(long));
}

int _data_cmp_int(data_t *d1, data_t *d2) {
  return d1 -> intval - d2 -> intval;
}

char * _data_tostring_int(data_t *data) {
  return itoa(data -> intval);
}

data_t * _data_parse_int(char *str) {
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

data_t * _data_add_int(data_t *d1, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     type = Int;
  int     ix;
  long    intret = 0;
  double  fltret = 0.0;
  double  val;
  int     plus = (name[0] == '+');

  if (!args || !array_size(args)) {
    return data_error(ErrorArgCount, "int(+) requires at least two parameters");
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
                     ((plus)
                       ? "Cannot add integers and objects of type '%s'"
                       : "Cannot subtract integers and objects of type '%s'"),
                     descriptors[type].typecode);
  } else {
    if (type == Float) {
      fltret = (data_type(d1) == Int) ? (double) d1 -> intval : d1 -> dblval;
    } else {
      intret = d1 -> intval;
    }
    for (ix = 0; ix < array_size(args); ix++) {
      d = (data_t *) array_get(args, ix);
      switch(type) {
        case Int:
          /* Type of d must be Int, can't be Float */
          intret += (plus) ? d -> intval : (-(d -> intval));
          break;
        case Float:
          /* Here type of d can be Int or Float */
          val = (data_type(d) == Int) ? (double) d -> intval : d -> dblval;
          fltret += (plus) ? val : -val;
          break;
      }
    }
    ret = (type == Int)
        ? data_create_int(intret)
        : data_create_float(fltret);
  }
  return ret;
}

data_t * _data_mult_int(data_t *d1, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     type = Int;
  int     ix;
  long    intret = 0;
  double  fltret = 0.0;
  int     plus = (name[0] == '+');

  if (array_size(args) != 1) {
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
                     descriptors[type].typecode);
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
    ret = (type == Int)
        ? data_create_int(intret)
        : data_create_float(fltret);
  }
  return ret;
}

/*
 * --------------------------------------------------------------------------
 * Float
 * --------------------------------------------------------------------------
 */

data_t * _data_new_float(data_t *target, va_list arg) {
  double val;

  val = va_arg(arg, double);
  target -> dblval = val;
  return target;
}

unsigned int _data_hash_float(data_t *data) {
  return hash(&(data -> dblval), sizeof(double));
}

int _data_cmp_float(data_t *d1, data_t *d2) {
  return (d1 -> dblval == d2 -> dblval)
      ? 0
      : (d1 -> dblval > d2 -> dblval) ? 1 : -1;
}

char * _data_tostring_float(data_t *data) {
  return dtoa(data -> dblval);
}

data_t * _data_parse_float(char *str) {
  char   *endptr;
  double  val;

  val = strtod(str, &endptr);
  return ((*endptr == 0) || (isspace(*endptr)))
      ? data_create_float(val)
      : NULL;
}

data_t * _data_add_float(data_t *d1, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     ix;
  int     type = Float;
  double  retval = 0.0;
  double  val;
  int     plus = (name[0] == '+');

  if (array_size(args) != 1) {
    return data_error(ErrorArgCount, "float(+) requires at least two parameters");
  }

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
                     descriptors[type].typecode);
  } else {
    retval = d1 -> dblval;
    for (ix = 0; ix < array_size(args); ix++) {
      d = (data_t *) array_get(args, ix);
      val = (data_type(d) == Int) ? (double) d -> intval : d -> dblval;
      retval += (plus) ? val : -val;
    }
    ret = data_create_float(retval);
  }
  return ret;
}

data_t * _data_mult_float(data_t *d1, char *name, array_t *args, dict_t *kwargs) {
  data_t *d;
  data_t *ret;
  int     ix;
  int     type = Float;
  double  retval = 0.0;

  if (array_size(args) != 1) {
    return data_error(ErrorArgCount, "float(+) requires at least two parameters");
  }

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
                     descriptors[type].typecode);
  } else {
    retval = d1 -> dblval;
    for (ix = 0; ix < array_size(args); ix++) {
      d = (data_t *) array_get(args, ix);
      retval *= (data_type(d) == Int) ? (double) d -> intval : d -> dblval;
    }
    ret = data_create(type, retval);
  }
  return ret;
}

/*
 * --------------------------------------------------------------------------
 * Bool
 * --------------------------------------------------------------------------
 */

char * _data_tostring_bool(data_t *data) {
  return btoa(data -> intval);
}

data_t * _data_parse_bool(char *str) {
  return data_create_bool(atob(str));
}


/*
 * --------------------------------------------------------------------------
 * List
 * --------------------------------------------------------------------------
 */

data_t * _data_new_list(data_t *target, va_list arg) {
  list_t *list;
  int     count;
  int     ix;
  data_t *elem;

  list = data_list_create();
  target -> ptrval = list;
  count = va_arg(arg, int);
  for (ix = 0; ix < count; ix++) {
    elem = va_arg(arg, data_t *);
    list_append(list, elem);
  }
  return target;
}

int _data_cmp_list(data_t *d1, data_t *d2) {
  list_t         *l1;
  list_t         *l2;
  listiterator_t *iter1;
  listiterator_t *iter2;
  data_t         *e1;
  data_t         *e2;
  int             cmp;

  l1 = d1 -> ptrval;
  l2 = d2 -> ptrval;
  if (list_size(l1) != list_size(l2)) {
    return list_size(l1) - list_size(l2);
  }
  iter1 = li_create(l1);
  iter2 = li_create(l2);
  while (li_has_next(iter1)) {
    e1 = li_next(iter1);
    e2 = li_next(iter2);
    cmp = data_cmp(e1, e2);
    if (!cmp) {
      break;
    }
  }
  li_free(iter1);
  li_free(iter2);
  return cmp;
}

data_t * _data_copy_list(data_t *dest, data_t *src) {
  list_t         *dest_list;
  list_t         *src_list;
  listiterator_t *iter;
  data_t         *data;

  dest_list = dest -> ptrval;
  src_list = src -> ptrval;
  for (iter = li_create(src_list); li_has_next(iter); ) {
    data = (data_t *) li_next(iter);
    list_push(dest_list, data_copy(data));
  }
  return dest;
}

char * _data_tostring_list(data_t *data) {
  list_t         *list;
  str_t          *s;
  static char     buf[1024];

  list = data -> ptrval;
  s = list_tostr(list);
  strncpy(buf, str_chars(s), 1023);
  buf[1023] = 0;
  return buf;
}

unsigned int _data_hash_list(data_t *data) {
  return list_hash(data -> ptrval);
}

data_t * _data_list_len(data_t *d1, char *name, array_t *args, dict_t *kwargs) {
  return data_create_int(list_size((list_t *) d1 -> ptrval));
}

/*
 * --------------------------------------------------------------------------
 * Function
 * --------------------------------------------------------------------------
 */

data_t * _data_new_function(data_t *target, va_list arg) {
  function_t *fnc;

  fnc = va_arg(arg, function_t *);
  target -> ptrval = function_copy(fnc);
  return target;
}

data_t * _data_copy_function(data_t *target, data_t *src) {
  target -> ptrval = function_copy(src -> ptrval);
  return target;
}

int _data_cmp_function(data_t *d1, data_t *d2) {
  function_t *fnc1;
  function_t *fnc2;

  fnc1 = d1 -> ptrval;
  fnc2 = d2 -> ptrval;
  return (int) ((long) fnc1 -> fnc) - ((long) fnc2 -> fnc);
}

char * _data_tostring_function(data_t *data) {
  function_t     *fnc;

  fnc = data -> ptrval;
  return fnc -> name;
}

unsigned int _data_hash_function(data_t *data) {
  function_t     *fnc;

  fnc = data -> ptrval;
  return hashptr(fnc -> fnc);
}

/*
 * typedescr_t public functions
 */

int typedescr_register(typedescr_t *descr) {
  typedescr_t *new_descriptors;
  int          newsz;
  int          cursz;

  if (descr -> type <= 0) {
    descr -> type = data_numtypes;
  }
  assert((descr -> type >= data_numtypes) || descriptors[descr -> type].type == 0);
  if (descr -> type >= data_numtypes) {
    newsz = (descr -> type + 1) * sizeof(typedescr_t);
    cursz = data_numtypes * sizeof(typedescr_t);
    if (descriptors == builtins) {
      new_descriptors = (typedescr_t *) new(newsz);
      memcpy(new_descriptors, descriptors, cursz);
    } else {
      new_descriptors = (typedescr_t *) resize_block(descriptors, newsz, cursz);
    }
    descriptors = new_descriptors;
    data_numtypes = descr -> type + 1;
  }
  memcpy(descriptors + descr -> type * sizeof(typedescr_t),
         descr, sizeof(typedescr_t));
  return descr -> type;
}

typedescr_t * typedescr_get(int datatype) {
  if ((datatype >= 0) && (datatype < data_numtypes)) {
    return &descriptors[datatype];
  } else {
    return NULL;
  }
}

typedescr_t *  typedescr_add_method(typedescr_t *descr, char *name, method_t method) {
  if (!descr -> methods) {
    descr -> methods = strvoid_dict_create();
  }
  dict_put(descr -> methods, strdup(name), method);
  return descr;
}

method_t typedescr_get_method(typedescr_t *descr, char *name) {
  if (!descr -> methods) {
    descr -> methods = strvoid_dict_create();
  }
  return (method_t) dict_get(descr -> methods, name);
}


/*
 * data_t static functions
 */

static int _types_initialized = 0;

void _data_initialize_types(void) {
  if (!_types_initialized) {
    typedescr_add_method(typedescr_get(Int), "+", _data_add_int);
    typedescr_add_method(typedescr_get(Int), "-", _data_add_int);
    typedescr_add_method(typedescr_get(Int), "*", _data_mult_int);
    typedescr_add_method(typedescr_get(Float), "+", _data_add_float);
    typedescr_add_method(typedescr_get(Float), "-", _data_add_float);
    typedescr_add_method(typedescr_get(Float), "*", _data_mult_float);
    typedescr_add_method(typedescr_get(List), "len", _data_list_len);
    _types_initialized = 1;
  }
}

data_t * _data_create(datatype_t type) {
  data_t *ret;

  _data_initialize_types();
  ret = NEW(data_t);
  ret -> type = type;
  ret -> refs = 1;
  ret -> str = NULL;
#ifndef NDEBUG
  data_count++;
  ret -> debugstr = NULL;
#endif
  return ret;
}


/*
 * data_t public functions
 */

data_t * data_create(int type, ...) {
  va_list  arg;
  int      proxy;
  data_t  *ret;
  void    *value;

  if (descriptors[type].new) {
    ret = _data_create(type);
    va_start(arg, type);
    ret = descriptors[type].new(ret, arg);
    va_end(arg);
  } else {
    ret = NULL;
  }
  return ret;
}

data_t * data_create_pointer(void *ptr) {
  data_t *ret;

  ret = _data_create(Pointer);
  ret -> ptrval = ptr;
  return ret;
}

data_t * data_null(void) {
  return data_create_pointer(NULL);
}

data_t * data_error(int code, char * msg, ...) {
  char    buf[256];
  char   *ptr;
  int     size;
  va_list args;
  va_list args_copy;
  data_t *ret;

  va_start(args, msg);
  va_copy(args_copy, args);
  size = vsnprintf(buf, 256, msg, args);
  if (size > 256) {
    ptr = (char *) new(size);
    vsprintf(ptr, msg, args_copy);
  } else {
    ptr = buf;
  }
  va_end(args);
  va_end(args_copy);
  ret = data_create(Error, code, ptr);
  if (size > 256) {
    free(ptr);
  }
  return ret;
}

data_t * data_create_int(long value) {
  return data_create(Int, value);
}

data_t * data_create_float(double value) {
  return data_create(Float, value);
}

data_t * data_create_bool(long value) {
  data_t *ret;

  ret = _data_create(Bool);
  ret -> intval = value ? 1 : 0;
  return ret;
}

data_t * data_create_string(char * value) {
  data_t *ret;

  debug(" <-- %s", value);
  ret = _data_create(String);
  ret -> ptrval = strdup(value);
  debug(" --> %d", data_tostring(ret));
  return ret;
}

data_t * data_create_list(list_t *list) {
  data_t *ret;
  list_t *l;

  ret = _data_create(List);
  l = data_list_create();
  list_add_all(l, list);
  ret -> ptrval = l;
  return ret;
}

data_t * data_create_list_fromarray(array_t *array) {
  data_t *ret;
  list_t *l;
  int     ix;

  ret = _data_create(List);
  l = data_list_create();
  for (ix = 0; ix < array_size(array); ix++) {
    list_append(l, data_copy((data_t *) array_get(array, ix)));
  }
  ret -> ptrval = l;
  return ret;
}

array_t * data_list_toarray(data_t *data) {
  listiterator_t *iter;
  array_t        *ret;

  ret = data_array_create(list_size((list_t *) data -> ptrval));
  for (iter = li_create((list_t *) data -> ptrval); li_has_next(iter); ) {
    array_push(ret, data_copy((data_t *) li_next(iter)));
  }
  return ret;
}

data_t * data_create_function(function_t *fnc) {
  data_t *ret;

  ret = _data_create(Function);
  ret -> ptrval = function_copy(fnc);
  return ret;
}

data_t * data_parse(int type, char *str) {
  debug(" == %d:%s -- %p", type, str, descriptors[type].parse);
  return (descriptors[type].parse) 
    ? descriptors[type].parse(str) 
    : NULL;
}

void data_free(data_t *data) {
  if (data) {
    data -> refs--;
    if (data -> refs <= 0) {
      if (descriptors[data -> type].free) {
	descriptors[data -> type].free(data -> ptrval);
      }
      free(data -> str);
#ifndef NDEBUG
      free(data -> debugstr);
      data_count--;
#endif
      free(data);
    }
  }
}

int data_type(data_t *data) {
  return data -> type;
}

int data_is_numeric(data_t *data) {
  return (data -> type == Int) || (data -> type == Float);
}

int data_is_error(data_t *data) {
  return (!data || (data -> type == Error));
}

data_t * data_copy(data_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
#if 0
  data_t *ret;

  ret = NEW(data_t);
  if (ret) {
    memcpy(ret, src, sizeof(data_t));
    if (descriptors[src -> type].copy) {
      descriptors[src -> type].copy(ret, src);
    }
  }
  return ret;
#endif
}

method_t data_method(data_t *data, char *name) {
  return typedescr_get_method(typedescr_get(data -> type), name);
}

data_t * data_execute(data_t *data, char *name, array_t *args, dict_t *kwargs) {
  method_t  m;
  data_t   *ret;
  array_t  *args_shifted = NULL;

  if (!data && array_size(args)) {
    data = (data_t *) array_get(args, 0);
    args_shifted = array_slice(args, 1, -1);
    if (!args_shifted) {
      args_shifted = data_array_create(1);
    }
  }

  m = data_method(data, name);
  if (!m) {
    m = typedescr_get(data -> type) -> fallback;
  }
  if (m) {
    ret = m(data, name, (args_shifted) ? args_shifted : args, kwargs);
    array_free(args_shifted);
  } else {
    ret = data_error(ErrorName, "data object '%s' has no method '%s", data_tostring(data), name);
  }
  return ret;
}

unsigned int data_hash(data_t *data) {
  return (descriptors[data -> type].hash)
    ? descriptors[data -> type].hash(data)
    : hashptr(data);
}

char * data_tostring(data_t *data) {
  char  buf[20];
  char *ret;

  free(data -> str);
  if (!data) {
    ret = "data:NULL";
  }
  if (descriptors[data -> type].tostring) {
    ret =  descriptors[data -> type].tostring(data);
  } else {
    snprintf(buf, 20, "data:%s:%p", descriptors[data -> type].typecode, data);
    ret = buf;
  }
  data -> str = strdup(ret);
  return data -> str;
}

char * data_debugstr(data_t *data) {
#ifndef NDEBUG
  int         len;

  free(data -> debugstr);
  len = snprintf(NULL, 0, "%c %s", 
		 descriptors[data -> type].typecode[0], 
		 data_tostring(data));
  data -> debugstr = (char *) new(len + 1);
  sprintf(data -> debugstr, "%c %s", 
	  descriptors[data -> type].typecode[0], 
	  data_tostring(data));
  return data -> debugstr;
#else
  return data_tostring(data);
#endif
}

int data_cmp(data_t *d1, data_t *d2) {
  if (!d1 && !d2) {
    return 0;
  } else if (!d1 || !d2) {
    return 1;
  } else if (d1 -> type != d2 -> type) {
    return 1;
  } else {
    return descriptors[d1 -> type].cmp(d1, d2);
  }
}

dict_t * data_add_all_reducer(entry_t *e, dict_t *target) {
  dict_put(target, strdup(e -> key), data_copy(e -> value));
  return target;
}

