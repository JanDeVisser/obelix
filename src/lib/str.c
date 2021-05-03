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

static data_t * _str_create(int, va_list);
static void     _str_free(str_t *);
static data_t * _str_cast(str_t *, int);
static str_t *  _str_parse(char *s);
static data_t * _str_resolve(str_t *, char *);
static char *   _str_encode(str_t *);
static str_t *  _str_serialize(str_t *);
static data_t * _str_deserialize(str_t *);

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
  {.id = FunctionFactory,     .fnc = (void_t) _str_create},
  {.id = FunctionCmp,         .fnc = (void_t) str_cmp},
  {.id = FunctionFree,        .fnc = (void_t) _str_free},
  {.id = FunctionToString,    .fnc = (void_t) str_chars},
  {.id = FunctionParse,       .fnc = (void_t) _str_parse},
  {.id = FunctionCast,        .fnc = (void_t) _str_cast},
  {.id = FunctionHash,        .fnc = (void_t) str_hash},
  {.id = FunctionLen,         .fnc = (void_t) str_len},
  {.id = FunctionRead,        .fnc = (void_t) str_read},
  {.id = FunctionWrite,       .fnc = (void_t) str_write},
  {.id = FunctionResolve,     .fnc = (void_t) _str_resolve},
  {.id = FunctionEncode,      .fnc = (void_t) _str_encode},
  {.id = FunctionSerialize,   .fnc = (void_t) _str_serialize},
  {.id = FunctionDeserialize, .fnc = (void_t) _str_deserialize},
  {.id = FunctionNone,        .fnc = NULL}
};

static methoddescr_t _methods_String[] = {
  {.type = String, .name = "at",         .method = _string_at,         .argtypes = {Int, NoType, NoType},    .minargs = 1, .varargs = 0},
  {.type = String, .name = "slice",      .method = _string_slice,      .argtypes = {Int, NoType, NoType},    .minargs = 1, .varargs = 1},
  {.type = String, .name = "upper",      .method = _string_forcecase,  .argtypes = {NoType, NoType, NoType}, .minargs = 0, .varargs = 0},
  {.type = String, .name = "lower",      .method = _string_forcecase,  .argtypes = {NoType, NoType, NoType}, .minargs = 0, .varargs = 0},
  {.type = String, .name = "has",        .method = _string_has,        .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "indexof",    .method = _string_indexof,    .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "rindexof",   .method = _string_rindexof,   .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "startswith", .method = _string_startswith, .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "endswith",   .method = _string_endswith,   .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = String, .name = "+",          .method = _string_concat,     .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 1},
  {.type = String, .name = "concat",     .method = _string_concat,     .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 1},
  {.type = String, .name = "*",          .method = _string_repeat,     .argtypes = {Int, NoType, NoType},    .minargs = 1, .varargs = 0},
  {.type = String, .name = "repeat",     .method = _string_repeat,     .argtypes = {Int, NoType, NoType},    .minargs = 1, .varargs = 0},
  {.type = String, .name = "split",      .method = _string_split,      .argtypes = {String, NoType, NoType}, .minargs = 1, .varargs = 0},
  {.type = NoType, .name = NULL,         .method = NULL,               .argtypes = {NoType, NoType, NoType}, .minargs = 0, .varargs = 0}
};

extern int data_debug;
int str_debug;

/* ------------------------------------------------------------------------ */

void str_init(void) {
  logging_register_module(str);
  builtin_typedescr_register(String, "string", str_t);
}

/* -- S T R I N G   D A T A   F U N C T I O N S --------------------------- */

void _str_free(str_t *s) {
  if (s) {
    if (s->bufsize) {
      free(s->buffer);
    }
  }
}

data_t *_str_resolve(str_t *s, char *slice) {
  int sz = (int) str_len(s);
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
      buf[0] = (char) str_at(s, ix);
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
  char *s = str_chars(data);

  if (totype == Bool) {
    return int_as_bool(s && strlen(s));
  } else {
    type = typedescr_get(totype);
    assert(type);
    parse = (parse_t) typedescr_get_function(type, FunctionParse);
    return (parse) ? parse(str_chars(data)) : NULL;
  }
}

