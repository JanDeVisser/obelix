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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <data.h>

static void            _outofmemory(const char *, size_t) _noreturn_;
#ifndef _MSC_VER
#define outofmemory(i) _outofmemory(__func__, (i))
#else
#define outofmemory(i) _outofmemory(__FUNCTION__, (i))
#endif

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

static void _outofmemory(const char *func, size_t sz _unused_) {
  fputs("Could not allocate memory in function ", stderr);
  fputs(func, stderr);
  fputs(". Out of Memory. Terminating...\n", stderr);
  abort();
}

application_t * application_init(const char *appname, int argc, char **argv) {
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

void * _new(size_t sz) {
  void *ret;

  ret = malloc(sz);
  if (sz && !ret) {
    outofmemory(sz);
  } else {
    memset(ret, 0, sz);
  }
  return ret;
}

void * new_array(size_t num, size_t sz) {
  void * ret = calloc(num, sz);

  if (num && !ret) {
    outofmemory(sz * num);
  }
  return ret;
}

void * new_ptrarray(size_t num) {
  return new_array(num, sizeof(void *));
}

void * resize_block(void *block, size_t newsz, size_t oldsz) {
  void *ret;

  if (block) {
    ret = realloc(block, newsz);
    if (newsz && !ret) {
      outofmemory(newsz);
    } else if (oldsz) {
      memset((char *) ret + oldsz, 0, newsz - oldsz);
    }
  } else {
    ret = _new(newsz);
  }
  return ret;
}

void * resize_ptrarray(void *array, size_t newsz, size_t oldsz) {
  return resize_block(array, newsz * sizeof(void *), oldsz * sizeof(void *));
}


#ifdef asprintf
  #undef asprintf
#endif
#ifdef vasprintf
  #undef vasprintf
#endif

int oblcore_asprintf(char **strp, const char *fmt, ...) {
  va_list args;
  int     ret;

  va_start(args, fmt);
  ret = oblcore_vasprintf(strp, fmt, args);
  va_end(args);
  return ret;
}

#ifndef HAVE_VASPRINTF
int oblcore_vasprintf(char **strp, const char *fmt, va_list args) {
  va_list  copy;
  size_t   ret;
  char    *str;

  va_copy(copy, args);
  ret = vsnprintf(NULL, 0, fmt, copy);
  va_end(copy);
  str = stralloc(ret);
  if (!str) {
    outofmemory(ret);
  } else {
    vsnprintf(str, ret, fmt, args);
    *strp = str;
  }
  return ret;
}
#else
int oblcore_vasprintf(char **strp, const char *fmt, va_list args) {
  size_t ret;

  ret = vasprintf(strp, fmt, args);
  if (ret < 0) {
    outofmemory(ret);
  }
  return ret;
}
#endif

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

char * labels_for_bitmap(code_label_t *table, int bitmap, char *buf, size_t maxlen) {
  int     bit;
  int     ix;
  size_t  len = 0;
  char   *label;

  buf[0] = 0;
  for (bit = 1, ix = 8 * sizeof(int) - 1; ix >= 0; ix--, bit <<= 1) {
    if (bit & bitmap) {
      if (len) {
        if (len < maxlen - 3) {
          strcat(buf, " | ");
        } else {
          break;
        }
        len += 3;
      }
      label = label_for_code(table, bit);
      len += strlen(label);
      if (len <= maxlen) {
        strcat(buf, label);
      } else {
        break;
      }
    }
  }
  return buf;
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
  return reduce_ctx_initialize(ctx, user, data, fnc);
}

reduce_ctx * reduce_ctx_initialize(reduce_ctx *ctx, void *user, void *data, void_t fnc) {
  if (ctx) {
    ctx -> data = data;
    ctx -> fnc = fnc;
    ctx -> user = user;
  }
  return ctx;
}

reduce_ctx * collection_hash_reducer(void *elem, reduce_ctx *ctx) {
  hash_t hash = (hash_t) ctx -> fnc;
  ctx -> longdata = hashblend(hash(elem), (unsigned int) ctx -> longdata);
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
