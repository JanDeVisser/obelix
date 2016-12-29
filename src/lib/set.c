/*
 * set.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>
#include <stdlib.h>

#include "libcore.h"
#include <set.h>

static void *       _set_copy(set_t *, void *);
static reduce_ctx * _set_reduce_reducer(entry_t *, reduce_ctx *);
static void *       _set_reduce(set_t *, reduce_t, void *, char *);
static reduce_ctx * _set_visitor(entry_t *, reduce_ctx *);
static set_t *      _set_union_reducer(void *, set_t *);
static reduce_ctx * _set_intersect_reducer(void *, reduce_ctx *);
static set_t *      _set_remover(void *, set_t *);
static reduce_ctx * _set_minus_reducer(void *, reduce_ctx *);
static reduce_ctx * _set_disjoint_reducer(void *, reduce_ctx *);
static reduce_ctx * _set_subsetof_reducer(void *, reduce_ctx *);
static void **      _set_find_reducer(void *, void **);

// --------------------
// set_t static methods

void * _set_copy(set_t *set, void *elem) {
  return (set -> dict -> key_type.copy) ? set -> dict -> key_type.copy(elem) : elem;
}

reduce_ctx * _set_reduce_reducer(entry_t *entry, reduce_ctx *ctx) {
  void   *elem;
  set_t  *set;
  free_t  f;
  char   *c;
  int     type;

  set = (set_t *) ctx -> obj;
  f = NULL;
  type = ((char *) ctx -> user)[0];
  if ((type == 'c') || (type == 's')) {
    c = (set -> dict -> key_type.tostring)
          ? set -> dict -> key_type.tostring(entry -> key)
          : itoa((intptr_t) entry -> key);
  }
  switch (type) {
    case 'c':
      elem = c;
      break;
    case 's':
      elem =  str_wrap(c);
      f = (free_t) data_free;
      break;
    default:
      elem = entry -> key;
      break;
  }
  ctx -> data = ((reduce_t) ctx -> fnc)(elem, ctx -> data);
  if (f) {
    f(elem);
  }
  return ctx;
}

void * _set_reduce(set_t *set, reduce_t reducer, void *data, char *type) {
  reduce_ctx  ctx;
  void       *ret;

  reduce_ctx_initialize(&ctx, type, data, (void_t) reducer);
  ctx.obj = set;
  dict_reduce(set -> dict, (reduce_t) _set_reduce_reducer, &ctx);
  ret = ctx.data;
  return ret;
}

reduce_ctx * _set_visitor(entry_t *entry, reduce_ctx *ctx) {
  ((visit_t) ctx -> fnc)(entry -> key);
  return ctx;
}

set_t * _set_union_reducer(void *elem, set_t *set) {
  set_add(set, _set_copy(set, elem));
  return set;
}

reduce_ctx * _set_intersect_reducer(void *elem, reduce_ctx *ctx) {
  set_t *other;
  set_t *remove;

  other = (set_t *) ctx -> user;
  remove = (set_t *) ctx -> data;
  if (!set_has(other, elem)) {
    set_add(remove, _set_copy(remove, elem));
  }
  return ctx;
}

set_t * _set_remover(void *elem, set_t *set) {
  set_remove(set, elem);
  return set;
}

reduce_ctx * _set_minus_reducer(void *elem, reduce_ctx *ctx) {
  set_t *other;
  set_t *remove;

  other = (set_t *) ctx -> user;
  remove = (set_t *) ctx -> data;
  if (set_has(other, elem)) {
    set_add(remove, _set_copy(other, elem));
  }
  return ctx;
}

reduce_ctx * _set_disjoint_reducer(void *elem, reduce_ctx *ctx) {
  set_t *other;
  int   *disjoint;

  other = (set_t *) ctx -> user;
  disjoint = (int *) ctx -> data;
  *disjoint &= !set_has(other, elem);
  return ctx;
}

reduce_ctx * _set_subsetof_reducer(void *elem, reduce_ctx *ctx) {
  set_t *other;
  int   *hasall;

  other = (set_t *) ctx -> user;
  hasall = (int *) ctx -> data;
  *hasall &= set_has(other, elem);
  return ctx;
}

void ** _set_find_reducer(void *element, void **ctx) {
  cmp_t finder = (cmp_t) ctx[0];

  if (!finder(element, ctx[1])) {
    ctx[2] = element;
  }
  return ctx;
}

// -------------------
// set_t

set_t * set_create(cmp_t cmp) {
  set_t *ret;

  ret = NEW(set_t);
  ret -> dict = dict_create(cmp);
  ret -> str = NULL;
  return ret;
}

set_t * set_clone(set_t *src) {
  set_t      *ret = set_create(NULL);

  set_set_type(ret, &(ret -> dict -> key_type));
  return ret;
}

set_t * set_copy(set_t *src) {
  return set_union(set_clone(src), src);
}

set_t * set_set_type(set_t *set, type_t *type) {
  dict_set_key_type(set -> dict, type);
  return set;
}

set_t * set_set_free(set_t *set, free_t freefnc) {
  dict_set_free_key(set -> dict, freefnc);
  return set;
}

set_t * set_set_hash(set_t *set, hash_t hash) {
  dict_set_hash(set -> dict, hash);
  return set;
}

set_t * set_set_tostring(set_t *set, tostring_t tostring) {
  dict_set_tostring_key(set -> dict, tostring);
  return set;
}

set_t * set_set_copy(set_t *set, copy_t copyfnc) {
  dict_set_copy_key(set -> dict, copyfnc);
  return set;
}

void set_free(set_t *set) {
  if (set) {
    dict_free(set -> dict);
    free(set -> str);
    free(set);
  }
}

set_t * set_add(set_t *set, void *elem) {
  return dict_put(set -> dict, elem, NULL) ? set : NULL;
}

set_t * set_remove(set_t *set, void *elem) {
  return (set_has(set, elem) && dict_remove(set -> dict, elem)) ? set : NULL;
}

int set_has(set_t *set, void *elem) {
  return dict_has_key(set -> dict, elem);
}

int set_size(set_t *set) {
  return dict_size(set -> dict);
}

void * set_reduce(set_t *set, reduce_t reducer, void *data) {
  return _set_reduce(set, reducer, data, "o");
}

void * set_reduce_chars(set_t *set, reduce_t reducer, void *data) {
  return _set_reduce(set, reducer, data, "c");
}

void * set_reduce_str(set_t *set, reduce_t reducer, void *data) {
  return _set_reduce(set, reducer, data, "s");
}

set_t * set_visit(set_t *set, visit_t visitor) {
  reduce_ctx *ctx;

  ctx = reduce_ctx_create(NULL, NULL, (void_t) visitor);
  if (ctx) {
    dict_reduce(set -> dict, (reduce_t) _set_visitor, ctx);
    free(ctx);
    return set;
  }
  return NULL;
}

set_t * set_clear(set_t *set) {
  if (set) {
    dict_clear(set -> dict);
  }
  return set;
}

set_t * set_union(set_t *set, set_t *other) {
  return (set && other) ? set_reduce(other, (reduce_t) _set_union_reducer, set) : NULL;
}

set_t * set_intersect(set_t *set, set_t *other) {
  set_t      *remove;
  reduce_ctx *ctx;

  if (!set || !other) {
    return NULL;
  }
  remove = set_clone(set);
  if (remove) {
    ctx = reduce_ctx_create(other, remove, NULL);
    if (ctx) {
      set_reduce(set, (reduce_t) _set_intersect_reducer, ctx);
      free(ctx);
      set_reduce(remove, (reduce_t) _set_remover, set);
      set_free(remove);
      return set;
    }
  }
  return NULL;
}

set_t * set_minus(set_t *set, set_t *other) {
  set_t      *remove;
  reduce_ctx *ctx;

  if (!set || !other) {
    return NULL;
  }
  remove = set_clone(set);
  if (remove) {
    ctx = reduce_ctx_create(other, remove, NULL);
    if (ctx) {
      set_reduce(set, (reduce_t) _set_minus_reducer, ctx);
      free(ctx);
      set_reduce(remove, (reduce_t) _set_remover, set);
      set_free(remove);
      return set;
    }
  }
  return NULL;
}

int set_disjoint(set_t *s1, set_t *s2) {
  int         disjoint;
  reduce_ctx *ctx;

  if (!s1 || !s2) {
    return -1;
  }
  ctx = reduce_ctx_create(s2, &disjoint, NULL);
  disjoint = 1;
  if (ctx) {
    set_reduce(s1, (reduce_t) _set_disjoint_reducer, ctx);
    free(ctx);
    return disjoint;
  }
  return FALSE;
}

int set_subsetof(set_t *s1, set_t *s2) {
  int         hasall;
  reduce_ctx *ctx;

  if (!s1 || !s2) {
    return -1;
  }
  ctx = reduce_ctx_create(s2, &hasall, NULL);
  hasall = 1;
  if (ctx) {
    set_reduce(s1, (reduce_t) _set_subsetof_reducer, ctx);
    free(ctx);
    return hasall;
  }
  return FALSE;
}

int set_cmp(set_t *s1, set_t *s2) {
  int   ret;

  if (!s1 || !s2) {
    return -1;
  }
  ret = set_size(s1) - set_size(s2);
  if (!ret) {
    ret = !set_subsetof(s1, s2);
  }
  return ret;
}

void * set_find(set_t *haystack, cmp_t finder, void *needle) {
  void *ctx[3];

  if (!haystack) {
    return NULL;
  }
  ctx[0] = (void *) finder;
  ctx[1] = needle;
  ctx[2] = NULL;
  set_reduce(haystack, (reduce_t) _set_find_reducer, ctx);
  return ctx[2];
}

str_t * set_tostr(set_t *set) {
  str_t *ret;
  str_t *catted;

  if (!set) {
    return NULL;
  }
  ret = str_copy_chars("{");
  catted = str_join(", ", set, set_reduce_chars);
  if (ret && catted) {
    str_append(ret, catted);
    str_append_chars(ret, "}");
  }
  str_free(catted);
  return ret;
}

char * set_tostring(set_t *set) {
  str_t *s = set_tostr(set);

  if (!set) {
    return NULL;
  }
  set -> str = strdup(str_chars(s));
  str_free(s);
  return set -> str;
}
