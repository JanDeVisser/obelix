/*
 * /obelix/src/types/string.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <error.h>

static data_t *     _string_new(data_t *, va_list);
static data_t *     _string_copy(data_t *, data_t *);
static int          _string_cmp(data_t *, data_t *);
static unsigned int _string_hash(data_t *);
static char *       _string_tostring(data_t *);
static data_t *     _string_cast(data_t *, int);

static data_t * _string_len(data_t *, char *, array_t *, dict_t *);
static data_t * _string_slice(data_t *, char *, array_t *, dict_t *);
static data_t * _string_at(data_t *, char *, array_t *, dict_t *);
static data_t * _string_forcecase(data_t *, char *, array_t *, dict_t *);
static data_t * _string_has(data_t *, char *, array_t *, dict_t *);
static data_t * _string_indexof(data_t *, char *, array_t *, dict_t *);
static data_t * _string_rindexof(data_t *, char *, array_t *, dict_t *);
static data_t * _string_startswith(data_t *, char *, array_t *, dict_t *);
static data_t * _string_endswith(data_t *, char *, array_t *, dict_t *);


typedescr_t typedescr_string =   {
  type:                  String,
  typecode:              "S",
  typename:              "str",
  new:      (new_t)      _string_new,
  copy:     (copydata_t) _string_copy,
  cmp:      (cmp_t)      _string_cmp,
  free:     (free_t)     free,
  tostring: (tostring_t) _string_tostring,
  parse:    (parse_t)    data_create_string,
  cast:     (cast_t)     _string_cast,
  hash:     (hash_t)     _string_hash
};

methoddescr_t methoddescr_string[] = {
  { type: String, name: "len",        method: _string_len,        min_args: 1, max_args: 1  },
  { type: String, name: "at",         method: _string_at,         min_args: 2, max_args: 2  },
  { type: String, name: "slice",      method: _string_slice,      min_args: 2, max_args: 3  },
  { type: String, name: "upper",      method: _string_forcecase,  min_args: 1, max_args: 1  },
  { type: String, name: "lower",      method: _string_forcecase,  min_args: 1, max_args: 1  },
  { type: String, name: "has",        method: _string_has,        min_args: 2, max_args: 2  },
  { type: String, name: "indexof",    method: _string_indexof,    min_args: 2, max_args: 2  },
  { type: String, name: "rindexof",   method: _string_indexof,    min_args: 2, max_args: 2  },
  { type: String, name: "startswith", method: _string_startswith, min_args: 2, max_args: 2  },
  { type: String, name: "endswith",   method: _string_endswith,   min_args: 2, max_args: 2  },
  { type: -1,     name: NULL,         method: NULL,               min_args: 0, max_args: 0  },
};

/*
 * --------------------------------------------------------------------------
 * String
 * --------------------------------------------------------------------------
 */

data_t * _string_new(data_t *target, va_list arg) {
  char *str;

  str = va_arg(arg, char *);
  target -> ptrval = str ? strdup(str) : NULL;
  return target;
}

unsigned int _string_hash(data_t *data) {
  return strhash(data -> ptrval);
}

data_t * _string_copy(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval ? strdup(src -> ptrval) : NULL;
  return target;
}

int _string_cmp(data_t *d1, data_t *d2) {
  return strcmp((char *) d1 -> ptrval, (char *) d2 -> ptrval);
}

char * _string_tostring(data_t *data) {
  return (char *) data -> ptrval;
}

data_t * _string_cast(data_t *data, int totype) {
  typedescr_t *type = typedescr_get(totype);
  
  return (type -> parse) ? type -> parse(data -> ptrval) : NULL;
}

/* ----------------------------------------------------------------------- */

data_t * _string_len(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, strlen(self -> ptrval));
}

