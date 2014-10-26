/*
 * dict.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <errno.h>
#include <stdlib.h>

#include "core.h"
#include "dict.h"

typedef struct _dictentry {
  dict_t       *dict;
  void         *key;
  void         *value;
  unsigned int hash;
} dictentry_t;

static dictentry_t * _dictentry_create(dict_t *, void *, void  *);
static void          _dictentry_free(dictentry_t *);
static unsigned int  _dictentry_hash(dictentry_t *);
static int           _dictentry_cmp_key(dictentry_t *, void *);

static entry_t       _entry_from_dictentry(dictentry_t *);

static unsigned int  _dict_hash(dict_t *, void *);
static int           _dict_cmp_keys(dict_t *, void *, void *);
static void          _dict_clear_bucket(list_t *);
static void          _dict_free_bucket(list_t *);
static int           _dict_rehash(dict_t *);
static list_t *      _dict_get_bucket(dict_t *, void *);
static int           _dict_add_to_bucket(array_t *, dictentry_t *);
static dictentry_t * _dict_find_in_bucket(dict_t *, void *);

// ---------------------------
// dictentry_t static functions

dictentry_t * _dictentry_create(dict_t *dict, void *key, void *value) {
  dictentry_t *entry;

  entry = NEW(dictentry_t);
  if (entry) {
    entry -> dict = dict;
    entry -> key = key;
    entry -> value = value;
    entry -> hash = _entry_hash(entry);
  }
  return entry;
}

void _dictentry_free(dictentry_t *entry) {
  if (entry -> dict -> free_key) {
    entry -> dict -> free_key(entry -> key);
  }
  if (entry -> dict -> free_data) {
    entry -> dict -> free_data(entry -> value);
  }
  free(entry);
}

unsigned int _dictentry_hash(dictentry_t *entry) {
  return _dict_hash(entry -> dict, entry -> key);
}

int _dictentry_cmp_key(dictentry_t *entry, void *other) {
  return _dict_cmp_keys(entry -> dict, entry -> key, other);
}

// ---------------------------
// static entry_t functions

entry_t _entry_from_dictentry(dictentry_t *e) {
  entry_t *ret;
  
  ret = NEW(entry_t);
  if (ret) {
    ret -> key = e -> key;
    ret -> value = e -> value;
  }
  return ret;
}

// ---------------------------
// static dict_t functions

unsigned int _dict_hash(dict_t *dict, void *key) {
  return (dict -> hash)
    ? dict -> hash(key)
    : hash(&key, sizeof(void *));
}

int _dict_cmp_keys(dict_t *dict, void *key1, void *key2) {
  return (dict -> cmp)
    ? dict -> cmp(key1, key2)
    : (long) key1 - (long) key2;
}

void _dict_clear_bucket(list_t *bucket) {
  list_clear(bucket, (visit_t) _entry_free);
}

void _dict_free_bucket(list_t *bucket) {
  list_free(bucket, (visit_t) _entry_free);
}

int _dict_rehash(dict_t *dict) {
  array_t *new_buckets;
  int num, i, bucket, ok;
  list_t *l;
  listiterator_t *iter;

  num = dict -> num_buckets * 2;
  new_buckets = array_create(num);
  if (!new_buckets) {
    return FALSE;
  }
  ok = TRUE;
  num = array_capacity(new_buckets);
  for (i = 0; i < num; i++) {
    l = list_create();
    if (!l) {
      ok = FALSE;
      break;
    }
    array_set(new_buckets, i, l);
  }
  if (ok) {
    for (i = 0; i < dict -> num_buckets; i++) {
      for(iter = li_create((list_t *) array_get(dict -> buckets, i));
	  li_has_next(iter); ) {
	ok = _dict_add_to_bucket(new_buckets, (dictentry_t *) li_next(iter));
	if (!ok) break;
      }
      li_free(iter);
      if (!ok) break;
    }
  }
  if (ok) {
    dict -> buckets = new_buckets;
    dict -> num_buckets = num;
  } else {
    array_free(new_buckets, (visit_t) list_free);
    free(new_buckets);
  }
  return ok;
}

list_t * _dict_get_bucket(dict_t *dict, void *key) {
  int bucket_num;
  list_t *bucket;
  unsigned int hash;

  hash = _dict_hash(dict, key);
  bucket_num = hash % array_capacity(dict -> buckets);
  debug("hash: %x cap: %d bucket_num: %d", hash, array_capacity(dict -> buckets), bucket_num);
  return (list_t *) array_get(dict -> buckets, bucket_num);
}

int _dict_add_to_bucket(array_t *buckets, dictentry_t *entry) {
  int bucket_num;
  list_t *bucket;
  listiterator_t *iter;
  int ok;
  dictentry_t *e;

  bucket_num = (int) (entry -> hash % (unsigned int) array_capacity(buckets));
  bucket = (list_t *) array_get(buckets, bucket_num);
  if (!bucket) {
    return FALSE;
  }
  ok = FALSE;
  for(iter = li_create((list_t *) bucket); li_has_next(iter); ) {
    e = (dictentry_t *) li_next(iter);
    if (_entry_cmp_key(e, entry -> key) == 0) {
      li_replace(iter, entry);
      _entry_free(e);
      ok = 1;
      break;
    }
  }
  li_free(iter);
  if (!ok) {
    ok = list_append(bucket, entry);
    if (ok) ok = 2;
  }
  return ok;
}

dictentry_t * _dict_find_in_bucket(dict_t *dict, void *key) {
  list_t *bucket;
  listiterator_t *iter;
  dictentry_t *e, *ret;

  ret = NULL;
  bucket = _dict_get_bucket(dict, key);
  for(iter = li_create((list_t *) bucket); li_has_next(iter); ) {
    e = (dictentry_t *) li_next(iter);
    if (_dict_cmp_keys(dict, e -> key, key) == 0) {
      ret = e;
      break;
    }
  }
  li_free(iter);
  return ret;
}

// ---------------------------
// dict_t

dict_t * dict_create(cmp_t cmp) {
  dict_t *ret;
  dict_t *d;
  int i, ok;
  list_t *l;

  ret = NULL;
  d = NEW(dict_t);
  if (d) {
    ok = FALSE;
    d -> buckets = array_create(0);
    if (d -> buckets) {
      d -> cmp = cmp;
      d -> hash = NULL;
      d -> free_key = NULL;
      d -> free_data = NULL;
      d -> size = 0;
      d -> loadfactor = 0.75; /* TODO: Make configurable */
      d -> num_buckets = array_capacity(d -> buckets);
      for (i = 0; i < d -> num_buckets; i++) {
        l = list_create();
        if (!l) break;
        array_set(d -> buckets, i, l);
      }
      ok = (i == d -> num_buckets);
    }
    if (ok) {
      ret = d;
    } else {
      if (d -> buckets) {
        array_free(d -> buckets, (visit_t) list_free);
      }
      free(d);
    }
  }
  return ret;
}

