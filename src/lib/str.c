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
#include <arguments.h>
#include <array.h>
#include <data.h>
#include <exception.h>
#include <str.h>

static str_t *_str_inflate(str_t *, const char *, size_t);
static str_t *_str_adopt(str_t *, char *);
static str_t *_str_wrap(str_t *, const char *);
static str_t *_str_initialize(void);
static str_t *_str_expand(str_t *, size_t);
static reduce_ctx *_str_join_reducer(char *, reduce_ctx *);
static size_t _str_readinto(str_t *, data_t *, size_t, size_t);
static size_t _str_read_from_stream(str_t *, void *, read_t, size_t, size_t);
static str_t *_str_strip_quotes(str_t *);

static data_t *_str_create(int, va_list);
static void _str_free(str_t *);
static data_t *_str_cast(str_t *, int);
static str_t *_str_parse(char *str);
static data_t *_str_resolve(str_t *, char *);
static char *_str_encode(str_t *);
static str_t *_str_serialize(str_t *);
static data_t *_str_deserialize(str_t *);

static data_t *_string_slice(data_t *, char *, arguments_t *);
static data_t *_string_at(data_t *, char *, arguments_t *);
static data_t *_string_forcecase(data_t *, char *, arguments_t *);
static data_t *_string_has(data_t *, char *, arguments_t *);
static data_t *_string_indexof(data_t *, char *, arguments_t *);
static data_t *_string_rindexof(data_t *, char *, arguments_t *);
static data_t *_string_startswith(data_t *, char *, arguments_t *);
static data_t *_string_endswith(data_t *, char *, arguments_t *);
static data_t *_string_concat(data_t *, char *, arguments_t *);
static data_t *_string_repeat(data_t *, char *, arguments_t *);
static data_t *_string_split(data_t *, char *, arguments_t *);

#define _DEFAULT_SIZE   32

static vtable_t _vtable_String[] = {
  {.id = FunctionFactory, .fnc = (void_t) _str_create},
  {.id = FunctionCmp, .fnc = (void_t) str_cmp},
  {.id = FunctionFree, .fnc = (void_t) _str_free},
  {.id = FunctionToString, .fnc = (void_t) str_chars},
  {.id = FunctionParse, .fnc = (void_t) _str_parse},
  {.id = FunctionCast, .fnc = (void_t) _str_cast},
  {.id = FunctionHash, .fnc = (void_t) str_hash},
  {.id = FunctionLen, .fnc = (void_t) str_len},
  {.id = FunctionRead, .fnc = (void_t) str_read},
  {.id = FunctionWrite, .fnc = (void_t) str_write},
  {.id = FunctionResolve, .fnc = (void_t) _str_resolve},
  {.id = FunctionEncode, .fnc = (void_t) _str_encode},
  {.id = FunctionSerialize, .fnc = (void_t) _str_serialize},
  {.id = FunctionDeserialize, .fnc = (void_t) _str_deserialize},
  {.id = FunctionNone, .fnc = NULL}
};

