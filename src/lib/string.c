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
#include <exception.h>

static void         _string_init(void) __attribute__((constructor));
static data_t *     _string_new(data_t *, va_list);
static data_t *     _string_copy(data_t *, data_t *);
static int          _string_cmp(data_t *, data_t *);
static unsigned int _string_hash(data_t *);
static char *       _string_tostring(data_t *);
static data_t *     _string_parse(typedescr_t *, char *);
static data_t *     _string_cast(data_t *, int);
static data_t *     _string_resolve(data_t *, char *);
static int          _string_len(data_t *);
static int          _string_read(data_t *, char *, int);

static data_t *     _string_format(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_slice(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_at(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_forcecase(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_has(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_indexof(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_rindexof(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_startswith(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_endswith(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_concat(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_repeat(data_t *, char *, array_t *, dict_t *);
static data_t *     _string_split(data_t *, char *, array_t *, dict_t *);

vtable_t _vtable_string[] = {
  { .id = FunctionNew,      .fnc = (void_t) _string_new },
  { .id = FunctionCopy,     .fnc = (void_t) _string_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _string_cmp },
  { .id = FunctionToString, .fnc = (void_t) _string_tostring },
  { .id = FunctionParse,    .fnc = (void_t) _string_parse },
  { .id = FunctionCast,     .fnc = (void_t) _string_cast },
  { .id = FunctionHash,     .fnc = (void_t) _string_hash },
  { .id = FunctionLen,      .fnc = (void_t) _string_len },
  { .id = FunctionResolve,  .fnc = (void_t) _string_resolve },
  { .id = FunctionFree,     .fnc = (void_t) free },
  { .id = FunctionRead,     .fnc = (void_t) _string_read },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_str = {
  .type =        String,
  .type_name =   "str",
  .vtable =      _vtable_string
};

static methoddescr_t _methoddescr_str[] = {
  { .type = String, .name = "format",     .method = _string_format,     .argtypes = { Any, NoType, NoType },    .minargs = 0, .varargs = 1 },
  { .type = String, .name = "at",         .method = _string_at,         .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = String, .name = "slice",      .method = _string_slice,      .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 1 },
  { .type = String, .name = "upper",      .method = _string_forcecase,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = String, .name = "lower",      .method = _string_forcecase,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = String, .name = "has",        .method = _string_has,        .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = String, .name = "indexof",    .method = _string_indexof,    .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = String, .name = "rindexof",   .method = _string_indexof,    .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = String, .name = "startswith", .method = _string_startswith, .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = String, .name = "endswith",   .method = _string_endswith,   .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = String, .name = "+",          .method = _string_concat,     .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = String, .name = "concat",     .method = _string_concat,     .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = String, .name = "*",          .method = _string_repeat,     .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = String, .name = "repeat",     .method = _string_repeat,     .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = String, .name = "split",      .method = _string_split,      .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,         .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/*
 * --------------------------------------------------------------------------
 * String
 * --------------------------------------------------------------------------
 */

void _string_init(void) {
  typedescr_register(&_typedescr_str);
  typedescr_register_methods(_methoddescr_str);
}

data_t * _string_new(data_t *target, va_list arg) {
  char *str;

  str = va_arg(arg, char *);
  target -> ptrval = str ? strdup(str) : NULL;
  return target;
}

unsigned int _string_hash(data_t *data) {
  return strhash(data -> ptrval);
}

int _string_len(data_t *self) {
  return strlen(data_charval(self));
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

data_t * _string_parse(typedescr_t *type, char *str) {
  return data_create(type -> type, str);
}

data_t * _string_cast(data_t *data, int totype) {
  typedescr_t *type;
  parse_t      parse;
  char        *str = data_charval(data);
  
  if (totype == Bool) {
    return data_create(Bool, str && strlen(str));
  } else {
    type = typedescr_get(totype);
    assert(type);
    parse = (parse_t) typedescr_get_function(type, FunctionParse);
    return (parse) ? parse(data -> ptrval) : NULL;
  }
}

data_t * _string_resolve(data_t *data, char *slice) {
  char *str = data_charval(data);
  int   sz = strlen(str);
  long  ix;
  char  buf[2];
  
  if (!strtoint(slice, &ix)) {
    if ((ix >= sz) || (ix < -sz)) {
      return data_exception(ErrorRange, 
                            "Index %d is not in range %d ~ %d", 
                            ix, -sz, sz - 1);
    } else {
      if (ix < 0) {
        ix = sz + ix;
      }
      buf[0] = str[ix];
      buf[1] = 0;
      return data_create(String, buf);
    }
  } else {
    return NULL;
  }
}

int _string_read(data_t *data, char *buf, int num) {
  //
}

data_t * data_create_string(char * value) {
  return data_create(String, value);
}

/* ----------------------------------------------------------------------- */

data_t * _string_format(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  str_t  *s;
  data_t *ret;

  s = format(data_tostring(self), args, kwargs);
  ret = data_create(String, str_chars(s));
  str_free(s);
  return ret;
}

data_t * _string_at(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) kwargs;
  return _string_resolve(self, data_tostring(data_array_get(args, 0)));
}

data_t * _string_slice(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *from = (data_t *) array_get(args, 0);
  data_t *to = (data_t *) array_get(args, 1);
  int     len = strlen(data_charval(self));
  int     i;
  int     j;
  char    buf[len + 1];

  /* FIXME: second argument (to) is optional; ommiting it gives you the tail */
  i = data_intval(from);
  j = data_intval(to);
  if (j <= 0) {
    j = len + j;
  }
  if (i < 0) {
    i = len + i;
  }
  if ((i < 0) || (i >= len)) {
    return data_exception(ErrorRange, "%s.%s argument out of range: %d not in [0..%d]",
                                  typedescr_get(data_type(self)) -> type_name,
                                  name,
                                  i, len - 1);
  } else if ((j <= i) || (j > len)) {
    return data_exception(ErrorRange, "%s.%s argument out of range: %d not in [%d..%d]",
                                  typedescr_get(data_type(self)) -> type_name,
                                  name,
                                  j, i+1, len);
  } else {
    // FIXME --- Or at least check me.
    strncpy(buf, data_charval(self) + i, j - i);
    buf[j-i] = 0;
    return data_create(String, buf);
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

  return data_create(Bool, strstr(self -> ptrval, needle -> ptrval) != NULL);
}

data_t * _string_indexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *needle = (data_t *) array_get(args, 0);
  char   *pos;
  int     diff;

  pos = strstr(self -> ptrval, needle -> ptrval);
  diff = (pos) ? pos - (char *) self -> ptrval : -1;
  return data_create(Int, diff);
}

data_t * _string_rindexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *needle = (data_t *) array_get(args, 0);
  char   *needle_str;
  int     needle_len;
  char   *pos = NULL;
  char   *next;
  int     diff;

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

data_t * _string_startswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *prefix = (data_t *) array_get(args, 0);
  int     len;
  int     prflen;
  
  len = strlen(self -> ptrval);
  prflen = strlen(prefix -> ptrval);
  if (strlen(prefix -> ptrval) > strlen(self -> ptrval)) {
    return data_create(Bool, 0);
  } else {
    return data_create(Bool, strncmp(prefix -> ptrval, self -> ptrval, prflen) == 0);
  }
}

data_t * _string_endswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *suffix = (data_t *) array_get(args, 0);
  int     len;
  int     suflen;
  char   *ptr;
  
  len = strlen(self -> ptrval);
  suflen = strlen(suffix -> ptrval);
  if (suflen > len) {
    return data_create(Bool, 0);
  } else {
    ptr = ((char *) self -> ptrval) + (len - suflen);
    return data_create(Bool, strncmp(suffix -> ptrval, ptr, suflen) == 0);
  }
}

data_t * _string_concat(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     len = strlen((char *) self -> ptrval);
  int     ix;
  data_t *d;
  char   *buf;
  
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    len += strlen((char *) d -> ptrval);
  }
  buf = (char *) new(len + 1);
  strcpy(buf, (char *) self -> ptrval);
  for (ix = 0; ix < array_size(args); ix++) {
    d = (data_t *) array_get(args, ix);
    strcat(buf, (char *) d -> ptrval);
  }
  d = data_create(String, buf);
  free(buf);
  return d;
}

data_t * _string_repeat(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     len = strlen((char *) self -> ptrval);
  data_t *num = (data_t *) array_get(args, 0);
  int     numval = data_intval(num);
  int     ix;
  char   *buf;
  data_t *ret;
  
  len *= numval;
  if (len < 0) {
    buf = strdup("");
  } else {
    buf = (char *) new(len + 1);
    for (ix = 0; ix < numval; ix++) {
      strcat(buf, data_charval(self));
    }
  }
  ret = data_create(String, buf);
  free(buf);
  return ret;
}

data_t * _string_split(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  array_t *split = array_split(data_charval(self), data_charval(data_array_get(args, 0)));
  data_t  *ret = data_str_array_to_list(split);
  
  array_free(split);
  return ret;
}

str_t * format(char *fmt, array_t *args, dict_t *kwargs) {
  char  *ptr;
  char  *specstart;
  long   ix;
  str_t *ret = str_create(strlen(fmt));
  char   buf[8];
  int    bufsize = 8;
  char  *bigbuf = NULL;
  char  *spec = buf;
  int    len;
  
  for (ptr = fmt; *ptr; ptr++) {
    if (!strncmp(ptr, "${", 2)) {
      ptr += 2;
      specstart = ptr;
      for (; *ptr && (*ptr != '}'); ptr++);
      len = ptr - specstart;
      if (!*ptr) {
        str_append_nchars(ret, specstart, len);
      } else {
        while (len >= bufsize - 1) {
          bufsize *= 2;
        }
        if (bufsize > 8) {
          bigbuf = (char *) new(bufsize);
          spec = bigbuf;
        }
        strncpy(spec, specstart, len);
        spec[len] = 0;
        if (kwargs && dict_has_key(kwargs, spec)) {
          str_append_chars(ret, data_tostring(data_dict_get(kwargs, spec)));
        } else {
          if (!strtoint(spec, &ix) && (ix >= 0) && (ix < array_size(args))) {
            str_append_chars(ret, data_tostring(data_array_get(args, ix)));
          } else {
            str_append_chars(ret, "${%s}", spec);
          }
        }
      }
    } else {
      str_append_char(ret, *ptr);
    }
  }
  if (bigbuf) free(bigbuf);
  return ret;
}

