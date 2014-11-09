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

#include <stdlib.h>

#include <set.h>

static reduce_ctx * _set_reduce_reducer(entry_t *, reduce_ctx *);
static reduce_ctx * _set_visitor(entry_t *, reduce_ctx *);
static set_t *      _set_union_reducer(void *, set_t *);
static reduce_ctx * _set_intersect_reducer(void *, reduce_ctx *);
static set_t *      _set_remover(void *, set_t *);
static reduce_ctx * _set_minus_reducer(void *, reduce_ctx *);

// --------------------
// set_t static methods

reduce_ctx * _set_reduce_reducer(entry_t *entry, reduce_ctx *ctx) {
  ctx -> data = ctx -> fnc.reducer(entry -> key, ctx -> data);
  return ctx;
}

reduce_ctx * _set_visitor(entry_t *entry, reduce_ctx *ctx) {
  ctx -> fnc.visitor(entry -> key);
  return ctx;
}

set_t * _set_union_reducer(void *elem, set_t *set) {
  set_add(set, elem);
  return set;
}

reduce_ctx * _set_intersect_reducer(void *elem, reduce_ctx *ctx) {
  set_t *set;
  set_t *other;
  set_t *remove;

  set = (set_t *) ctx -> obj;
  other = (set_t *) ctx -> user;
  remove = (set_t *) ctx -> data;
  if (!set_has(other, elem)) {
    set_add(remove, elem);
  }
  return ctx;
}

set_t * _set_remover(void *elem, set_t *set) {
  set_remove(set, elem);
}

reduce_ctx * _set_minus_reducer(void *elem, reduce_ctx *ctx) {
  set_t *set;
  set_t *other;
  set_t *remove;

  set = (set_t *) ctx -> obj;
  other = (set_t *) ctx -> user;
  remove = (set_t *) ctx -> data;
  if (set_has(other, elem)) {
    set_add(remove, elem);
  }
  return ctx;
}

// -------------------
// set_t

set_t * set_create(cmp_t cmp) {
  set_t *ret;

  ret = NEW(set_t);
  if (ret) {
    ret -> dict = dict_create(cmp);
    if (!ret -> dict) {
      free(ret);
      ret = NULL;
    }
  } 
  return ret;
}

set_t * set_set_free(set_t *set, free_t freefnc) {
  dict_set_free_key(set -> dict, freefnc);
  return set;
}

set_t * set_set_hash(set_t *set, hash_t hash) {
  dict_set_hash(set -> dict, hash);
  return set;
}

void set_free(set_t *set) {
  dict_free(set -> dict);
  free(set);
}

int set_add(set_t *set, void *elem) {
  if (!set_has(set, elem)) {
    return dict_put(set -> dict, elem, NULL);
  } else {
    return TRUE;
  }
}

void set_remove(set_t *set, void *elem) {
  if (set_has(set, elem)) {
    dict_remove(set -> dict, elem);
  }
}

int set_has(set_t *set, void *elem) {
  return dict_has_key(set -> dict, elem);
}

int set_size(set_t *set) {
  return dict_size(set -> dict);
}

void * set_reduce(set_t *set, reduce_t reducer, void *data) {
  reduce_ctx *ctx;
  void *ret;
  function_ptr_t fnc;

  fnc.reducer = reducer;
  ctx = reduce_ctx_create(NULL, data, fnc);
  if (ctx) {
    dict_reduce(set -> dict, (reduce_t) _set_reduce_reducer, ctx);
    ret = ctx -> data;
    free(ctx);
    return ret;
  }
  return NULL;
}

set_t * set_visit(set_t *set, visit_t visitor) {
  reduce_ctx *ctx;
  function_ptr_t fnc;

  fnc.visitor = visitor;
  ctx = reduce_ctx_create(NULL, NULL, fnc );
  if (ctx) {
    dict_reduce(set -> dict, (reduce_t) _set_visitor, ctx);
    free(ctx);
    return set;
  }
  return NULL;
}

set_t * set_clear(set_t *set) {
  dict_clear(set -> dict);
  return set;
}

set_t * set_union(set_t *set, set_t *other) {
  return set_reduce(other, (reduce_t) _set_union_reducer, set);
}

set_t * set_intersect(set_t *set, set_t *other) {
  set_t *remove;
  reduce_ctx *ctx;

  remove = set_create(set -> dict -> cmp);
  if (remove) {
    ctx = reduce_ctx_create(other, remove, NOFUNCPTR);
    if (ctx) {
      remove = set_reduce(set, (reduce_t) _set_intersect_reducer, ctx);
      free(ctx);
      set_reduce(remove, (reduce_t) _set_remover, set);
      set_free(remove);
      return set;
    }
  }
  return NULL;
}

set_t * set_minus(set_t *set, set_t *other) {
  set_t *remove;
  reduce_ctx *ctx;

  remove = set_create(set -> dict -> cmp);
  if (remove) {
    ctx = reduce_ctx_create(other, remove, NOFUNCPTR);
    if (ctx) {
      remove = set_reduce(set, (reduce_t) _set_minus_reducer, ctx);
      free(ctx);
      set_reduce(remove, (reduce_t) _set_remover, set);
      set_free(remove);
      return set;
    }
  }
  return NULL;
}

