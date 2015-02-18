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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <array.h>
#include <str.h>

static str_t *      _str_initialize(void);
static str_t *      _str_expand(str_t *, int);
static reduce_ctx * _str_join_reducer(char *, reduce_ctx *);

#define _DEFAULT_SIZE   32

/*
 * str_t static functions
 */

str_t * _str_initialize(void) {
  str_t *ret;

  ret = NEW(str_t);
  ret -> read_fnc = (read_t) str_read;
  ret -> free = (free_t) str_free;
  ret -> buffer = NULL;
  ret -> pos = 0;
  ret -> len = 0;
  ret -> bufsize = 0;
  return ret;
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

/*
 * str_t public functions
 */

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

str_t * str_copy(str_t *str) {
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

void str_free(str_t *str) {
  if (str) {
    if (str -> bufsize) {
      free(str -> buffer);
    }
    free(str);
  }
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

int str_readinto(str_t *str, reader_t *rdr) {
  int ret;

  str_erase(str);
  ret = reader_read(rdr, str -> buffer, str -> bufsize);
  if (ret >= 0) {
    str -> buffer[ret] = '\0';
    str -> len = ret;
  }
  return ret;
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
    (free_t) str_free);

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
