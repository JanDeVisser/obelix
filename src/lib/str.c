/*
 * /obelix/src/str.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <array.h>
#include <data.h>
#include <exception.h>
#include <str.h>
#include <typedescr.h>

static void         _str_init(void) __attribute__((constructor));
static str_t *      _str_initialize(void);
static str_t *      _str_expand(str_t *, int);
static reduce_ctx * _str_join_reducer(char *, reduce_ctx *);

static data_t *     _str_create(int, va_list);
static void         _str_free(str_t *);
static data_t *     _str_cast(str_t *, int);
static str_t *      _str_parse(char *str);
static data_t *     _str_resolve(str_t *, char *);

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

#define _DEFAULT_SIZE   32

static vtable_t _vtable_string[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _str_create },
  { .id = FunctionCmp,      .fnc = (void_t) str_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _str_free },
  { .id = FunctionToString, .fnc = (void_t) str_chars },
  { .id = FunctionParse,    .fnc = (void_t) _str_parse },
  { .id = FunctionCast,     .fnc = (void_t) _str_cast },
  { .id = FunctionHash,     .fnc = (void_t) str_hash },
  { .id = FunctionLen,      .fnc = (void_t) str_len },
  { .id = FunctionRead,     .fnc = (void_t) str_read },
  { .id = FunctionResolve,  .fnc = (void_t) _str_resolve },
  { .id = FunctionNone,     .fnc = NULL }
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

/* ------------------------------------------------------------------------ */

void _str_init(void) {
  typedescr_create_and_register(String, "string", _vtable_string, _methoddescr_str);
}

/* -- S T R I N G   D A T A   F U N C T I O N S --------------------------- */

void _str_free(str_t *str) {
  if (str) {
    if (str -> bufsize) {
      free(str -> buffer);
    }
    free(str);
  }
}

data_t * _str_resolve(str_t *str, char *slice) {
  int   sz = str_len(str);
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
      buf[0] = str_at(str, ix);
      buf[1] = 0;
      return data_create(String, buf);
    }
  } else {
    return NULL;
  }
}

data_t * _str_cast(str_t *data, int totype) {
  typedescr_t *type;
  parse_t      parse;
  char        *str = str_chars(data);

  if (totype == Bool) {
    return data_create(Bool, str && strlen(str));
  } else {
    type = typedescr_get(totype);
    assert(type);
    parse = (parse_t) typedescr_get_function(type, FunctionParse);
    return (parse) ? parse(str_chars(data)) : NULL;
  }
}

str_t * _str_parse(char *str) {
  return str_copy_chars(str);
}

/* -- S T R _ T   S T A T I C   F U N C T I O N S ------------------------- */

str_t * _str_initialize(void) {
  str_t *ret = data_new(String, str_t);

  ret -> buffer = NULL;
  ret -> pos = 0;
  ret -> len = 0;
  ret -> bufsize = 0;
  return ret;
}

data_t * _str_create(int type, va_list args) {
  return (data_t *) str_copy_chars(va_arg(args, char *));
}

str_t * _str_expand(str_t *str, int targetlen) {
  int newsize;
  char *oldbuf;
  str_t *ret = str;

  if (str -> bufsize < (targetlen + 1)) {
    for (newsize = str -> bufsize * 2; newsize < (targetlen + 1); newsize *= 2);
    oldbuf = str -> buffer;
    str -> buffer = realloc(str -> buffer, newsize);
    if (str -> buffer) {
      memset(str -> buffer + str -> len, 0, newsize - str -> len);
      str -> bufsize = newsize;
    } else {
      str -> buffer = oldbuf;
      ret = NULL;
    }
  }
  return ret;
}

reduce_ctx * _str_join_reducer(char *elem, reduce_ctx *ctx) {
  str_t *target;
  char  *glue;

  target = (str_t *) ctx -> data;
  glue = (char *) ctx -> user;
  str_append_chars(target, elem);
  str_append_chars(target, glue);
  return ctx;
}


/* -- S T R _ T   P U B L I C   F U N C T I O N S ------------------------- */

str_t * str_create(int size) {
  str_t *ret;

  ret = _str_initialize();
  size = size ? size : _DEFAULT_SIZE;
  ret -> buffer = (char *) new(size);
  memset(ret -> buffer, 0, size);
  ret -> bufsize = size;
  return ret;
}

str_t * str_wrap(char *buffer) {
  str_t *ret;

  ret = _str_initialize();
  if (ret) {
    ret -> buffer = buffer;
    ret -> len = strlen(buffer);
  }
  return ret;
}