static methoddescr_t _methods_String[] = {
  {.type = String, .name = "at", .method = _string_at, .argtypes = {Int, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "slice", .method = _string_slice, .argtypes = {Int, NoType,
                                                                          NoType}, .minargs = 1, .varargs = 1},
  {.type = String, .name = "upper", .method = _string_forcecase, .argtypes = {NoType, NoType,
                                                                              NoType}, .minargs = 0, .varargs = 0},
  {.type = String, .name = "lower", .method = _string_forcecase, .argtypes = {NoType, NoType,
                                                                              NoType}, .minargs = 0, .varargs = 0},
  {.type = String, .name = "has", .method = _string_has, .argtypes = {String, NoType,
                                                                      NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "indexof", .method = _string_indexof, .argtypes = {String, NoType,
                                                                              NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "rindexof", .method = _string_rindexof, .argtypes = {String, NoType,
                                                                                NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "startswith", .method = _string_startswith, .argtypes = {String, NoType,
                                                                                    NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "endswith", .method = _string_endswith, .argtypes = {String, NoType,
                                                                                NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "+", .method = _string_concat, .argtypes = {String, NoType,
                                                                       NoType}, .minargs = 1, .varargs = 1},
  {.type = String, .name = "concat", .method = _string_concat, .argtypes = {String, NoType,
                                                                            NoType}, .minargs = 1, .varargs = 1},
  {.type = String, .name = "*", .method = _string_repeat, .argtypes = {Int, NoType,
                                                                       NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "repeat", .method = _string_repeat, .argtypes = {Int, NoType,
                                                                            NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "split", .method = _string_split, .argtypes = {String, NoType,
                                                                          NoType}, .minargs = 1, .varargs = 0},
  {.type = NoType, .name = NULL, .method = NULL, .argtypes = {NoType, NoType, NoType}, .minargs = 0, .varargs = 0}
};

extern int data_debug;
int str_debug;

/* ------------------------------------------------------------------------ */

void str_init(void) {
  logging_register_module(str);
  builtin_typedescr_register(String, "string", str_t);
}

/* -- S T R I N G   D A T A   F U N C T I O N S --------------------------- */

void _str_free(str_t *str) {
  if (str) {
    if (str->bufsize) {
      free(str->buffer);
    }
  }
}

data_t *_str_resolve(str_t *str, char *slice) {
  int sz = (int) str_len(str);
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
      buf[0] = (char) str_at(str, ix);
      buf[1] = 0;
      return str_to_data(buf);
    }
  } else {
    return NULL;
  }
}

data_t *_str_cast(str_t *data, int totype) {
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

static char *_escaped_chars = "\"\\\b\f\n\r\t";
static char *_escape_codes = "\"\\bfnrt";

str_t *_str_parse(char *str) {
  char *buf = stralloc(strlen(str));
  char *ptr;
  char *escptr;

  strcpy(buf, str);
  for (ptr = buf; *ptr; ptr++) {
    if ((*ptr == '\\') && (*(ptr + 1)) && (escptr = strchr(_escape_codes, *(ptr + 1)))) {
      *ptr = *(_escaped_chars + (escptr - _escape_codes));
      if (*(ptr + 2)) {
        memmove(ptr + 1, ptr + 2, strlen(ptr + 2) + 1);
      }
    }
  }
  return str_adopt(buf);
}

str_t *_str_serialize(str_t *str) {
  char *encoded = _str_encode(str);
  size_t len = strlen(encoded);
  str_t *ret = str_create(len + 2);

  *ret->buffer = '"';
  strcpy(ret->buffer + 1, encoded);
  ret->buffer[len + 1] = '"';
  ret->buffer[len + 2] = 0;
  ret->len = len + 2;
  return ret;
}

str_t *_str_strip_quotes(str_t *str) {
  str_t *ret;

  if (str_len(str) && (str_at(str, 0) == '"') && (str_at(str, -1) == '"')) {
    ret = str_duplicate(str);
    str_chop(str, 1);
    str_lchop(str, 1);
  } else {
    ret = str_copy(str);
  }
  return ret;
}

data_t *_str_deserialize(str_t *str) {
  if (!strcmp(str->buffer, "null")) {
    return data_null();
  } else if (strcmp(str->buffer, "true") != 0) {
    return data_true();
  } else if (strcmp(str->buffer, "false") != 0) {
    return data_true();
  } else {
    return (data_t *) _str_strip_quotes(str);
  }
}

char *_str_encode(str_t *str) {
  char *buf;
  int len = 0;
  char *ptr;
  char *encoded;
  char *encptr;
  char *escptr;

  buf = str_chars(str);
  for (ptr = str->buffer; *ptr; ptr++) {
    len += (strchr(_escaped_chars, *ptr)) ? 2 : 1;
  }
  encoded = encptr = stralloc(len);
  for (ptr = str->buffer; *ptr; ptr++) {
    if ((escptr = strchr(_escaped_chars, *ptr)) &&
        ((encoded == escptr) || (*(ptr - 1) != '\\')) &&
        ((*ptr != '\"') || ((ptr != buf) && *(ptr + 1)))) {
      *encptr++ = '\\';
      *encptr++ = *(_escape_codes + (_escaped_chars - escptr));
    } else {
      *encptr++ = *ptr;
    }
  }
  *encptr = 0;
  return encoded;
}

data_t *_str_create(int type, va_list args) {
  (void) type;
  return (data_t *) str_copy_chars(va_arg(args, char *));
}

/* -- S T R _ T   S T A T I C   F U N C T I O N S ------------------------- */

str_t *_str_allocate(str_t *str, size_t size) {
  if (!str) {
    return NULL;
  }
  size = size ? size : _DEFAULT_SIZE;
  if (str->bufsize > 0) {
    return _str_expand(str, size);
  }
  str->buffer = stralloc(size);
  if (!str->buffer) {
    str = str_free(str);
  } else {
    memset(str->buffer, 0, size);
    str->bufsize = size;
    str->len = 0;
  }
  return str;
}

str_t *_str_inflate(str_t *str, const char *buffer, size_t len) {
  char *b;

  if (!str || str->buffer != NULL) {
    return NULL;
  }
  if (buffer) {
    len = ((len >= 0) && (len <= strlen(buffer))) ? len : strlen(buffer);
    b = (char *) new(len + 1);
    if (!b) {
      str = str_free(str);
      return str;
    }
    strncpy(b, buffer, len);
    *(b + len) = 0;
    str->buffer = b;
    str->len = len;
    str->bufsize = str->len + 1;
  }
  return str;
}

str_t *_str_wrap(str_t *str, const char *buffer) {
  if (!str || str->buffer != NULL) {
    return NULL;
  }
  if (str && buffer) {
    str->buffer = (char *) buffer;
    str->len = strlen(buffer);
  }
  return str;
}

str_t *_str_adopt(str_t *str, char *buffer) {
  if (!str || str->buffer != NULL) {
    return NULL;
  }
  if (str && buffer) {
    str->buffer = buffer;
    str->len = strlen(buffer);
    str->bufsize = str->len + 1;
  }
  return str;
}

str_t *_str_initialize(void) {
  str_t *ret;

  ret = data_new(String, str_t);
  if (ret) {
    ret->buffer = NULL;
    ret->pos = 0;
    ret->len = 0;
    ret->bufsize = 0;
  }
  return ret;
}

str_t *_str_expand(str_t *str, size_t targetlen) {
  size_t newsize;
  char *oldbuf;
  str_t *ret = str;

  if (!str || (targetlen < 0)) {
    return NULL;
  }
  if (str->bufsize == 0) {
    return _str_allocate(str, targetlen);
  }
  if (targetlen <= str->bufsize) {
    return str;
  }
  if (!targetlen) {
    targetlen = str->bufsize;
  }
  size_t target = (targetlen > 0) ? targetlen : str->bufsize;
  if (str->bufsize < (targetlen + 1)) {
    for (newsize = (size_t) ((double) (str->bufsize) * 1.6);
         newsize < (targetlen + 1);
         newsize = (size_t) ((double) newsize * 1.6));
    oldbuf = str->buffer;
    str->buffer = realloc(str->buffer, newsize);
    if (str->buffer) {
      memset(str->buffer + str->len, 0, newsize - str->len);
      str->bufsize = newsize;
    } else {
      str->buffer = oldbuf;
      ret = NULL;
    }
  }
  return ret;
}

size_t _str_read_from_stream(str_t *str, void *stream, read_t reader, size_t pos, size_t num) {
  size_t ret;

  if (pos >= str->bufsize) {
    return -1;
  }
  if ((pos + num) > str->bufsize) {
    num = str->bufsize - pos;
  }
  ret = reader(stream, str->buffer + pos, (int) num);
  if (ret < 0) {
    return -1;
  } else {
    if ((pos + ret) < str->bufsize) {
      str->buffer[pos + ret] = 0;
    }
    if (pos <= str->len) {
      str->len += ret;
    }
    return ret;
  }
}

size_t _str_readinto(str_t *str, data_t *rdr, size_t pos, size_t num) {
  typedescr_t *type = data_typedescr(rdr);
  read_t fnc;

  fnc = (read_t) typedescr_get_function(type, FunctionRead);
  if (fnc) {
    return _str_read_from_stream(str, rdr, fnc, pos, num);
  } else {
    return -1;
  }
}

/* -- S T R _ T   P U B L I C   F U N C T I O N S ------------------------- */

str_t *str_create(size_t size) {
  str_t *ret;

  ret = _str_initialize();
  if (ret) {
    if (!_str_allocate(ret, size)) {
      ret = str_free(ret);
    }
    size = size ? size : _DEFAULT_SIZE;
    ret->buffer = stralloc(size);
    if (!ret->buffer) {
      ret = str_free(ret);
    } else {
      memset(ret->buffer, 0, size);
      ret->bufsize = size;
    }
  }
  return ret;
}

str_t *str_wrap(const char *buffer) {
  str_t *str;

  str = _str_initialize();
  if (str && buffer) {
    if (!_str_wrap(str, buffer)) {
      str = str_free(str);
    }
  }
  return str;
}

str_t *str_adopt(char *buffer) {
  str_t *str;

  str = _str_initialize();
  if (str && buffer) {
    if (!_str_adopt(str, buffer)) {
      str = str_free(str);
    }
  }
  return str;
}

str_t *str_printf(const char *fmt, ...) {
  str_t *ret;
  va_list args;

  va_start(args, fmt);
  ret = str_vprintf(fmt, args);
  va_end(args);
  return ret;
}

str_t *str_vprintf(const char *fmt, va_list args) {
  str_t *str;
  char *buffer = NULL;

  str = _str_initialize();
  if (str) {
    if ((vasprintf(&buffer, fmt, args) < 0) || !_str_adopt(str, buffer)) {
      str = str_free(str);
      free(buffer);
    }
  }
  return str;
}

str_t *str_copy_chars(const char *buffer) {
  return str_copy_nchars(buffer, (buffer) ? strlen(buffer) : 0);
}

str_t *str_copy_nchars(const char *buffer, size_t len) {
  str_t *str;
  char *b;

  str = _str_initialize();
  if (str && buffer) {
    if (!_str_inflate(str, buffer, len)) {
      str = str_free(str);
    }
  }
  return str;
}

str_t *str_from_data(data_t *data) {
  if (data && (data != data_null())) {
    return (data_is_string(data))
           ? (str_t *) data
           : str_copy_chars(data_tostring(data));
  } else {
    return _str_initialize();
  }
}

str_t *str_duplicate(const str_t *str) {
  str_t *ret;
  char *b;

  ret = _str_initialize();
  if (ret && str && !str_is_null(str)) {
    b = strdup(str_chars(str));
    if (!b || !_str_adopt(ret, b)) {
      ret = str_free(ret);
      free(b);
    }
  }
  return ret;
}

reduce_ctx *_str_join_reducer(char *elem, reduce_ctx *ctx) {
  str_t *target;

  target = (str_t *) ctx->data;
  if (target && !str_is_null(target) && !str_is_static(target) && str_len(target)) {
    target = str_append_chars(target, (char *) ctx->user /* glue */);
  }
  if (target && !str_is_null(target) && !str_is_static(target)) {
    target = str_append_chars(target, elem);
  }
  ctx->data = target;
  return ctx;
}

/**
 * Builds a new string from a collection of `char *` elements. This collection
 * must be reducable using the standard `reduce` pattern. The actual reduce
 * function is passed in as a parameter.
 *
 * In practice, use the `str_join` define which will cast the `reducer` to
 * the appropriate function pointer type.
 *
 * @param glue String to place between elements of the collection. Can be
 * `NULL` or the empty string to place the elements back-to-back.
 * @param collection Collection of `char *` elements to join into the new
 * string.
 * @param reducer Reduce function for the collection. This function is called
 * with the `collection` pointer, a standard `reduce_t` reduce callback, and
 * an opaque `void *` reduce context pointer. It is expected to call the
 * callback with a `char *` and the opaque `void *` context pointer.
 * @return A pointer to a new str_t object consisting of the `char *` elements
 * of the `collection` joined together connected by the `glue` string, in the
 * order generated by the `reducer` function. If `collection` or `reducer` are
 * `NULL`, `NULL` is returned.
 */
str_t *_str_join(const char *glue, const void *collection, obj_reduce_t reducer) {
  str_t *ret;
  reduce_ctx ctx;

  if (!collection || !reducer) {
    return NULL;
  }
  if (!glue) {
    glue = "";
  }

  ret = str_create(0);
  reduce_ctx_initialize(&ctx, (char *) glue, ret, NULL);
  reducer((void *) collection, (reduce_t) _str_join_reducer, &ctx);
  return ret;
}

/*
 * =======================================================================
 *
 *     S T R _ T  D I S P O S A L
 *
 * =======================================================================
 */

str_t *str_free(str_t *str) {
  data_release(str);
  return NULL;
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
char *str_reassign(str_t *str) {
  char *ret;

  if (!str) {
    return NULL;
  }
  ret = str->buffer;
  str->bufsize = 0;
  data_release(str);
  return ret;
}

/*
 * =======================================================================
 *
 *     S T R _ T  A C C E S S O R S
 *
 * =======================================================================
 */

int str_is_null(const str_t *str) {
  return str && str->buffer == NULL;
}

int str_is_static(const str_t *str) {
  return str && !str_is_null(str) && (str->bufsize == 0);
}

size_t str_len(const str_t *str) {
  return (str && !str_is_null(str)) ? str->len : -1;
}

char *str_chars(const str_t *str) {
  return (str && !str_is_null(str)) ? str->buffer : NULL;
}

int str_at(const str_t *str, size_t i) {
  int ix = (int) i;
  if (!str || str_is_null(str)) {
    return -1;
  }
  if (ix < 0) {
    ix = str->len + ix;
  }
  return ((ix >= 0) && (ix < str->len)) ? str->buffer[ix] : -1;
}

unsigned int str_hash(const str_t *str) {
  return (str && !str_is_null(str)) ? strhash(str->buffer) : 0;
}

int str_cmp(const str_t *s1, const str_t *s2) {
  if (str_is_null(s1)) {
    return (str_is_null(s2)) ? 0 : -1;
  }
  if (str_is_null(s2)) {
    return 1;
  }
  return strcmp(s1->buffer, s2->buffer);
}

int str_cmp_chars(const str_t *s1, const char *s2) {
  if (str_is_null(s1)) {
    return (s2 == NULL) ? 0 : -1;
  }
  if (s2 == NULL) {
    return 1;
  }
  return strcmp(s1->buffer, s2);
}

int str_ncmp(const str_t *s1, const str_t *s2, size_t numchars) {
  if (numchars == 0) {
    return 0;
  }
  if (str_is_null(s1)) {
    return (str_is_null(s2)) ? 0 : -1;
  }
  if (str_is_null(s2)) {
    return 1;
  }
  return strncmp(s1->buffer, s2->buffer, numchars);
}

int str_ncmp_chars(const str_t *s1, const char *s2, size_t numchars) {
  if (numchars == 0) {
    return 0;
  }
  if (str_is_null(s1)) {
    return (s2 == NULL) ? 0 : -1;
  }
  if (s2 == NULL) {
    return 1;
  }
  return strncmp(s1->buffer, s2, numchars);
}

int str_indexof(const str_t *str, const str_t *pattern) {
  if (!str || str_is_null(str) || !pattern || str_is_null(pattern)) {
    return -1;
  }
  if (pattern->len > str->len) {
    return -1;
  } else {
    return str_indexof_chars(str, str_chars(pattern));
  }
}

int str_indexof_chars(const str_t *str, const char *pattern) {
  char *ptr;

  if (!str || str_is_null(str) || !pattern) {
    return -1;
  }
  if (strlen(pattern) > str->len) {
    return -1;
  } else {
    ptr = strstr(str_chars(str), pattern);
    return (ptr) ? ptr - str_chars(str) : -1;
  }
}

int str_rindexof(const str_t *str, const str_t *pattern) {
  if (!str || str_is_null(str) || !pattern || str_is_null(pattern)) {
    return -1;
  }
  return str_rindexof_chars(str, str_chars(pattern));
}

int str_rindexof_chars(const str_t *str, const char *pattern) {
  char *ptr;
  size_t len;

  if (!str || str_is_null(str) || !pattern) {
    return -1;
  }
  len = strlen(pattern);
  if (len > str->len) {
    return -1;
  } else {
    for (ptr = str->buffer + (str->len - len); ptr != str->buffer; ptr--) {
      if (!strncmp(ptr, pattern, len)) {
        return ptr - str->buffer;
      }
    }
    return -1;
  }
}

int str_rewind(str_t *str) {
  str->pos = 0;
  return 0;
}

int str_read(str_t *str, char *target, size_t num) {
  if ((str->pos + num) > str->len) {
    num = str->len - str->pos;
  }
  if (num > 0) {
    strncpy(target, str->buffer + str->pos, num);
    str->pos = str->pos + num;
    return num;
  } else {
    return 0;
  }
}

int str_peek(const str_t *str) {
  return (str->pos < str->len) ? str->buffer[str->pos] : 0;
}

int str_readchar(str_t *str) {
  int ret;

  ret = str_peek(str);
  str->pos++;
  return ret;
}

int str_skip(str_t *str, int num) {
  if (str->pos + num > str->len) {
    num = str->len - str->pos;
  }
  str->pos += num;
  return num;
}

int str_pushback(str_t *str, size_t num) {
  if (num > str->pos) {
    num = str->pos;
  }
  str->pos -= num;
  return num;
}

int str_readinto(str_t *str, data_t *rdr) {
  return _str_readinto(str, rdr, 0, str->bufsize);
}

int str_fillup(str_t *str, data_t *rdr) {
  return _str_readinto(str, rdr, str->len, str->bufsize - str->len);
}

int str_replenish(str_t *str, data_t *rdr) {
  if (str->pos > str->len) {
    str->pos = str->len;
  }
  if (str->len < str->bufsize) {
    return str_fillup(str, rdr);
  } else {
    _str_expand(str, 0);
    return _str_readinto(str, rdr, str->len, str->len);
  }
}

/**
 * Chop everything from the string before the current read position.
 */
str_t *str_reset(str_t *str) {
  if (str->pos) {
    str_lchop(str, str->pos);
  }
  return str;
}

int str_read_from_stream(str_t *str, void *stream, read_t reader) {
  str_erase(str);
  return _str_read_from_stream(str, stream, reader, 0, str->bufsize);
}

int str_write(str_t *str, char *buf, size_t num) {
  return (str_append_nchars(str, buf, num)) ? num : -1;
}

int str_replace(str_t *str, const char *pat, const char *repl, int max) {
  char *ptr;
  size_t pat_len;
  size_t repl_len;
  size_t offset;
  int diff;
  int num;
  char *end;

  if (!str || str_is_static(str)) {
    return -1;
  }
  if (str_is_null(str)) {
    return 0;
  }
  if (!pat || !repl) {
    return -1;
  }
  if (max == 0) {
    max = -1;
  }
  pat_len = strlen(pat);
  repl_len = strlen(repl);
  diff = (int) repl_len - (int) pat_len;

  for (num = 0, ptr = strstr(str->buffer, pat);
       max && ptr;
       max--, ptr = strstr(ptr + repl_len, pat), num++) {
    offset = ptr - str->buffer;
    if (diff > 0) {
      _str_expand(str, str->len + diff + 1);
      ptr = str->buffer + offset;
    }
    if (diff) {
      end = str->buffer + str->len;
      memmove(ptr + repl_len, ptr + pat_len,
              (str->len - offset) - pat_len + 1);
      str->len += diff;
    }
    memcpy(ptr, repl, repl_len);
    if (str->bufsize > str->len + 1) {
      memset(str->buffer + str->len, 0, str->bufsize - str->len);
    }
    str->pos = 0;
  }
  return num;
}

str_t *str_append_char(str_t *str, int ch) {
  str_t *ret = NULL;

  if (str->bufsize && (ch > 0)) {
    if (_str_expand(str, str->len + 1)) {
      str->buffer[str->len++] = ch;
      ret = str;
    }
  }
  return ret;
}

str_t *str_append_chars(str_t *str, const char *other) {
  if (!str) {
    return NULL;
  }
  if (!other) {
    return str;
  }
  return str_append_nchars(str, other, strlen(other));
}

str_t *str_append_nchars(str_t *str, const char *other, size_t len) {
  if (!str) {
    return NULL;
  }
  if (!str_is_null(str) && str->bufsize == 0) {
    return NULL;
  }
  if (!other) {
    return str;
  }
  len = ((len >= 0) && (len <= strlen(other))) ? len : strlen(other);
  if (_str_expand(str, str_len(str) + len + 1)) {
    strncat(str->buffer, other, len);
    str->len += len;
    str->buffer[str->len] = 0;
  }
  return str;
}

str_t *str_append_printf(str_t *str, const char *other, ...) {
  va_list args;
  str_t *ret;

  va_start(args, other);
  ret = str_append_vprintf(str, other, args);
  va_end(args);
  return ret;
}

str_t *str_append_vprintf(str_t *str, const char *other, va_list args) {
  char *b = NULL;
  size_t len = 0;

  if (!str) {
    return NULL;
  }
  if (!str_is_null(str) && str->bufsize == 0) {
    return NULL;
  }
  if ((len = vasprintf(&b, other, args)) < 0) {
    if (b) free(b);
    return NULL;
  }
  str = str_append_chars(str, b);
  free(b);
  return str;
}

str_t *str_append(str_t *str, const str_t *other) {
  if (!str) {
    return NULL;
  }
  if (!str_is_null(str) && str->bufsize == 0) {
    return NULL;
  }
  if (!other || str_is_null(other)) {
    return str;
  }
  str = str_append_chars(str, str_chars(other));
  return str;
}

str_t *str_chop(str_t *str, int num) {
  if (!str || str_is_static(str)) {
    return NULL;
  }
  if (str_is_null(str)) {
    return str;
  }
  if (num >= (int) str->len) {
    str_erase(str);
  } else if (num > 0) {
    str->len = str->len - num;
    memset(str->buffer + str->len, 0, num);
  }
  if (str->pos > str->len) {
    str->pos = str->len;
  }
  return str;
}

str_t *str_lchop(str_t *str, int num) {
  if (!str || str_is_static(str)) {
    return NULL;
  }
  if (str_is_null(str)) {
    return str;
  }
  if (num >= (int) str_len(str)) {
    str_erase(str);
  } else if (num > 0) {
    memmove(str->buffer, str->buffer + num, str->len - num);
    memset(str->buffer + str->len - num, 0, num);
    str->len = str->len - num;
  }
  str->pos = (str->pos < num) ? 0 : (str->pos - num);
  return str;
}

str_t *str_erase(str_t *str) {
  if (!str || str_is_static(str)) {
    return NULL;
  }
  if (str_is_null(str)) {
    return str;
  }
  memset(str->buffer, 0, str->bufsize);
  str->len = 0;
  str->pos = 0;
  return str;
}

str_t *str_set(str_t *str, size_t index, int ch) {
  if (!str || str_is_static(str) || (index >= str->len)) {
    return NULL;
  }
  str->buffer[index] = ch;
  str->pos = 0;
  if (!ch) {
    str->len = index;
  }
  return str;
}

str_t *str_forcecase(str_t *str, int upper) {
  char c;
  size_t ix;

  if (!str || str_is_static(str)) {
    return NULL;
  }
  for (ix = 0; ix < str->len; ix++) {
    c = str->buffer[ix];
    str->buffer[ix] = upper ? (char) toupper(c) : (char) tolower(c);
  }
  return str;
}

str_t *str_slice(const str_t *str, int from, int upto) {
  str_t *ret;
  size_t len;

  if (!str || str_is_null(str)) {
    return _str_initialize();
  }
  if ((int) from < 0) {
    from = 0;
  }
  if (upto > (int) str_len(str)) {
    upto = (int) str_len(str);
  }
  if (upto < 0) {
    upto = (int) str_len(str) + upto;
  }
  if (upto < from) {
    return str_create(1);
  }
  len = upto - from;
  ret = str_copy_nchars(str_chars(str) + from, len);
  return ret;
}

array_t *str_split(const str_t *str, const char *sep) {
  char *ptr;
  char *sepptr;
  array_t *ret;
  str_t *c;

  ret = data_array_create(4);
  if (!sep) {
    sep = " ";
  }
  if (str && !str_is_null(str) && str_len(str)) {
    ptr = str->buffer;
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

str_t *str_format(const char *fmt, const arguments_t *args) {
  char *copy;
  char *ptr;
  char *specstart;
  long ix;
  str_t *ret = str_create(strlen(fmt));
  char buf[_DEFAULT_SIZE];
  size_t bufsize = _DEFAULT_SIZE;
  char *bigbuf = NULL;
  char *spec = buf;
  size_t len;

  copy = strdup(fmt);
  for (ptr = copy; *ptr; ptr++) {
    if ((*ptr == '$') && (*(ptr + 1) == '{') &&
        ((ptr == copy) || (*(ptr - 1) != '\\'))) {
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
        if (arguments_get_kwarg(args, spec)) {
          str_append_chars(ret, arguments_kwarg_tostring(args, spec));
        } else {
          if (!strtoint(spec, &ix) && (ix >= 0) &&
              (ix < arguments_args_size(args))) {
            str_append_chars(ret,
                             data_tostring(arguments_get_arg(args, (int) ix)));
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

struct _placeholder {
  int num;
  size_t start;
  size_t len;
  int type;
};

str_t *str_vformatf(const char *fmt, va_list args) {
  array_t *arr = NULL;
  arguments_t *arguments;
  char *f = (char *) fmt;
  char *ptr;
  int ix;
  size_t last;
  int ixx;
  int lastix;
  int num = 0;
  char needle[32];
  str_t *ret;
  size_t needlelen;
  struct _placeholder placeholders[99];

  if (!strstr(fmt, "${")) {
    return str_copy_chars(fmt);
  }
  strcpy(needle, "${");
  debug(str, "fmt: %s", fmt);
  do {
    placeholders[num].start = (size_t) -1;

    /* micro-optimization for sprintf(needle+2, "%d", num): */
    if (num < 10) {
      needle[2] = (char) ('0' + num);
      needle[3] = 0;
    } else {
      needle[2] = (char) ('0' + (num / 10));
      needle[3] = (char) ('0' + (num % 10));
      needle[4] = 0;
    }

    sprintf(needle + 2, "%d", num);
    needlelen = strlen(needle);
    for (ptr = strstr(fmt, needle); ptr; ptr = strstr(ptr, needle)) {
      if ((ptr == fmt) || (*(ptr - 1) != '\\')) {
        ptr = ptr + needlelen;
        if (strpbrk(ptr, ":;}") == ptr) {
          placeholders[num].num = num;
          placeholders[num].start = ptr - fmt;
          switch (*ptr) {
            case ':':
            case ';':
              if ((strpbrk(ptr + 1, "adspf") != (ptr + 1)) || (*(ptr + 2) != '}')) {
                return NULL;
              } else {
                placeholders[num].type = *(ptr + 1);
              }
              break;
            case '}':
              placeholders[num].type = 't';
              break;
            default:
              /* Nothing; can't happen */
              break;
          }
          num++;
          break;
        }
      }
      ptr++;
    }
  } while ((ptr) && (num < 100));

  if (num > 0) {
    for (ix = 0; ix < num; ix++) {
      for (ixx = 0, last = (size_t) -1, lastix = -1; ixx < num; ixx++) {
        if ((placeholders[ixx].num != -1) &&
            ((placeholders[ixx].start > last) || (last == -1))) {
          last = placeholders[ixx].start;
          lastix = ixx;
        }
      }
      assert(lastix >= 0);
      assert(lastix < num);
      placeholders[lastix].num = -1;
      if (f == fmt) {
        if (placeholders[lastix].type != 't') {
          f = strdup(fmt);
        }
      }
      ptr = f + last;
      if (placeholders[lastix].type != 't') {
        memmove(ptr, ptr + 2, strlen(ptr + 2) + 1);
      }
    }
    arr = data_array_create(num);
    for (ix = 0; ix < num; ix++) {
      switch (placeholders[ix].type) {
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
        case 't':
        case 'a':
          array_push(arr, va_arg(args, data_t *));
          break;
        default:
          /* Nothing; can't happen */
          break;
      }
    }
  }

  arguments = arguments_create(arr, NULL);
  array_free(arr);
  ret = str_format(f, arguments);
  arguments_free(arguments);
  if (f != fmt) {
    free(f);
  }
  return ret;
}

str_t *str_formatf(const char *fmt, ...) {
  va_list args;
  str_t *ret;

  va_start(args, fmt);
  ret = str_vformatf(fmt, args);
  va_end(args);
  return ret;
}

/* -- S T R I N G   T Y P E   M E T H O D S ------------------------------- */

data_t *_string_at(data_t *self, _unused_ char *name, arguments_t *args) {
  return _str_resolve((str_t *) self, arguments_arg_tostring(args, 0));
}

data_t *_string_slice(data_t *self, char *name, arguments_t *args) {
  data_t *from = arguments_get_arg(args, 0);
  data_t *to = arguments_get_arg(args, 1);
  data_t *ret;
  size_t len = strlen(data_tostring(self));
  size_t i;
  size_t j;
  char *buf;

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

data_t *_string_forcecase(data_t *self, char *name, arguments_t *args) {
  int upper = name[0] == 'u';
  str_t *ret = str_copy_chars(data_tostring(self));

  return (data_t *) str_forcecase(ret, upper);
}

data_t *_string_has(data_t *self, char _unused_ *name, arguments_t *args) {
  char *needle = arguments_arg_tostring(args, 0);

  return int_as_bool(str_indexof_chars((str_t *) self, needle) >= 0);
}

data_t *_string_indexof(data_t *self, char _unused_ *name, arguments_t *args) {
  char *needle = arguments_arg_tostring(args, 0);

  return int_to_data(str_indexof_chars((str_t *) self, needle));
}

data_t *_string_rindexof(data_t *self, char _unused_ *name, arguments_t *args) {
  char *needle = arguments_arg_tostring(args, 0);

  return int_to_data(str_rindexof_chars((str_t *) self, needle));
}

data_t *_string_startswith(data_t *self, char _unused_ *name, arguments_t *args) {
  char *prefix = arguments_arg_tostring(args, 0);
  size_t len;
  size_t prflen;

  len = str_len((str_t *) self);
  prflen = strlen(prefix);
  if (prflen > len) {
    return data_false();
  } else {
    return int_as_bool(strncmp(prefix, str_chars((str_t *) self), prflen) == 0);
  }
}

data_t *_string_endswith(data_t *self, char *name, arguments_t *args) {
  char *suffix = arguments_arg_tostring(args, 0);
  size_t len;
  size_t suflen;
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

data_t *_string_concat(data_t *self, char _unused_ *name, arguments_t *args) {
  str_t *ret = str_copy_chars(data_tostring(self));
  int ix;
  size_t len = str_len(ret);

  for (ix = 0; ix < arguments_args_size(args); ix++) {
    len += strlen(arguments_arg_tostring(args, ix));
  }
  _str_expand(ret, len);
  for (ix = 0; ix < arguments_args_size(args); ix++) {
    str_append_chars(ret, arguments_arg_tostring(args, ix));
  }
  return (data_t *) ret;
}

data_t *_string_repeat(data_t *self, char _unused_ *name, arguments_t *args) {
  char *s = data_tostring(self);
  str_t *ret = str_copy_chars(s);
  size_t len = str_len(ret);
  int numval = data_intval(arguments_get_arg(args, 0));
  int ix;

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

data_t *_string_split(data_t *self, char _unused_ *name, arguments_t *args) {
  array_t *split = array_split(data_tostring(self),
                               arguments_arg_tostring(args, 0));
  data_t *ret = (data_t *) str_array_to_datalist(split);

  array_free(split);
  return ret;
}
