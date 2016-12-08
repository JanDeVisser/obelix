/*
 * core.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "libcore.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <resolve.h>

typedef struct _memmonitor {
  size_t  blocksize;
  int     count;
} memmonitor_t;

static void        _outofmemory(int);

static type_t _type_str = {
  .hash     = (hash_t) strhash,
  .tostring = (tostring_t) chars,
  .copy     = (copy_t) strdup,
  .free     = (free_t) free,
  .cmp      = (cmp_t) strcmp
};

static type_t _type_int = {
  .hash     = (hash_t) hashlong,
  .tostring = (tostring_t) oblcore_itoa,
  .copy     = NULL,
  .free     = NULL,
  .cmp      = NULL
};

type_t *type_str = NULL;
type_t *type_int = NULL;

static application_t *_app = NULL;

/* ------------------------------------------------------------------------ */

static void _outofmemory(int sz) {
  error("Could not allocate %d bytes. Out of Memory. Terminating...", sz);
  exit(1);
}

application_t * application_init(const char *appname, int argc, char **argv) {
  assert(!_app);
  type_str = &_type_str;
  type_int = &_type_int;
  logging_init();
  data_init();
  typedescr_init();

  _app = NEW(application_t);
  _app -> name = strdup(appname);
  return _app;
}

void application_terminate(void) {
  if (_app) {
    free(_app -> name);
  }
}

void * _new(int sz) {
  void *ret;

  ret = malloc(sz);
  if (sz && !ret) {
    _outofmemory(sz);
  } else {
    memset(ret, 0, sz);
  }
  return ret;
}

void * new_array(int num, int sz) {
  void * ret = calloc(num, sz);

  if (num && !ret) {
    _outofmemory(sz * num);
  }
  return ret;
}

void * new_ptrarray(int num) {
  return new_array(num, sizeof(void *));
}

void * resize_block(void *block, int newsz, int oldsz) {
  void *ret;

  if (block) {
    ret = realloc(block, newsz);
    if (newsz && !ret) {
      _outofmemory(newsz);
    } else if (oldsz) {
      memset((char *) ret + oldsz, 0, newsz - oldsz);
    }
  } else {
    ret = _new(newsz);
  }
  return ret;
}

void * resize_ptrarray(void *array, int newsz, int oldsz) {
  return resize_block(array, newsz * sizeof(void *), oldsz * sizeof(void *));
}

#ifndef HAVE_ASPRINTF
int asprintf(char **strp, const char *fmt, ...) {
  va_list args;
  int     ret;

  va_start(args, fmt);
  ret = vasprintf(strp, fmt, args);
  va_end(args);
  return ret;
}
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char **strp, const char *fmt, va_list args) {
  va_list  copy;
  int      ret;
  char    *str;

  va_copy(copy, args);
  ret = vsnprintf(NULL, 0, fmt, copy);
  va_end(copy);
  str = (char *) new(ret + 1);
  if (!str) {
    ret = -1;
  } else {
    vsnprintf(str, ret, fmt, args);
    *strp = str;
  }
  return ret;
}
#endif

unsigned int hash(const void *buf, size_t size) {
  int            hash = 5381;
  size_t         i;
  int            c;
  unsigned char *data = (unsigned char *) buf;

  for (i = 0; i < size; i++) {
    c = *data++;
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

unsigned int hashptr(const void *ptr) {
  return hash(&ptr, sizeof(void *));
}

unsigned int hashlong(long val) {
  return hash(&val, sizeof(long));
}

unsigned int hashdouble(double val) {
  return hash(&val, sizeof(double));
}

unsigned int hashblend(unsigned int h1, unsigned int h2) {
  return (h1 << 1) + h1 + h2;
}

/*
 * code_label_t public functions
 */

char * label_for_code(code_label_t *table, int code) {
  assert(table);
  while (table -> label) {
    if (table -> code == code) {
      return table -> label;
    } else {
      table++;
    }
  }
  return NULL;
}

int code_for_label(code_label_t *table, char *label) {
  assert(table);
  assert(label);
  while (table -> label) {
    if (!strcmp(table -> label, label)) {
      return table -> code;
    } else {
      table++;
    }
  }
  return -1;
}

/*
 * reduce_ctx public functions
 */


reduce_ctx * reduce_ctx_create(void *user, void *data, void_t fnc) {
  reduce_ctx *ctx;

  ctx = NEW(reduce_ctx);
  if (ctx) {
    ctx -> data = data;
    ctx -> fnc = fnc;
    ctx -> user = user;
  }
  return ctx;
}

reduce_ctx * collection_hash_reducer(void *elem, reduce_ctx *ctx) {
  hash_t hash = (hash_t) ctx -> fnc;
  ctx -> longdata += hash(elem);
  return ctx;
}

reduce_ctx * collection_add_all_reducer(void *data, reduce_ctx *ctx) {
  ((reduce_t) ctx -> fnc)(ctx -> obj, data);
  return ctx;
}

visit_t collection_visitor(void *data, visit_t visitor) {
  visitor(data);
  return visitor;
}