str_t * str_copy_chars(char *buffer, ...) {
  str_t   *ret;
  va_list  args;

  va_start(args, buffer);
  ret = str_copy_vchars(buffer, args);
  va_end(args);
  return ret;
}

str_t * str_copy_vchars(char *buffer, va_list args) {
  str_t   *ret;
  char    *b;
  va_list  args_copy;

  va_copy(args_copy, args);
  ret = _str_initialize();
  if (ret) {
    ret -> len = vsnprintf(NULL, 0, buffer, args);
    ret -> buffer = (char *) new(ret -> len + 1);
    vsprintf(ret -> buffer, buffer, args_copy);
    va_end(args_copy);
    ret -> bufsize = ret -> len + 1;
  }
  return ret;
}

str_t * str_copy_nchars(int len, char *buffer) {
  str_t *ret;
  char  *b;

  len = (len <= strlen(buffer)) ? len : strlen(buffer);
  ret = _str_initialize();
  b = (char *) new(len + 1);
  strncpy(b, buffer, len);
  ret -> buffer = b;
  ret -> len = len;
  ret -> bufsize = ret -> len + 1;
  return ret;
}

str_t * str_deepcopy(str_t *str) {
  str_t *ret;
  char  *b;

  ret = _str_initialize();
  if (ret) {
    b = strdup(str_chars(str));
    if (b) {
      ret -> buffer = b;
      ret -> len = strlen(b);
      ret -> bufsize = ret -> len + 1;
    } else {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

/**
 * Frees the str_t object, but does not free the actual underlying char buffer,
 * instead making that available for use by the caller. Note that the caller
 * now becomes responsible for freeing this buffer (if the str_t was not
 * obtained using a str_wrap).
 *
 * @param str String to free and return the buffer of.
 * @return char buffer of the str_t object passed in.
 */
char * str_reassign(str_t *str) {
  char *ret = str -> buffer;

  str -> bufsize = 0;
  free(str);
  return ret;
}

int str_len(str_t *str) {
  return str -> len;
}

char * str_chars(str_t *str) {
  return str -> buffer;
}

int str_at(str_t* str, int i) {
  if (i < 0) {
    i = str -> len + i;
  }
  return (i < str -> len) ? str -> buffer[i] : -1;
}

unsigned int str_hash(str_t *str) {
  return strhash(str -> buffer);
}

int str_cmp(str_t *s1, str_t *s2) {
  return strcmp(s1 -> buffer, s2 -> buffer);
}

int str_cmp_chars(str_t *s1, char *s2) {
  return strcmp(s1 -> buffer, s2);
}

int str_ncmp(str_t *s1, str_t *s2, int numchars) {
  return strncmp(s1 -> buffer, s2 -> buffer, numchars);
}

int str_ncmp_chars(str_t *s1, char *s2, int numchars) {
  return strncmp(s1 -> buffer, s2, numchars);
}

int str_indexof(str_t *str, str_t *pattern) {
  if (pattern -> len > str -> len) {
    return -1;
  } else {
    return str_indexof_chars(str, str_chars(pattern));
  }
}

int str_indexof_chars(str_t *str, char *pattern) {
  char *ptr;

  if (strlen(pattern) > str -> len) {
    return -1;
  } else {
    ptr = strstr(str_chars(str), pattern);
    return (ptr) ? ptr - str_chars(str) : -1;
  }
}

int str_rindexof(str_t *str, str_t *pattern) {
  return str_rindexof_chars(str, str_chars(pattern));
}

int str_rindexof_chars(str_t *str, char *pattern) {
  char *ptr;
  int   len;

  len = strlen(pattern);
  if (len > str -> len) {
    return -1;
  } else {
    for (ptr = str -> buffer + (str -> len - len); ptr != str -> buffer; ptr--) {
      if (!strncmp(ptr, pattern, len)) {
        return ptr - str -> buffer;
      }
    }
    return -1;
  }
}

int str_read(str_t *str, char *target, int num) {
  if ((str -> pos + num) > str -> len) {
    num = str -> len - str -> pos;
  }
  if (num > 0) {
    strncpy(target, str -> buffer + str -> pos, num);
    str -> pos = str -> pos + num;
    return num;
  } else {
    return 0;
  }
}

int str_readchar(str_t *str) {
  return (str -> pos < str -> len) ? str -> buffer[str -> pos++] : 0;
}

int str_pushback(str_t *str, int num) {
  if (num > str -> pos) {
    num = str -> pos;
  }
  str -> pos -= num;
  return num;
}

int str_readinto(str_t *str, data_t *rdr) {
  data_t * ret;
  int      retval;

  str_erase(str);
  ret = data_read(rdr, str -> buffer, str -> bufsize);
  if (data_type(ret) == Int) {
    retval = data_intval(ret);
    str -> buffer[retval] = '\0';
    str -> len = retval;
  } else {
    retval = -1;
  }
  return retval;
}

int str_write(str_t *str, char *buf, int num) {
  return (str_append_nchars(str, buf, num)) ? num : -1;
}

str_t * str_set(str_t* str, int i, int ch) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (i < 0) {
      i = 0;
    }
    if (i < str -> len) {
      str -> buffer[i] = ch;
      str -> pos = 0;
      ret = str;
    }
  }
  return ret;
}

str_t * str_forcecase(str_t *str, int upper) {
  int len = str_len(str);
  int c;
  int ix;

  for (ix = 0; ix < len; ix++) {
    c = str_at(str, ix);
    str_set(str, ix, upper ? toupper(c) : tolower(c));
  }
  return str;
}

str_t * str_append_char(str_t *str, int ch) {
  str_t *ret = NULL;

  if (str -> bufsize && (ch > 0)) {
    if (_str_expand(str, str -> len + 1)) {
      str -> buffer[str -> len++] = ch;
      str -> pos = 0;
      ret = str;
    }
  }
  return ret;
}

str_t * str_append_chars(str_t *str, char *other, ...) {
  va_list  args;
  str_t   *ret;

  va_start(args, other);
  ret = str_append_vchars(str, other, args);
  va_end(args);
  return ret;
}

str_t * str_append_nchars(str_t *str, char *other, int n) {
  va_list  args;
  str_t   *ret = NULL;

  if (other && _str_expand(str, str_len(str) + n + 1)) {
    strncat(str -> buffer, other, n);
    str -> len += (strlen(other) > n) ? n : strlen(other);
    str -> buffer[str -> len] = 0;
    str -> pos = 0;
    ret = str;
  }
  return ret;
}

str_t * str_append_vchars(str_t *str, char *other, va_list args) {
  str_t   *ret = NULL;
  char    *b;
  va_list  args_copy;

  if (str -> bufsize) {
    va_copy(args_copy, args);
    b = (char *) new(vsnprintf(NULL, 0, other, args) + 1);
    vsprintf(b, other, args_copy);
    va_end(args_copy);
    if (b && _str_expand(str, str_len(str) + strlen(b) + 1)) {
      strcat(str -> buffer, b);
      str -> len += strlen(b);
      free(b);
      str -> pos = 0;
      ret = str;
    }
  }
  return ret;
}

str_t * str_append(str_t *str, str_t *other) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (_str_expand(str, str_len(str) + str_len(other) + 1)) {
      strcat(str -> buffer, str_chars(other));
      str -> len += str_len(other);
      str -> pos = 0;
      ret = str;
    }
  }
  return ret;
}