static char *_escaped_chars = "\"\\\b\f\n\r\t";
static char *_escape_codes = "\"\\bfnrt";

str_t *_str_parse(char *s) {
  char *buf = stralloc(strlen(s));
  char *ptr;
  char *escptr;

  strcpy(buf, s);
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

str_t *_str_serialize(str_t *s) {
  char *encoded = _str_encode(s);
  size_t len = strlen(encoded);
  str_t *ret = str_create(len + 2);

  *ret->buffer = '"';
  strcpy(ret->buffer + 1, encoded);
  ret->buffer[len + 1] = '"';
  ret->buffer[len + 2] = 0;
  ret->len = len + 2;
  return ret;
}

str_t *_str_strip_quotes(str_t *s) {
  str_t *ret;

  if (str_len(s) && (str_at(s, 0) == '"') && (str_at(s, -1) == '"')) {
    ret = str_duplicate(s);
    str_chop(s, 1);
    str_lchop(s, 1);
  } else {
    ret = str_copy(s);
  }
  return ret;
}

data_t *_str_deserialize(str_t *s) {
  if (!strcmp(s->buffer, "null")) {
    return data_null();
  } else if (strcmp(s->buffer, "true") != 0) {
    return data_true();
  } else if (strcmp(s->buffer, "false") != 0) {
    return data_true();
  } else {
    return (data_t *) _str_strip_quotes(s);
  }
}

char *_str_encode(str_t *s) {
  char *buf;
  int len = 0;
  char *ptr;
  char *encoded;
  char *encptr;
  char *escptr;

  buf = str_chars(s);
  for (ptr = s->buffer; *ptr; ptr++) {
    len += (strchr(_escaped_chars, *ptr)) ? 2 : 1;
  }
  encoded = encptr = stralloc(len);
  for (ptr = s->buffer; *ptr; ptr++) {
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
  return (data_t *) str(va_arg(args, char *));
}

/* -- S T R _ T   S T A T I C   F U N C T I O N S ------------------------- */

str_t *_str_allocate(str_t *s, size_t size) {
  if (!s) {
    return NULL;
  }
  size = size ? size : _DEFAULT_SIZE;
  if (s->bufsize > 0) {
    return _str_expand(s, size);
  }
  s->buffer = stralloc(size);
  if (!s->buffer) {
    s = str_free(s);
  } else {
    memset(s->buffer, 0, size);
    s->bufsize = size;
    s->len = 0;
  }
  return s;
}

str_t *_str_inflate(str_t *s, const char *buffer, size_t len) {
  char *b;

  if (!s || s->buffer != NULL) {
    return NULL;
  }
  if (buffer) {
    len = ((len >= 0) && (len <= strlen(buffer))) ? len : strlen(buffer);
    b = (char *) _new(len + 1);
    if (!b) {
      s = str_free(s);
      return s;
    }
    strncpy(b, buffer, len);
    *(b + len) = 0;
    s->buffer = b;
    s->len = len;
    s->bufsize = s->len + 1;
  }
  return s;
}

str_t *_str_wrap(str_t *s, const char *buffer) {
  if (!s || s->buffer != NULL) {
    return NULL;
  }
  if (s && buffer) {
    s->buffer = (char *) buffer;
    s->len = strlen(buffer);
  }
  return s;
}

str_t *_str_adopt(str_t *s, char *buffer) {
  if (!s || s->buffer != NULL) {
    return NULL;
  }
  if (s && buffer) {
    s->buffer = buffer;
    s->len = strlen(buffer);
    s->bufsize = s->len + 1;
  }
  return s;
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

str_t *_str_expand(str_t *s, size_t targetlen) {
  size_t newsize;
  char *oldbuf;
  str_t *ret = s;

  if (!s || (targetlen < 0)) {
    return NULL;
  }
  if (s->bufsize == 0) {
    return _str_allocate(s, targetlen);
  }
  if (targetlen <= s->bufsize) {
    return s;
  }
  if (!targetlen) {
    targetlen = s->bufsize;
  }
  size_t target = (targetlen > 0) ? targetlen : s->bufsize;
  if (s->bufsize < (targetlen + 1)) {
    for (newsize = (size_t) ((double) (s->bufsize) * 1.6);
         newsize < (targetlen + 1);
         newsize = (size_t) ((double) newsize * 1.6));
    oldbuf = s->buffer;
    s->buffer = realloc(s->buffer, newsize);
    if (s->buffer) {
      memset(s->buffer + s->len, 0, newsize - s->len);
      s->bufsize = newsize;
    } else {
      s->buffer = oldbuf;
      ret = NULL;
    }
  }
  return ret;
}

size_t _str_read_from_stream(str_t *s, void *stream, read_t reader, size_t pos, size_t num) {
  size_t ret;

  if (pos >= s->bufsize) {
    return -1;
  }
  if ((pos + num) > s->bufsize) {
    num = s->bufsize - pos;
  }
  ret = reader(stream, s->buffer + pos, (int) num);
  if (ret < 0) {
    return -1;
  } else {
    if ((pos + ret) < s->bufsize) {
      s->buffer[pos + ret] = 0;
    }
    if (pos <= s->len) {
      s->len += ret;
    }
    return ret;
  }
}

size_t _str_readinto(str_t *s, data_t *rdr, size_t pos, size_t num) {
  typedescr_t *type = data_typedescr(rdr);
  read_t fnc;

  fnc = (read_t) typedescr_get_function(type, FunctionRead);
  if (fnc) {
    return _str_read_from_stream(s, rdr, fnc, pos, num);
  } else {
    return -1;
  }
}

/* -- S T R _ T   P U B L I C   F U N C T I O N S ------------------------- */

str_t * str_create(size_t size) {
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

str_t * str_wrap(const char *buffer) {
  str_t *s;

  s = _str_initialize();
  if (s && buffer) {
    if (!_str_wrap(s, buffer)) {
      s = str_free(s);
    }
  }
  return s;
}

str_t * str_adopt(char *buffer) {
  str_t *s;

  s = _str_initialize();
  if (s && buffer) {
    if (!_str_adopt(s, buffer)) {
      s = str_free(s);
    }
  }
  return s;
}

str_t * str_printf(const char *fmt, ...) {
  str_t *ret;
  va_list args;

  va_start(args, fmt);
  ret = str_vprintf(fmt, args);
  va_end(args);
  return ret;
}

str_t * str_vprintf(const char *fmt, va_list args) {
  str_t *s;
  char *buffer = NULL;

  s = _str_initialize();
  if (s) {
    if ((vasprintf(&buffer, fmt, args) < 0) || !_str_adopt(s, buffer)) {
      s = str_free(s);
      free(buffer);
    }
  }
  return s;
}

str_t * str(const char *buffer) {
  return str_n(buffer, (buffer) ? strlen(buffer) : 0);
}

str_t * str_n(const char *buffer, size_t len) {
  str_t *s;
  char *b;

  s = _str_initialize();
  if (s && buffer) {
    if (!_str_inflate(s, buffer, len)) {
      s = str_free(s);
    }
  }
  return s;
}

str_t * str_from_data(data_t *data) {
  if (data && (data != data_null())) {
    return (data_is_string(data))
           ? (str_t *) data
           : str(data_tostring(data));
  } else {
    return _str_initialize();
  }
}

str_t * str_duplicate(const str_t *s) {
  str_t *ret;
  char *b;

  ret = _str_initialize();
  if (ret && s && !str_is_null(s)) {
    b = strdup(str_chars(s));
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
str_t * _str_join(const char *glue, const void *collection, obj_reduce_t reducer) {
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

str_t * str_free(str_t *s) {
  data_release(s);
  return NULL;
}

/**
 * Frees the str_t object, but does not free the actual underlying char buffer,
 * instead making that available for use by the caller. Note that the caller
 * now becomes responsible for freeing this buffer (if the str_t was not
 * obtained using a str_wrap).
 *
 * @param s String to free and return the buffer of.
 * @return char buffer of the str_t object passed in.
 */
char * str_reassign(str_t *s) {
  char *ret;

  if (!s) {
    return NULL;
  }
  ret = s->buffer;
  s->bufsize = 0;
  data_release(s);
  return ret;
}

/*
 * =======================================================================
 *
 *     S T R _ T  A C C E S S O R S
 *
 * =======================================================================
 */

int str_is_null(const str_t *s) {
  return s && s->buffer == NULL;
}

int str_is_static(const str_t *s) {
  return s && !str_is_null(s) && (s->bufsize == 0);
}

size_t str_len(const str_t *s) {
  if (!s || str_is_null(s)) {
    return -1;
  }
  return (!str_is_static(s)) ? s->len : strlen(s->buffer);
}

char * str_chars(const str_t *s) {
  return (s && !str_is_null(s)) ? s->buffer : NULL;
}

int str_at(const str_t *s, size_t i) {
  int ix = (int) i;
  if (!s || str_is_null(s)) {
    return -1;
  }
  if (ix < 0) {
    ix = s->len + ix;
  }
  return ((ix >= 0) && (ix < s->len)) ? s->buffer[ix] : -1;
}

unsigned int str_hash(const str_t *s) {
  return (s && !str_is_null(s)) ? strhash(s->buffer) : 0;
}

int str_cmp(const str_t *s1, const str_t *s2) {
  if (!s1) {
    return (s2) ? -1 : 0;
  }
  if (!s2) {
    return 1;
  }
  if (str_is_null(s1)) {
    return (str_is_null(s2)) ? 0 : -1;
  }
  if (str_is_null(s2)) {
    return 1;
  }
  return strcmp(s1->buffer, s2->buffer);
}

int str_cmp_chars(const str_t *s1, const char *s2) {
  if (!s1) {
    return (!s2) ? 0 : -1;
  }
  if (str_is_null(s1)) {
    return (!s2) ? 1 : -1;
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
  if (!s1) {
    return (s2) ? -1 : 0;
  }
  if (!s2) {
    return 1;
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
  if (!s1) {
    return (!s2) ? 0 : -1;
  }
  if (str_is_null(s1)) {
    return (!s2) ? 1 : -1;
  }
  if (s2 == NULL) {
    return 1;
  }
  return strncmp(s1->buffer, s2, numchars);
}

int str_indexof(const str_t *s, const str_t *pattern) {
  if (!s || str_is_null(s) || !pattern || str_is_null(pattern)) {
    return -1;
  }
  if (pattern->len > s->len) {
    return -1;
  } else {
    return str_indexof_chars(s, str_chars(pattern));
  }
}

int str_indexof_chars(const str_t *s, const char *pattern) {
  char *ptr;

  if (!s || str_is_null(s) || !pattern) {
    return -1;
  }
  if (strlen(pattern) > s->len) {
    return -1;
  } else {
    ptr = strstr(str_chars(s), pattern);
    return (ptr) ? ptr - str_chars(s) : -1;
  }
}

int str_rindexof(const str_t *s, const str_t *pattern) {
  if (!s || str_is_null(s) || !pattern || str_is_null(pattern)) {
    return -1;
  }
  return str_rindexof_chars(s, str_chars(pattern));
}

int str_rindexof_chars(const str_t *s, const char *pattern) {
  char *ptr;
  size_t len;

  if (!s || str_is_null(s) || !pattern) {
    return -1;
  }
  len = strlen(pattern);
  if (len > s->len) {
    return -1;
  } else {
    for (ptr = s->buffer + (s->len - len); ptr != s->buffer; ptr--) {
      if (!strncmp(ptr, pattern, len)) {
        return ptr - s->buffer;
      }
    }
    return -1;
  }
}

/* -- B U F F E R E D  R E A D S / W R I T E S --------------------------- */

int str_rewind(str_t *s) {
  if (!s) {
    return -1;
  }
  s->pos = 0;
  return 0;
}

int str_read(str_t *s, char *target, int num) {
  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  if ((num < 0) || ((s->pos + num) > s->len)) {
    num = s->len - s->pos;
  }
  if (num > 0) {
    strncpy(target, s->buffer + s->pos, num);
    s->pos = s->pos + num;
    return num;
  } else {
    return 0;
  }
}

int str_peek(const str_t *s) {
  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  return (s->pos < s->len) ? s->buffer[s->pos] : 0;
}

int str_readchar(str_t *s) {
  int ret;

  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  ret = str_peek(s);
  s->pos++;
  return ret;
}

int str_skip(str_t *s, int num) {
  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  if (s->pos + num > s->len) {
    num = s->len - s->pos;
  }
  s->pos += num;
  return num;
}

int str_pushback(str_t *s, size_t num) {
  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  if ((int) num < 0) {
    return -1;
  }
  if (num > s->pos) {
    num = s->pos;
  }
  s->pos -= num;
  return num;
}

int str_readinto(str_t *s, data_t *rdr) {
  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  return _str_readinto(s, rdr, 0, s->bufsize);
}

int str_fillup(str_t *s, data_t *rdr) {
  if (!s) {
    return -1;
  }
  if (str_is_null(s)) {
    return 0;
  }
  return _str_readinto(s, rdr, s->len, s->bufsize - s->len);
}

int str_replenish(str_t *s, data_t *rdr) {
  if (!s) {
    return -1;
  }
  if (s->bufsize == 0) {
    return -1;
  }
  if (s->pos > s->len) {
    s->pos = s->len;
  }
  if (s->len < s->bufsize) {
    return str_fillup(s, rdr);
  } else {
    _str_expand(s, 0);
    return _str_readinto(s, rdr, s->len, s->len);
  }
}

/**
 * Chop everything from the string before the current read position.
 */
str_t * str_reset(str_t *s) {
  if (s->pos) {
    str_lchop(s, s->pos);
  }
  return s;
}

int str_read_from_stream(str_t *s, void *stream, read_t reader) {
  str_erase(s);
  return _str_read_from_stream(s, stream, reader, 0, s->bufsize);
}

int str_write(str_t *s, char *buf, size_t num) {
  return (str_append_nchars(s, buf, num)) ? num : -1;
}

int str_replace(str_t *s, const char *pat, const char *repl, int max) {
  char *ptr;
  size_t pat_len;
  size_t repl_len;
  size_t offset;
  int diff;
  int num;
  char *end;

  if (!s || str_is_static(s)) {
    return -1;
  }
  if (str_is_null(s)) {
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

  for (num = 0, ptr = strstr(s->buffer, pat);
       max && ptr;
       max--, ptr = strstr(ptr + repl_len, pat), num++) {
    offset = ptr - s->buffer;
    if (diff > 0) {
      _str_expand(s, s->len + diff + 1);
      ptr = s->buffer + offset;
    }
    if (diff) {
      end = s->buffer + s->len;
      memmove(ptr + repl_len, ptr + pat_len,
              (s->len - offset) - pat_len + 1);
      s->len += diff;
    }
    memcpy(ptr, repl, repl_len);
    if (s->bufsize > s->len + 1) {
      memset(s->buffer + s->len, 0, s->bufsize - s->len);
    }
    s->pos = 0;
  }
  return num;
}

str_t * str_append_char(str_t *s, int ch) {
  str_t *ret = NULL;

  if (s->bufsize && (ch > 0)) {
    if (_str_expand(s, s->len + 1)) {
      s->buffer[s->len++] = ch;
      ret = s;
    }
  }
  return ret;
}

str_t * str_append_chars(str_t *s, const char *other) {
  if (!s) {
    return NULL;
  }
  if (!other) {
    return s;
  }
  return str_append_nchars(s, other, strlen(other));
}

str_t * str_append_nchars(str_t *s, const char *other, size_t len) {
  if (!s) {
    return NULL;
  }
  if (!str_is_null(s) && s->bufsize == 0) {
    return NULL;
  }
  if (!other) {
    return s;
  }
  len = ((len >= 0) && (len <= strlen(other))) ? len : strlen(other);
  if (_str_expand(s, str_len(s) + len + 1)) {
    strncat(s->buffer, other, len);
    s->len += len;
    s->buffer[s->len] = 0;
  }
  return s;
}

str_t * str_append_printf(str_t *s, const char *other, ...) {
  va_list args;
  str_t *ret;

  va_start(args, other);
  ret = str_append_vprintf(s, other, args);
  va_end(args);
  return ret;
}

str_t * str_append_vprintf(str_t *s, const char *other, va_list args) {
  char *b = NULL;
  size_t len = 0;

  if (!s) {
    return NULL;
  }
  if (!str_is_null(s) && s->bufsize == 0) {
    return NULL;
  }
  if ((len = vasprintf(&b, other, args)) < 0) {
    if (b) free(b);
    return NULL;
  }
  s = str_append_chars(s, b);
  free(b);
  return s;
}

str_t * str_append(str_t *s, const str_t *other) {
  if (!s) {
    return NULL;
  }
  if (!str_is_null(s) && s->bufsize == 0) {
    return NULL;
  }
  if (!other || str_is_null(other)) {
    return s;
  }
  s = str_append_chars(s, str_chars(other));
  return s;
}

str_t * str_chop(str_t *s, int num) {
  if (!s || str_is_static(s)) {
    return NULL;
  }
  if (str_is_null(s)) {
    return s;
  }
  if (num >= (int) s->len) {
    str_erase(s);
  } else if (num > 0) {
    s->len = s->len - num;
    memset(s->buffer + s->len, 0, num);
  }
  if (s->pos > s->len) {
    s->pos = s->len;
  }
  return s;
}

str_t * str_lchop(str_t *s, int num) {
  if (!s || str_is_static(s)) {
    return NULL;
  }
  if (str_is_null(s)) {
    return s;
  }
  if (num >= (int) str_len(s)) {
    str_erase(s);
  } else if (num > 0) {
    memmove(s->buffer, s->buffer + num, s->len - num);
    memset(s->buffer + s->len - num, 0, num);
    s->len = s->len - num;
  }
  s->pos = (s->pos < num) ? 0 : (s->pos - num);
  return s;
}

str_t * str_erase(str_t *s) {
  if (!s || str_is_static(s)) {
    return NULL;
  }
  if (str_is_null(s)) {
    return s;
  }
  memset(s->buffer, 0, s->bufsize);
  s->len = 0;
  s->pos = 0;
  return s;
}

str_t * str_set(str_t *s, size_t index, int ch) {
  if (!s || str_is_static(s) || (index >= s->len)) {
    return NULL;
  }
  s->buffer[index] = ch;
  s->pos = 0;
  if (!ch) {
    s->len = index;
  }
  return s;
}

str_t * str_forcecase(str_t *s, int upper) {
  char c;
  size_t ix;

  if (!s || str_is_static(s)) {
    return NULL;
  }
  for (ix = 0; ix < s->len; ix++) {
    c = s->buffer[ix];
    s->buffer[ix] = upper ? (char) toupper(c) : (char) tolower(c);
  }
  return s;
}

str_t * str_slice(const str_t *s, int from, int upto) {
  str_t *ret;
  size_t len;

  if (!s || str_is_null(s)) {
    return _str_initialize();
  }
  if ((int) from < 0) {
    from = 0;
  }
  if (upto > (int) str_len(s)) {
    upto = (int) str_len(s);
  }
  if (upto < 0) {
    upto = (int) str_len(s) + upto;
  }
  if (upto < from) {
    return str_create(1);
  }
  len = upto - from;
  ret = str_n(str_chars(s) + from, len);
  return ret;
}

array_t *str_split(const str_t *s, const char *sep) {
  char *ptr;
  char *sepptr;
  array_t *ret;
  str_t *c;

  ret = data_array_create(4);
  if (!sep) {
    sep = " ";
  }
  if (s && !str_is_null(s) && str_len(s)) {
    ptr = s->buffer;
    for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
      c = str_n(ptr, sepptr - ptr);
      array_push(ret, c);
      ptr = sepptr + strlen(sep);
    }
    c = str(ptr);
    array_push(ret, c);
  }
  return ret;
}

str_t * str_format(const char *fmt, const arguments_t *args) {
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
          bigbuf = (char *) _new(bufsize);
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

str_t * str_vformatf(const char *fmt, va_list args) {
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
    return str(fmt);
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

str_t * str_formatf(const char *fmt, ...) {
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
  str_t *ret = str(data_tostring(self));

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
  str_t *ret = str(data_tostring(self));
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
  str_t *ret = str(s);
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
