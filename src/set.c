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

#include "core.h"
#include "set.h"

static void *       _set_reduce_reducer(entry_t *, reduce_ctx *);
static visit_t      _set_visitor(void *data, visit_t visitor);
static set_t *      _set_union_reducer(void *, set_t *);
static reduce_ctx * _set_intersect_reducer(void *, reduce_ctx *);
static set_t *      _set_remover(void *, set_t *);
static reduce_ctx * _set_minus_reducer(void *, reduce_ctx *);

// --------------------
// set_t static methods

void * _set_reduce_reducer(entry_t *entry, reduce_ctx *ctx) {
  ctx -> data = ctx -> reducer(entry -> key, ctx -> data);
  return ctx;
}

visit_t _set_visitor(void *data, visit_t visitor) {
  visitor(data);
  return visitor;
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
  return _reduce(set -> dict, (obj_reduce_t) dict_reduce, 
		 (reduce_t) _set_reduce_reducer, 
		 NULL, data);
}

set_t * set_visit(set_t *set, visit_t visitor) {
  set_reduce(set, (reduce_t) _set_visitor, visitor);
  return set;
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

  remove = set_create(set -> dict -> cmp);
  if (remove) {
    remove = _reduce(set, (obj_reduce_t) set_reduce, 
		     (reduce_t) _set_intersect_reducer, other, remove);
    set_reduce(remove, (reduce_t) _set_remover, set);
    set_free(remove);
    return set;
  } else {
    return NULL;
  }
}

set_t * set_minus(set_t *set, set_t *other) {
  set_t *remove;

  remove = set_create(set -> dict -> cmp);
  if (remove) {
    remove = _reduce(set, (obj_reduce_t) set_reduce, 
		     (reduce_t) _set_minus_reducer, other, remove);
    set_reduce(remove, (reduce_t) _set_remover, set);
    set_free(remove);
    return set;
  } else {
    return NULL;
  }
}

