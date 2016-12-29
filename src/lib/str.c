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

#include "libcore.h"
#include <array.h>
#include <data.h>
#include <exception.h>
#include <str.h>
#include <typedescr.h>

extern void         str_init(void);

static str_t *      _str_initialize(void);
static str_t *      _str_expand(str_t *, size_t);
static reduce_ctx * _str_join_reducer(char *, reduce_ctx *);
static int          _str_readinto(str_t *, data_t *, int, int);
static int          _str_read_from_stream(str_t *, void *, read_t, int, int);

static data_t * _str_create(int, va_list);
static void     _str_free(str_t *);
static data_t * _str_cast(str_t *, int);
static str_t *  _str_parse(char *str);
static data_t * _str_resolve(str_t *, char *);

static data_t * _string_slice(data_t *, char *, array_t *, dict_t *);
static data_t * _string_at(data_t *, char *, array_t *, dict_t *);
static data_t * _string_forcecase(data_t *, char *, array_t *, dict_t *);
static data_t * _string_has(data_t *, char *, array_t *, dict_t *);
static data_t * _string_indexof(data_t *, char *, array_t *, dict_t *);
static data_t * _string_rindexof(data_t *, char *, array_t *, dict_t *);
static data_t * _string_startswith(data_t *, char *, array_t *, dict_t *);
static data_t * _string_endswith(data_t *, char *, array_t *, dict_t *);
static data_t * _string_concat(data_t *, char *, array_t *, dict_t *);
static data_t * _string_repeat(data_t *, char *, array_t *, dict_t *);
static data_t * _string_split(data_t *, char *, array_t *, dict_t *);

#define _DEFAULT_SIZE   32

static vtable_t _vtable_String[] = {
  { .id = FunctionFactory, .fnc = (void_t) _str_create},
  { .id = FunctionCmp, .fnc = (void_t) str_cmp},
  { .id = FunctionFree, .fnc = (void_t) _str_free},
  { .id = FunctionToString, .fnc = (void_t) str_chars},
  { .id = FunctionParse, .fnc = (void_t) _str_parse},
  { .id = FunctionCast, .fnc = (void_t) _str_cast},
  { .id = FunctionHash, .fnc = (void_t) str_hash},
  { .id = FunctionLen, .fnc = (void_t) str_len},
  { .id = FunctionRead, .fnc = (void_t) str_read},
  { .id = FunctionWrite, .fnc = (void_t) str_write},
  { .id = FunctionResolve, .fnc = (void_t) _str_resolve},
  { .id = FunctionNone, .fnc = NULL}
};