str_t * str_chop(str_t *str, int num) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (num >= str -> len) {
      str_erase(str);
    } else if (num > 0) {
      str -> len = str -> len - num;
      memset(str -> buffer + str -> len, 0, num);
    }
    str -> pos = 0;
    ret = str;
  }
  return ret;
}

str_t * str_lchop(str_t *str, int num) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (num >= str_len(str)) {
      str_erase(str);
    } else if (num > 0) {
      memmove(str -> buffer, str -> buffer + num, str -> len - num);
      memset(str -> buffer + str -> len - num, 0, num);
      str -> len = str -> len - num;
    }
    str -> pos = 0;
    ret = str;
  }
  return ret;
}

str_t * str_erase(str_t *str) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    memset(str -> buffer, 0, str -> bufsize);
    str -> len = 0;
    str -> pos = 0;
    ret = str;
  }
  return ret;
}

str_t * _str_join(char *glue, void *collection, obj_reduce_t reducer) {
  str_t      *ret;
  reduce_ctx *ctx;

  ret = str_create(0);
  ctx = reduce_ctx_create(glue, ret, NULL);
  reducer(collection, (reduce_t) _str_join_reducer, ctx);
  if (str_len(ret)) {
    str_chop(ret, strlen(glue));
  }
  free(ctx);
  return ret;
}

str_t * str_slice(str_t *str, int from, int upto) {
  str_t *ret;
  int len;

  if (from < 0) {
    from = 0;
  }
  if ((upto > str_len(str)) || (upto < 0)) {
    upto = str_len(str);
  }
  len = upto - from;
  ret = str_copy_chars(str_chars(str) + from);
  if (ret && (str_len(ret) > len)) {
    str_chop(ret, str_len(ret) - len);
  }
  return ret;
}

