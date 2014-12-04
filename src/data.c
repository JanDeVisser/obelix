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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <list.h>

static data_t *      _data_create(datatype_t);

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

static data_t *      _data_new_function(data_t *, va_list);
static data_t *      _data_copy_function(data_t *, data_t *);
static int           _data_cmp_function(data_t *, data_t *);
static char *        _data_tostring_function(data_t *);
/* static data_t *      _data_parse_function(char *); */
static unsigned int  _data_hash_function(data_t *);

// -----------------------
// Data conversion support

int data_numtypes = Function + 1;
static typedescr_t builtins[] = {
  {
    type:                  Pointer,
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

/*
 * data_t static functions
 */

data_t * _data_create(datatype_t type) {
  data_t *ret;

  ret = NEW(data_t);
  ret -> type = type;
  return ret;
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
    snprintf(buf, 32, "<<pointer %p>>", (void *) data -> ptrval);
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

  list = list_create();
  list_set_free(list, data_free);
  list_set_tostring(list, data_tostring);
  list_set_hash(list, data_hash);
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

  dest_list = dest -> ptrval;
  src_list = src -> ptrval;
  list_add_all(dest_list, src_list);
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
 * data_t public functions
 */

int datatype_register(typedescr_t *descr) {
  typedescr_t *new_descriptors;
  int          newsz;
  int          cursz;

  if (descr -> type <= 0) {
    descr -> type = data_numtypes;
  }
  assert((descr -> type >= data_numtypes) || descriptors[descr -> type].type == 0);
  if (descr -> type >= data_numtypes) {
    newsz = (descr -> type + 1) * sizeof(typedescr_t);
    cursz = (data_numtypes - 1) * sizeof(typedescr_t);
    if (descriptors == builtins) {
      new_descriptors = (typedescr_t *) new(newsz);
      memcpy(new_descriptors, descriptors, cursz);
    } else {
      new_descriptors = (typedescr_t *) resize_block(descriptors, newsz, cursz);
    }
    descriptors = new_descriptors;
    data_numtypes = descr -> type + 1;
  }
  memcpy(descriptors + descr -> type, descr, sizeof(typedescr_t));
  return descr -> type;
}

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

data_t * data_create_int(long value) {
  data_t *ret;

  ret = _data_create(Int);
  ret -> intval = value;
  return ret;
}

data_t * data_create_float(double value) {
  data_t *ret;

  ret = _data_create(Float);
  ret -> dblval = value;
  return ret;
}

data_t * data_create_bool(long value) {
  data_t *ret;

  ret = _data_create(Bool);
  ret -> intval = value ? 1 : 0;
  return ret;
}

data_t * data_create_string(char * value) {
  data_t *ret;

  ret = _data_create(String);
  ret -> ptrval = strdup(value);
  return ret;
}

data_t * data_create_function(function_t *fnc) {
  data_t *ret;

  ret = _data_create(Function);
  ret -> ptrval = function_copy(fnc);
  return ret;
}

data_t * data_parse(int type, char *str) {
  return (descriptors[type].parse) 
    ? descriptors[type].parse(str) 
    : NULL;
}

void data_free(data_t *data) {
  if (data) {
    if (descriptors[data -> type].free) {
      descriptors[data -> type].free(data -> ptrval);
    }
    free(data);
  }
}

int data_type(data_t *data) {
  return data -> type;
}

int data_is_numeric(data_t *data) {
  return (data -> type == Int) && (data -> type == Float);
}

data_t * data_copy(data_t *src) {
  data_t *ret;

  ret = NEW(data_t);
  if (ret) {
    memcpy(ret, src, sizeof(data_t));
    if (descriptors[src -> type].copy) {
      descriptors[src -> type].copy(ret, src);
    }
  }
  return ret;
}

unsigned int data_hash(data_t *data) {
  return (descriptors[data -> type].hash)
    ? descriptors[data -> type].hash(data)
    : hashptr(data);
}

char * data_tostring(data_t *data) {
  if (!data) {
    return "<<null>>";
  }
  if (descriptors[data -> type].tostring) {
    return descriptors[data -> type].tostring(data);
  } else {
    return "<< ?? >>";
  }
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