static methoddescr_t _methods_String[] = {
  { .type = String, .name = "at", .method = _string_at, .argtypes =
    { Int, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "slice", .method = _string_slice, .argtypes =
    { Int, NoType, NoType}, .minargs = 1, .varargs = 1},
  { .type = String, .name = "upper", .method = _string_forcecase, .argtypes =
    { NoType, NoType, NoType}, .minargs = 0, .varargs = 0},
  { .type = String, .name = "lower", .method = _string_forcecase, .argtypes =
    { NoType, NoType, NoType}, .minargs = 0, .varargs = 0},
  { .type = String, .name = "has", .method = _string_has, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "indexof", .method = _string_indexof, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "rindexof", .method = _string_rindexof, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "startswith", .method = _string_startswith, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "endswith", .method = _string_endswith, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "+", .method = _string_concat, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 1},
  { .type = String, .name = "concat", .method = _string_concat, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 1},
  { .type = String, .name = "*", .method = _string_repeat, .argtypes =
    { Int, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "repeat", .method = _string_repeat, .argtypes =
    { Int, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = String, .name = "split", .method = _string_split, .argtypes =
    { String, NoType, NoType}, .minargs = 1, .varargs = 0},
  { .type = NoType, .name = NULL, .method = NULL, .argtypes =
    { NoType, NoType, NoType}, .minargs = 0, .varargs = 0}
};

extern int data_debug;

/* ------------------------------------------------------------------------ */

void str_init(void) {
  builtin_typedescr_register(String, "string", str_t);
}

/* -- S T R I N G   D A T A   F U N C T I O N S --------------------------- */

void _str_free(str_t *str) {
  if (str) {
    if (str -> bufsize) {
      free(str -> buffer);
    }
  }
}

data_t * _str_resolve(str_t *str, char *slice) {
  int sz = str_len(str);
  long ix;
  char buf[2];

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
      return str_to_data(buf);
    }
  } else {
    return NULL;
  }
}

data_t * _str_cast(str_t *data, int totype) {
  typedescr_t *type;
  parse_t parse;
  char *str = str_chars(data);

  if (totype == Bool) {
    return int_as_bool(str && strlen(str));
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
  str_t *ret;

  ret = data_new(String, str_t);
  ret -> buffer = NULL;
  ret -> pos = 0;
  ret -> len = 0;
  ret -> bufsize = 0;
  return ret;
}

data_t * _str_create(int type, va_list args) {
  return (data_t *) str_copy_chars(va_arg(args, char *));
}

str_t * _str_expand(str_t *str, size_t targetlen) {
  size_t  newsize;
  char   *oldbuf;
  str_t  *ret = str;

  if (!targetlen) {
    targetlen = str -> bufsize;
  }
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

int _str_read_from_stream(str_t *str, void *stream, read_t reader, int pos, int num) {
  size_t ret;

  if (pos >= str -> bufsize) {
    return -1;
  }
  if ((pos + num) > str -> bufsize) {
    num = str -> bufsize - pos;
  }
  ret = reader(stream, str -> buffer + pos, num);
  if ((int) ret < 0) {
    return -1;
  } else {
    if (pos + ret < str -> bufsize) {
      str -> buffer[pos + ret] = 0;
    }
    if (pos <= str -> len) {
      str -> len += ret;
    }
    return (int) ret;
  }
}

int _str_readinto(str_t *str, data_t *rdr, int pos, int num) {
  typedescr_t *type = data_typedescr(rdr);
  read_t       fnc;

  fnc = (read_t) typedescr_get_function(type, FunctionRead);
  if (fnc) {
    return _str_read_from_stream(str, rdr, fnc, pos, num);
  } else {
    return -1;
  }
}

/* -- S T R _ T   P U B L I C   F U N C T I O N S ------------------------- */

str_t * str_create(size_t size) {
  str_t *ret;

  ret = _str_initialize();
  size = size ? size : _DEFAULT_SIZE;
  ret -> buffer = stralloc(size);
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

str_t * str_printf(const char *fmt, ...) {
  str_t   *ret;
  va_list  args;

  va_start(args, fmt);
  ret = str_vprintf(fmt, args);
  va_end(args);
  return ret;
}

str_t * str_vprintf(const char *fmt, va_list args) {
  str_t *ret;

  ret = _str_initialize();
  if (ret) {
    vasprintf(&ret -> buffer, fmt, args);
    ret -> len = strlen(ret -> buffer);
    ret -> bufsize = ret -> len + 1;
  }
  return ret;
}

str_t * str_copy_chars(const char *buffer) {
  return str_copy_nchars(buffer, strlen(buffer));
}

str_t * str_copy_nchars(const char *buffer, size_t len) {
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

str_t * str_from_data(data_t *data) {
  return (data_is_string(data))
    ? (str_t *) data_copy(data)
    : str_copy_chars(data_tostring(data));
}

str_t * str_deepcopy(const str_t *str) {
  str_t *ret;
  char *b;

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

size_t str_len(const str_t *str) {
  return str -> len;
}

char * str_chars(const str_t *str) {
  return str -> buffer;
}

int str_at(const str_t* str, size_t i) {
  if ((int) i < 0) {
    i = str -> len + i;
  }
  return (i < str -> len) ? str -> buffer[i] : -1;
}

unsigned int str_hash(const str_t *str) {
  return strhash(str -> buffer);
}

int str_cmp(const str_t *s1, const str_t *s2) {
  return strcmp(s1 -> buffer, s2 -> buffer);
}

int str_cmp_chars(const str_t *s1, const char *s2) {
  return strcmp(s1 -> buffer, s2);
}

int str_ncmp(const str_t *s1, const str_t *s2, size_t numchars) {
  return strncmp(s1 -> buffer, s2 -> buffer, numchars);
}

int str_ncmp_chars(const str_t *s1, const char *s2, size_t numchars) {
  return strncmp(s1 -> buffer, s2, numchars);
}

int str_indexof(const str_t *str, const str_t *pattern) {
  if (pattern -> len > str -> len) {
    return -1;
  } else {
    return str_indexof_chars(str, str_chars(pattern));
  }
}

int str_indexof_chars(const str_t *str, const char *pattern) {
  char *ptr;

  if (strlen(pattern) > str -> len) {
    return -1;
  } else {
    ptr = strstr(str_chars(str), pattern);
    return (ptr) ? ptr - str_chars(str) : -1;
  }
}

int str_rindexof(const str_t *str, const str_t *pattern) {
  return str_rindexof_chars(str, str_chars(pattern));
}

int str_rindexof_chars(const str_t *str, const char *pattern) {
  char   *ptr;
  size_t  len;

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

int str_rewind(str_t *str) {
  str -> pos = 0;
  return 0;
}

int str_read(str_t *str, char *target, size_t num) {
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

int str_peek(const str_t * str) {
  return (str -> pos < str -> len) ? str -> buffer[str -> pos] : 0;
}

int str_readchar(str_t *str) {
  int ret;

  ret = str_peek(str);
  str -> pos++;
  return ret;
}

int str_skip(str_t *str, int num) {
  if (str -> pos + num > str -> len) {
    num = str -> len - str -> pos;
  }
  str -> pos += num;
  return num;
}

int str_pushback(str_t *str, size_t num) {
  if (num > str -> pos) {
    num = str -> pos;
  }
  str -> pos -= num;
  return num;
}

int str_readinto(str_t *str, data_t *rdr) {
  return _str_readinto(str, rdr, 0, str -> bufsize);
}

int str_fillup(str_t *str, data_t *rdr) {
  return _str_readinto(str, rdr, str -> len, str -> bufsize - str -> len);
}

int str_replenish(str_t *str, data_t *rdr) {
  if (str -> pos > str -> len) {
    str -> pos = str -> len;
  }
  if (str -> len < str -> bufsize) {
    return str_fillup(str, rdr);
  } else {
    _str_expand(str, 0);
    return _str_readinto(str, rdr, str -> len, str -> len);
  }
}

/**
 * Chop everything from the string before the current read position.
 */
str_t * str_reset(str_t *str) {
  if (str -> pos) {
    str_lchop(str, str -> pos);
  }
  return str;
}

int str_read_from_stream(str_t *str, void *stream, read_t reader) {
  str_erase(str);
  return _str_read_from_stream(str, stream, reader, 0, str -> bufsize);
}

int str_write(str_t *str, char *buf, size_t num) {
  return (str_append_nchars(str, buf, num)) ? num : -1;
}

str_t * str_set(str_t* str, size_t i, int ch) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if ((int) i < 0) {
      i = 0;
    }
    if (i < str -> len) {
      str -> buffer[i] = ch;
      str -> pos = 0;
      ret = str;
      if (!ch) {
        str -> len = i;
      }
    }
  }
  return ret;
}

str_t * str_forcecase(str_t *str, int upper) {
  size_t len = str_len(str);
  int    c;
  size_t ix;

  for (ix = 0; ix < len; ix++) {
    c = str_at(str, ix);
    str_set(str, ix, upper ? toupper(c) : tolower(c));
  }
  return str;
}

int str_replace(str_t *str, const char *pat, const char *repl, int max) {
  int   pos;
  int   pat_len = strlen(pat);
  int   repl_len = strlen(repl);
  int   diff = repl_len - pat_len;
  int   num = 0;
  char *target;

  for (pos = str_indexof_chars(str, pat);
       max && (pos >= 0);
       max--, pos = str_indexof_chars(str, pat), num++) {
    if (diff > 0) {
      _str_expand(str, str -> len + diff);
    }
    target = str -> buffer + pos;
    if (diff) {
      memmove(target + (pat_len + diff), target + pat_len,
          str -> len - pos - pat_len);
      str -> len += diff;
    }
    memcpy(str -> buffer + pos, repl, repl_len);
    str -> pos = 0;
  }
  return num;
}

str_t * str_append_char(str_t *str, int ch) {
  str_t *ret = NULL;

  if (str -> bufsize && (ch > 0)) {
    if (_str_expand(str, str -> len + 1)) {
      str -> buffer[str -> len++] = ch;
      ret = str;
    }
  }
  return ret;
}

str_t * str_append_chars(str_t *str, const char *other) {
  return str_append_nchars(str, other, strlen(other));
}

str_t * str_append_nchars(str_t *str, const char *other, size_t n) {
  str_t   *ret = NULL;

  if (other && _str_expand(str, str_len(str) + n + 1)) {
    strncat(str -> buffer, other, n);
    str -> len += (strlen(other) > n) ? n : strlen(other);
    str -> buffer[str -> len] = 0;
    ret = str;
  }
  return ret;
}

str_t * str_append_printf(str_t *str, const char *other, ...) {
  va_list  args;
  str_t   *ret;

  va_start(args, other);
  ret = str_append_vprintf(str, other, args);
  va_end(args);
  return ret;
}

str_t * str_append_vprintf(str_t *str, const char *other, va_list args) {
  str_t  *ret = NULL;
  char   *b = NULL;
  size_t  len = 0;

  if (str -> bufsize) {
    len = vasprintf(&b, other, args);
    if (b && _str_expand(str, str_len(str) + len + 1)) {
      strcat(str -> buffer, b);
      str -> len += len;
      ret = str;
    }
    free(b);
  }
  return ret;
}

str_t * str_append(str_t *str, const str_t *other) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (_str_expand(str, str_len(str) + str_len(other) + 1)) {
      strcat(str -> buffer, str_chars(other));
      str -> len += str_len(other);
      ret = str;
    }
  }
  return ret;
}

str_t * str_chop(str_t *str, size_t num) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (num >= str -> len) {
      str_erase(str);
    } else if (num > 0) {
      str -> len = str -> len - num;
      memset(str -> buffer + str -> len, 0, num);
    }
    if (str -> pos > str -> len) {
      str -> pos = str -> len;
    }
    ret = str;
  }
  return ret;
}

str_t * str_lchop(str_t *str, size_t num) {
  str_t *ret = NULL;

  if (str -> bufsize) {
    if (num >= str_len(str)) {
      str_erase(str);
    } else if (num > 0) {
      memmove(str -> buffer, str -> buffer + num, str -> len - num);
      memset(str -> buffer + str -> len - num, 0, num);
      str -> len = str -> len - num;
    }
    str -> pos = (str -> pos < num) ? 0 : (str -> pos - num);
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

str_t * _str_join(const char *glue, const void *collection, obj_reduce_t reducer) {
  str_t      *ret;
  reduce_ctx  ctx;

  ret = str_create(0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  reduce_ctx_initialize(&ctx, glue, ret, NULL);
  reducer(collection, (reduce_t) _str_join_reducer, &ctx);
#pragma GCC diagnostic pop
  if (str_len(ret)) {
    str_chop(ret, strlen(glue));
  }
  return ret;
}

str_t * str_slice(const str_t *str, size_t from, size_t upto) {
  str_t  *ret;
  size_t  len;

  if ((int) from < 0) {
    from = 0;
  }
  if ((upto > str_len(str)) || ((int) upto < 0)) {
    upto = str_len(str);
  }
  len = upto - from;
  ret = str_copy_chars(str_chars(str) + from);
  if (ret && (str_len(ret) > len)) {
    str_chop(ret, str_len(ret) - len);
  }
  return ret;
}

array_t * str_split(const str_t *str, const char *sep) {
  char    *ptr;
  char    *sepptr;
  array_t *ret;
  str_t   *c;

  ret = data_array_create(4);
  if (str_len(str)) {
    ptr = str -> buffer;
    for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
      c = str_copy_nchars(ptr, sepptr - ptr);
      array_push(ret, c);
      ptr = sepptr + strlen(sep);
    }
    c = str_copy_chars(ptr);
    array_push(ret, c);
  }
  return ret;
}

str_t * str_format(const char *fmt, const array_t *args, const dict_t *kwargs) {
  char   *copy;
  char   *ptr;
  char   *specstart;
  long    ix;
  str_t  *ret = str_create(strlen(fmt));
  char    buf[_DEFAULT_SIZE];
  size_t  bufsize = _DEFAULT_SIZE;
  char   *bigbuf = NULL;
  char   *spec = buf;
  size_t  len;

  copy = strdup(fmt);
  for (ptr = copy; *ptr; ptr++) {
    if ((*ptr == '$') && (*(ptr + 1) == '{')) {
      ptr += 2;
      specstart = ptr;
      for (; *ptr && (*ptr != '}'); ptr++);
      len = ptr - specstart;
      if (!*ptr) {
        str_append_nchars(ret, specstart - 2, len + 2);
        break;
      } else {
        while (len >= bufsize - 1) {
          bufsize *= 2;
        }
        if (bufsize > _DEFAULT_SIZE) {
          free(bigbuf);
          bigbuf = (char *) new(bufsize);
          spec = bigbuf;
        }
        strncpy(spec, specstart, len);
        spec[len] = 0;
        if (kwargs && dict_has_key(kwargs, spec)) {
          str_append_chars(ret, data_tostring(data_dict_get(kwargs, spec)));
        } else {
          if (!strtoint(spec, &ix) && args && (ix >= 0) && (ix < array_size(args))) {
            str_append_chars(ret, data_tostring(data_array_get(args, ix)));
          } else {
            str_append_printf(ret, "${%s}", spec);
          }
        }
      }
    } else {
      str_append_char(ret, *ptr);
    }
  }
  free(bigbuf);
  free(copy);
  return ret;
}

str_t * str_vformatf(const char *fmt, va_list args) {
  array_t *arr;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  char    *f = fmt;
#pragma GCC diagnostic pop
  char    *ptr;
  int      ix;
  int      num = 0;
  char     needle[32];
  str_t   *ret;
  int      done;

  if (!strstr(fmt, "${")) {
    return str_copy_chars(fmt);
  }
  strcpy(needle, "${");
  do {
    done = 1;
    sprintf(needle + 2, "%d", num);
    ptr = strstr(fmt, needle);
    if (ptr) {
      ptr = ptr + strlen(needle);
      if (strpbrk(ptr, ":;}") == ptr) {
        done = 0;
        num++;
      }
    }
  } while (!done);
  if (num > 0) {
    arr = data_array_create(num);
    for (ix = 0; ix < num; ix++) {
      done = 0;
      sprintf(needle + 2, "%d;", ix);
      if ((ptr = strstr(f, needle))) {
        if (f == fmt) {
          f = strdup(fmt);
          ptr = strstr(f, needle);
        }
        ptr += strlen(needle);
        if (strpbrk(ptr, "dspf") == ptr) {
          switch (ptr[0]) {
            case 'd':
              array_push(arr, int_to_data(va_arg(args, long)));
              break;
            case 's':
              array_push(arr, str_to_data(va_arg(args, char *)));
              break;
            case 'p':
              array_push(arr, ptr_to_data(0, va_arg(args, void *)));
              break;
            case 'f':
              array_push(arr, flt_to_data(va_arg(args, double)));
              break;
          }
          done = 1;
          memmove(ptr - 1, ptr + 1, strlen(ptr + 1) + 1);
        }
      }
      if (!done) {
        array_push(arr, data_copy(va_arg(args, data_t *)));
      }
    }
  }
  ret = str_format(f, arr, NULL);
  array_free(arr);
  if (f != fmt) {
    free(f);
  }
  return ret;
}

str_t * str_formatf(const char *fmt, ...) {
  va_list args;
  str_t *ret;

  va_start(args, fmt);
  ret = str_vformatf(fmt, args);
  va_end(args);
  return ret;
}

/* -- S T R I N G   T Y P E   M E T H O D S ------------------------------- */

data_t * _string_at(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) kwargs;
  return _str_resolve((str_t *) self, data_tostring(data_array_get(args, 0)));
}

data_t * _string_slice(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *from = data_array_get(args, 0);
  data_t *to = data_array_get(args, 1);
  data_t *ret;
  size_t  len = strlen(data_tostring(self));
  size_t  i;
  size_t  j;
  char   *buf;

  /* FIXME: second argument (to) is optional; ommiting it gives you the tail */
  i = (size_t) data_intval(from);
  j = (size_t) data_intval(to);
  if (j <= 0) {
    j = len + j;
  }
  if ((int) i < 0) {
    i = len + i;
  }
  if (((int) i < 0) || (i >= len)) {
    return data_exception(ErrorRange, "%s.%s argument out of range: %d not in [0..%d]",
                          data_typename(self),
                          name,
                          i, len - 1);
  } else if ((j <= i) || (j > len)) {
    return data_exception(ErrorRange, "%s.%s argument out of range: %d not in [%d..%d]",
                          data_typename(self),
                          name,
                          j, i + 1, len);
  } else {
    // FIXME --- Or at least check me.
    buf = (char *) _new(len + 1);
    strncpy(buf, data_tostring(self) + i, j - i);
    buf[j - i] = 0;
    ret = str_to_data(buf);
    free(buf);
    return ret;
  }
}

data_t * _string_forcecase(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int upper = name[0] == 'u';
  str_t *ret = str_copy_chars(data_tostring(self));

  return (data_t *) str_forcecase(ret, upper);
}

data_t * _string_has(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *needle = data_tostring(data_array_get(args, 0));

  return int_as_bool(str_indexof_chars((str_t *) self, needle) >= 0);
}

data_t * _string_indexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *needle = data_tostring(data_array_get(args, 0));

  return int_to_data(str_indexof_chars((str_t *) self, needle));
}

data_t * _string_rindexof(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *needle = data_tostring(data_array_get(args, 0));

  return int_to_data(str_rindexof_chars((str_t *) self, needle));
}

data_t * _string_startswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *prefix = data_tostring(data_array_get(args, 0));
  int len;
  int prflen;

  len = str_len((str_t *) self);
  prflen = strlen(prefix);
  if (prflen > len) {
    return data_false();
  } else {
    return int_as_bool(strncmp(prefix, str_chars((str_t *) self), prflen) == 0);
  }
}

data_t * _string_endswith(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char *suffix = data_tostring(data_array_get(args, 0));
  int len;
  int suflen;
  char *ptr;

  len = str_len((str_t *) self);
  suflen = strlen(suffix);
  if (suflen > len) {
    return data_false();
  } else {
    ptr = str_chars((str_t *) self) + (len - suflen);
    return int_as_bool(strncmp(suffix, ptr, suflen) == 0);
  }
}

data_t * _string_concat(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  str_t *ret = str_copy_chars(data_tostring(self));
  int ix;
  int len = str_len(ret);

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
  size_t  len = str_len(ret);
  int     numval = data_intval(data_array_get(args, 0));
  int     ix;

  len *= numval;
  if ((int) len < 0) {
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