array_t * str_split(str_t *str, char *sep) {
  char    *ptr;
  char    *sepptr;
  array_t *ret;
  str_t   *c;

  ret = array_create(4);
  array_set_free(
    array_set_hash(
      array_set_tostring(
        array_set_cmp(ret, (cmp_t) str_cmp),
        (tostring_t) str_chars),
      (hash_t) str_hash),
    (free_t) _str_free);

  if (str_len(str)) {
    ptr = str -> buffer;
    for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
      c = str_copy_nchars(sepptr - ptr, ptr);
      array_push(ret, c);
      ptr = sepptr + strlen(sep);
    }
    c = str_copy_chars(ptr);
    array_push(ret, c);
  }
  return ret;
}

str_t * str_format(char *fmt, array_t *args, dict_t *kwargs) {
  char  *ptr;
  char  *specstart;
  long   ix;
  str_t *ret = str_create(strlen(fmt));
  char   buf[_DEFAULT_SIZE];
  int    bufsize = _DEFAULT_SIZE;
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
        if (bufsize > _DEFAULT_SIZE) {
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

/* -- S T R I N G   T Y P E   M E T H O D S ------------------------------- */

data_t * _string_format(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return (data_t *) str_format(data_tostring(self), args, kwargs);
}

data_t * _string_at(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) kwargs;
  return _str_resolve((str_t *) self, data_tostring(data_array_get(args, 0)));
}

data_t * _string_slice(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *from = data_array_get(args, 0);
  data_t *to = data_array_get(args, 1);
  int     len = strlen(data_tostring(self));
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
                          data_typename(self),
                          name,
                          i, len - 1);
  } else if ((j <= i) || (j > len)) {
    return data_exception(ErrorRange, "%s.%s argument out of range: %d not in [%d..%d]",
                          data_typename(self),
                          name,
                          j, i+1, len);
  } else {
    // FIXME --- Or at least check me.
    strncpy(buf, data_tostring(self) + i, j - i);
    buf[j-i] = 0;
    return data_create(String, buf);
  }
}

data_t * _string_forcecase(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int  upper = name[0] == 'u';
  str_t *ret = str_copy_chars(data_tostring(self));

  return (data_t *) str_forcecase(ret, upper);
}

data_t * _string_has(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *needle = data_tostring(data_array_get(args, 0));

  return data_create(Bool, str_indexof_chars((str_t *) self, needle) >= 0);
}

data_t * _string_indexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *needle = data_tostring(data_array_get(args, 0));

  return data_create(Int, str_indexof_chars((str_t *) self, needle));
}

data_t * _string_rindexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *needle = data_tostring(data_array_get(args, 0));

  return data_create(Int, str_rindexof_chars((str_t *) self, needle));
}

data_t * _string_startswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *prefix = data_tostring(data_array_get(args, 0));
  int   len;
  int   prflen;

  len = str_len((str_t *) self);
  prflen = strlen(prefix);
  if (prflen > len) {
    return data_create(Bool, 0);
  } else {
    return data_create(Bool, strncmp(prefix, str_chars((str_t *) self), prflen) == 0);
  }
}

data_t * _string_endswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *suffix = data_tostring(data_array_get(args, 0));
  int   len;
  int   suflen;
  char *ptr;

  len = str_len((str_t *) self);
  suflen = strlen(suffix);
  if (suflen > len) {
    return data_create(Bool, 0);
  } else {
    ptr = str_chars((str_t *) self) + (len - suflen);
    return data_create(Bool, strncmp(suffix, ptr, suflen) == 0);
  }
}

data_t * _string_concat(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  str_t  *ret = str_copy_chars(data_tostring(self));
  int     ix;
  int     len = str_len(ret);

  for (ix = 0; ix < array_size(args); ix++) {
    len += strlen(data_tostring(data_array_get(args, ix)));
  }
  _str_expand(ret, len);
  for (ix = 0; ix < array_size(args); ix++) {
    str_append_chars(ret, data_tostring(data_array_get(args, ix)));
  }
  return (data_t *) ret;
}

data_t * _string_repeat(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char   *s = data_tostring(self);
  str_t  *ret = str_copy_chars(s);
  int     len = str_len(ret);
  int     numval = data_intval(data_array_get(args, 0));
  int     ix;
  char   *buf;

  len *= numval;
  if (len < 0) {
    str_erase(ret);
  } else {
    _str_expand(ret, len);
    for (ix = 0; ix < numval; ix++) {
      str_append_chars(ret, s);
    }
  }
  return (data_t *) ret;
}

data_t * _string_split(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  array_t *split = array_split(data_tostring(self),
                               data_tostring(data_array_get(args, 0)));
  data_t  *ret = data_str_array_to_list(split);

  array_free(split);
  return ret;
}

