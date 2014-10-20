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

static entry_t * _entry_create(dict_t *, void *, void  *);
static void      _entry_free(entry_t *);
static int       _entry_hash(entry_t *);

static void      _dict_clear_bucket(list_t *);
static void      _dict_free_bucket(list_t *);
static int       _dict_rehash(dict_t *);
static int       _dict_add_to_bucket(array_t *, entry_t *);
static void      _dict_find_in_bucket(array_t *, void *);

// ---------------------------
// entry_t static functions

entry_t * _entry_create(dict_t *dict, void *key, void *value) {
  entry_t *entry;

  entry = NEW(entry_t);
  if (entry) {
    entry -> dict = dict;
    entry -> key = key;
    entry -> value = value;
    entry -> hash = _entry_hash(entry);
  }
  return entry;
}

void _entry_free(entry_t *entry) {
  if (entry -> dict -> free_key) {
    entry -> dict -> free_key(entry -> key);
  }
  if (entry -> dict -> free_data) {
    entry -> dict -> free_data(entry -> value);
  }
  free(entry);
}

int _entry_hash(entry_t *entry) {
  if (entry -> dict -> hash) {
    return entry -> dict -> hash(entry -> key);
  } else {
    return hash(&(entry -> key), sizeof(void *));
  }
}

int _entry_cmp_key(entry_t *entry, void *other) {
  if (entry -> dict -> cmp) {
    return entry -> dict -> cmp(entry -> key, other);
  } else {
    return (long) entry -> key - (long) other;
  }
}

// ---------------------------
// static dict_t functions

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
	ok = _dict_add_to_bucket(new_buckets, (entry_t *) li_next(iter));
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

int _dict_add_to_bucket(array_t *buckets, entry_t *entry) {
  int bucket_num;
  list_t *bucket;
  listiterator_t *iter;
  int ok;
  entry_t *e;

  bucket_num = entry -> hash % array_capacity(buckets);
  bucket = (list_t *) array_get(buckets, bucket_num);
  ok = FALSE;
  for(iter = li_create((list_t *) bucket); li_has_next(iter); ) {
    e = (entry_t *) li_next(iter);
    if (_entry_cmp_key(e, entry -> key) == 0) {
      li_replace(iter, entry);
      _entry_free(e);
      ok = TRUE;
      break;
    }
  }
  li_free(iter);
  if (!ok) {
    ok = list_append(bucket, entry);
  }
  return ok;
}

void _dict_find_in_bucket(array_t *buckets, void *key) {
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
  entry_t *entry;
  entry_t *e;
  int bucket;
  float lf;

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

  _dict_add_to_bucket(dict -> buckets, entry);
  dict -> size++;
  return TRUE;
}

void dict_get(dict_t *dict, void *key) {
  void *ret;
  
  ret = _dict_find_in_bucket(dict -> buckets, key);
  return ret;
}