data_t * _string_at(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *ix = (data_t *) array_get(args, 0);
  int     i;
  char    buf[2];

  if (data_type(ix) != Int) {
    return data_error(ErrorType, "%s.%s expects an int argument",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    i = ix -> intval;
    if ((i < 0) || (i >= strlen(self -> ptrval))) {
      return data_error(ErrorRange, "%s.%s argument out of range: %d not in [0..%d]",
                                     typedescr_get(data_type(self)) -> typename,
                                     name,
                                     i, strlen(self -> ptrval) - 1);
    } else {
      buf[0] = ((char *) self -> ptrval)[i];
      buf[1] = 0;
      return data_create(String, buf);
    }
  }
}

data_t * _string_slice(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *from = (data_t *) array_get(args, 0);
  data_t *to = (data_t *) array_get(args, 0);
  int     len = strlen(self -> ptrval);
  int     i;
  int     j;
  char    buf[len + 1];

  if ((data_type(from) != Int) || (data_type(to) != Int)) {
    return data_error(ErrorType, "%s.%s expects an two int arguments",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    i = from -> intval;
    j = to -> intval;
    if (j < 0) {
      j = len + j;
    }
    if (i < 0) {
      i = len + i;
    }
    if ((i < 0) || (i >= len)) {
      return data_error(ErrorRange, "%s.%s argument out of range: %d not in [0..%d]",
                                    typedescr_get(data_type(self)) -> typename,
                                    name,
                                    i, len - 1);
    } else if ((j <= i) || (j > len)) {
      return data_error(ErrorRange, "%s.%s argument out of range: %d not in [%d..%d]",
                                    typedescr_get(data_type(self)) -> typename,
                                    name,
                                    j, i+1, len);
    } else {
      // FIXME --- Or at least check me.
      strncpy(buf, ((char *) self -> ptrval) + i, j - i);
      buf[j-i] = 0;
      return data_create(String, buf);
    }
  }
}

data_t * _string_forcecase(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int  upper = name[0] == 'u';
  int  len = strlen(self -> ptrval);
  char buf[len + 1];
  int  ix;
  int  c;

  for (ix = 0; ix < len; ix++) {
    c = ((char *) self -> ptrval)[ix];
    buf[ix] = upper ? toupper(c) : tolower(c);
  }
  return data_create(String, buf);
}

data_t * _string_has(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *needle = (data_t *) array_get(args, 0);

  if (data_type(needle) != String) {
    return data_error(ErrorType, "%s.%s expects a string argument",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    return data_create(Bool, strstr(self -> ptrval, needle -> ptrval) != NULL);
  }
}

data_t * _string_indexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *needle = (data_t *) array_get(args, 0);
  char   *pos;
  int     diff;

  if (data_type(needle) != String) {
    return data_error(ErrorType, "%s.%s expects a string argument",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    pos = strstr(self -> ptrval, needle -> ptrval);
    diff = (pos) ? pos - (char *) self -> ptrval : -1;
    return data_create(Int, diff);
  }
}

data_t * _string_rindexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *needle = (data_t *) array_get(args, 0);
  char   *needle_str;
  int     needle_len;
  char   *pos = NULL;
  char   *next;
  int     diff;

  if (data_type(needle) != String) {
    return data_error(ErrorType, "%s.%s expects a string argument",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    needle_str = (char *) needle -> ptrval;
    needle_len = strlen(needle_str);
    pos = NULL;
    if (strlen(self -> ptrval) >= needle_len) {
      next = strstr((char *) self -> ptrval, needle_str);
      while (next) {
        pos = next;
        if (strlen(next) > needle_len) {
          next = strstr(next + 1, needle_str);
        } else {
          next = NULL;
        }
      }
    }
    diff = (pos) ? pos - (char *) self -> ptrval : -1;
    return data_create(Int, diff);    
  }
}

data_t * _string_startswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *prefix = (data_t *) array_get(args, 0);
  int     len;
  int     prflen;
  
  if (data_type(prefix) != String) {
    return data_error(ErrorType, "%s.%s expects a string argument",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    len = strlen(self -> ptrval);
    prflen = strlen(prefix -> ptrval);
    if (strlen(prefix -> ptrval) > strlen(self -> ptrval)) {
      return data_create(Bool, 0);
    } else {
      return data_create(Bool, strncmp(prefix -> ptrval, self -> ptrval, prflen) == 0);
    }
  }
}

data_t * _string_endswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *suffix = (data_t *) array_get(args, 0);
  int     len;
  int     suflen;
  char   *ptr;
  
  if (data_type(suffix) != String) {
    return data_error(ErrorType, "%s.%s expects a string argument",
                                  typedescr_get(data_type(self)) -> typename,
                                  name);
  } else {
    len = strlen(self -> ptrval);
    suflen = strlen(suffix -> ptrval);
    if (suflen > len) {
      return data_create(Bool, 0);
    } else {
      ptr = ((char *) self -> ptrval) + (len - suflen);
      return data_create(Bool, strncmp(suffix -> ptrval, ptr, suflen) == 0);
    }
  }
}