void dict_set_hash(dict_t *dict, hash_t hash) {
  dict -> hash = hash;
}

void dict_set_free_key(dict_t *dict, visit_t free_key) {
  dict -> free_key = free_key;
}

void dict_set_free_data(dict_t *dict, visit_t free_data) {
  dict -> free_data = free_data;
}

int dict_size(dict_t *dict) {
  return dict -> size;
}

void dict_free(dict_t *dict) {
  array_free(dict -> buckets, (visit_t) _dict_free_bucket);
  free(dict);
}

void dict_clear(dict_t *dict) {
  array_clear(dict -> buckets, (visit_t) _dict_clear_bucket);
  dict -> size = 0;
}

int dict_put(dict_t *dict, void *key, void *data) {
  dictentry_t *entry;
  dictentry_t *e;
  int bucket;
  float lf;
  int ret;

  lf = dict -> size / dict -> num_buckets;
  if (lf > dict -> loadfactor) {
    if (!_dict_rehash(dict)) {
      return FALSE;
    }
  }

  entry = _entry_create(dict, key, data);
  if (!entry) {
    return FALSE;
  }

  ret = _dict_add_to_bucket(dict -> buckets, entry);
  if (ret) {
    dict -> size += ret - 1;
  }
  return TRUE;
}

void * dict_get(dict_t *dict, void *key) {
  dictentry_t *entry;
  void *ret;
  
  entry = _dict_find_in_bucket(dict, key);
  return (entry) ? entry -> value : NULL;
}

void list_reduce(dict_t *dict, reduce_t reducer, void *data) {
  list_t *bucket;
  int cap, len, i, j;
  listiterator_t *iter;
  dictentry_t *entry;
  
  ret = list_create();
  cap = array_capacity(dict -> buckets);
  for (i = 0; i < cap; i++) {
    bucket = (list_t *) array_get((array_t *) dict -> buckets, i);
    len = list_size(bucket);
    if (len > 0) {
      for (iter = li_create(bucket); li_has_next(iter); ) {
        entry = (dictentry_t *) li_next(iter);
        data = reducer(_entry_from_dictentry(entry), data);
      }
      li_free(iter);
    }
  }
  return data;
}

void * _dict_visitor(entry_t *e, visit_t visitor) {
  visitor(e);
  return visitor;
}
  
void dict_visit(dict_t *dict, visit_t visitor) {
  dict_reduce(dict, (reduce_t) _dict_visitor, visitor);
}

void * _dict_keys_reducer(entry_t *e, list_t *keys) {
  list_append(keys, e -> key);
  return keys;
}
  
list_t * dict_keys(dict_t *dict) {
  list_t *ret;
  
  ret = list_create();
  return dict_reduce(dict, (reduce_t) _dict_keys_reducer, ret);
}

void * _dict_values_reducer(entry_t *e, list_t *values) {
  list_append(values, e -> value);
  return keys;
}
  
list_t * dict_values(dict_t *dict) {
  list_t *ret;
  
  ret = list_create();
  return dict_reduce(dict, (reduce_t) _dict_values_reducer, ret);
}

void * _dict_items_reducer(entry_t *e, list_t *items) {
  list_append(values, e);
  return items;
}

list_t * dict_items(dict_t *dict) {
  list_t *ret;
  
  ret = list_create();
  return dict_reduce(dict, (reduce_t) _dict_items_reducer, ret);
}

void dict_dump(dict_t *dict) {
  list_t *bucket;
  int cap, len, i, j;
  listiterator_t *iter;
  dictentry_t *entry;
  
  debug("dict_dump");
  debug("==============================================");
  debug("Size: %d", dict_size(dict));
  cap = array_capacity(dict -> buckets);
  debug("Buckets: %d", cap);
  for (i = 0; i < cap; i++) {
    bucket = (list_t *) array_get((array_t *) dict -> buckets, i);
    len = list_size(bucket);
    debug("  Bucket: %d bucket size: %d", i, len);
    if (len > 0) {
      debug("  --------------------------------------------");
      for (iter = li_create(bucket); li_has_next(iter); ) {
        entry = (dictentry_t *) li_next(iter);
        debug("    %s (%u) --> %s", (char *) entry -> key, entry -> hash, (char *) entry -> value);
      }
      debug("");
      li_free(iter);
    }
  }
}