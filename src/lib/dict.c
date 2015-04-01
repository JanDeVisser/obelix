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

//#define NDEBUG
#include <errno.h>
#include <stdlib.h>

#include <dict.h>
#include <str.h>

typedef struct _dictentry {
  dict_t       *dict;
  void         *key;
  void         *value;
  unsigned int  hash;
} dictentry_t;

typedef enum _dict_reduce_type {
  DRTEntries,
  DRTEntriesNoFree,
  DRTDictEntries,
  DRTKeys,
  DRTValues,
  DRTStringString
} dict_reduce_type_t;

static dictentry_t *    _dictentry_create(dict_t *, void *, void  *);
static void             _dictentry_free(dictentry_t *);
static unsigned int     _dictentry_hash(dictentry_t *);
static int              _dictentry_cmp_key(dictentry_t *, void *);

static entry_t *        _entry_from_dictentry(dictentry_t *);
static entry_t *        _entry_from_entry(entry_t *);

static unsigned int     _dict_hash(dict_t *, void *);
static int              _dict_cmp_keys(dict_t *, void *, void *);
static dict_t *         _dict_rehash(dict_t *);
static list_t *         _dict_get_bucket(dict_t *, void *);
static dict_t *         _dict_add_to_bucket(array_t *, dictentry_t *);
static list_t *         _dict_position_in_bucket(dict_t *, void *);
static dictentry_t *    _dict_find_in_bucket(dict_t *, void *);
static dict_t *         _dict_remove_from_bucket(dict_t *, void *);
static list_t *         _dict_append_reducer(void *, list_t *);
static void *           _dict_visitor(void *, reduce_ctx *);
static dict_t *         _dict_visit(dict_t *, visit_t, dict_reduce_type_t);
static void *           _dict_entry_reducer(dictentry_t *, reduce_ctx *);
static void *           _dict_reduce(dict_t *, reduce_t, void *, dict_reduce_type_t);
static void *           _dict_bucket_reducer(list_t *, reduce_ctx *);
static dict_t *         _dict_put_all_reducer(entry_t *, dict_t *);
static void **          _dict_entry_formatter(entry_t *, void **);

// ---------------------------
// dictentry_t static functions

dictentry_t * _dictentry_create(dict_t *dict, void *key, void *value) {
  dictentry_t *entry;

  entry = NEW(dictentry_t);
  if (entry) {
    entry -> dict = dict;
    entry -> key = key;
    entry -> value = value;
    entry -> hash = _dictentry_hash(entry);
  }
  return entry;
}

void _dictentry_free(dictentry_t *entry) {
  if (entry) {
    if (entry -> dict -> free_key && entry -> key) {
      entry -> dict -> free_key(entry -> key);
    }
    if (entry -> dict -> free_data && entry -> value) {
      entry -> dict -> free_data(entry -> value);
    }
    free(entry);
  }
}

unsigned int _dictentry_hash(dictentry_t *entry) {
  return _dict_hash(entry -> dict, entry -> key);
}

int _dictentry_cmp_key(dictentry_t *entry, void *other) {
  return _dict_cmp_keys(entry -> dict, entry -> key, other);
}

// ---------------------------
// static entry_t functions

entry_t * _entry_from_dictentry(dictentry_t *e) {
  entry_t *ret;
  
  ret = NEW(entry_t);
  if (ret) {
    ret -> key = e -> key;
    ret -> value = e -> value;
  }
  return ret;
}

entry_t * _entry_from_entry(entry_t *e) {
  entry_t *ret;
  
  ret = NEW(entry_t);
  ret -> key = e -> key;
  ret -> value = e -> value;
  return ret;
}

entry_t * _entry_from_strings(dictentry_t *e) {
  entry_t *ret;
  
  ret = NEW(entry_t);
  assert(e -> dict -> tostring_key);
  assert(e -> dict -> tostring_data);
  ret -> key = strdup(e -> dict -> tostring_key(e -> key));
  ret -> value = strdup(e -> dict -> tostring_data(e -> value));
  return ret;
}

void entry_free(entry_t *e) {
  if (e) {
    free(e);
  }
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

dict_t * _dict_rehash(dict_t *dict) {
  array_t     *new_buckets;
  dict_t      *ret;
  int          num;
  int          i;
  int          bucket;
  list_t      *l;
  dictentry_t *entry;

  num = dict -> num_buckets * 2;
  new_buckets = array_create(num);
  array_set_free(new_buckets, (visit_t) list_free);

  ret = dict;
  num = array_capacity(new_buckets);
  for (i = 0; i < num; i++) {
    l = list_create();
    list_set_free(l, (visit_t) _dictentry_free);
    array_set(new_buckets, i, l);
  }

  for (i = 0; i < dict -> num_buckets; i++) {
    l = (list_t *) array_get(dict -> buckets, i);
    for (entry = (dictentry_t *) list_pop(l); entry; entry = (dictentry_t *) list_pop(l)) {
      ret = _dict_add_to_bucket(new_buckets, entry);
      if (!ret) break;
    }
    if (!ret) break;
  }

  if (ret) {
    array_free(dict -> buckets);
    dict -> buckets = new_buckets;
    dict -> num_buckets = num;
  } else {
    array_free(new_buckets);
  }
  return ret;
}

list_t * _dict_get_bucket(dict_t *dict, void *key) {
  int bucket_num;
  list_t *bucket;
  unsigned int hash;

  hash = _dict_hash(dict, key);
  bucket_num = hash % array_capacity(dict -> buckets);
  return (list_t *) array_get(dict -> buckets, bucket_num);
}

dict_t * _dict_add_to_bucket(array_t *buckets, dictentry_t *entry) {
  int             bucket_num;
  list_t         *bucket;
  listiterator_t *iter;
  dict_t         *ret;
  dictentry_t    *e;

  bucket_num = (int) (entry -> hash % (unsigned int) array_capacity(buckets));
  bucket = (list_t *) array_get(buckets, bucket_num);
  assert(bucket);

  ret = NULL;
  for (iter = list_start(bucket); li_has_next(iter); ) {
    e = (dictentry_t *) li_next(iter);
    if (_dictentry_cmp_key(e, entry -> key) == 0) {
      li_replace(iter, entry);
      /* _dictentry_free(e); */
      ret = entry -> dict;
      break;
    }
  }
  if (!ret) {
    ret = (list_append(bucket, entry) != NULL) ? entry -> dict : NULL;
  }
  return ret;
}

list_t * _dict_position_in_bucket(dict_t *dict, void *key) {
  list_t      *bucket;
  dictentry_t *e;
  dictentry_t *ret;

  ret = NULL;
  bucket = _dict_get_bucket(dict, key);
  for (list_start(bucket); list_has_next(bucket); ) {
    e = (dictentry_t *) list_next(bucket);
    if (_dict_cmp_keys(dict, e -> key, key) == 0) {
      return bucket;
    }
  }
  return NULL;
}

dictentry_t * _dict_find_in_bucket(dict_t *dict, void *key) {
  list_t      *bucket;
  dictentry_t *ret;

  bucket = _dict_position_in_bucket(dict, key);
  ret = (bucket) ? list_current(bucket) : NULL;
  return ret;
}

dict_t * _dict_remove_from_bucket(dict_t *dict, void *key) {
  list_t *bucket;
  dict_t *ret;

  bucket = _dict_position_in_bucket(dict, key);
  ret = NULL;
  if (bucket) {
    list_remove(bucket);
    ret = dict;
  }
  return ret;
}

void * _dict_reduce_param(dictentry_t *e, dict_reduce_type_t reducetype) {
  void *elem;

  elem = NULL;
  switch (reducetype) {
    case DRTEntries:
    case DRTEntriesNoFree:
      elem = _entry_from_dictentry(e);
      break;
    case DRTDictEntries:
      elem = e;
      break;
    case DRTKeys:
      elem = e -> key;
      break;
    case DRTValues:
      elem = e -> value;
      break;
    case DRTStringString:
      elem = _entry_from_strings(e);
      break;
  }
  return elem;
}

void * _dict_entry_reducer(dictentry_t *e, reduce_ctx *ctx) {
  void               *elem;
  entry_t            *entry;
  dict_reduce_type_t  type;

  type = (dict_reduce_type_t) ((int) ((long) ctx -> user));
  elem = _dict_reduce_param(e, type);
  ctx -> data = ((reduce_t) ctx -> fnc)(elem, ctx -> data);
  switch (type) {
    case DRTStringString:
      entry = (entry_t *) elem;
      free(entry -> key);
      free(entry -> value);
      /* no break */
    case DRTEntries:
      free(elem);
      break;
  }
  return ctx;
}

void * _dict_bucket_reducer(list_t *bucket, reduce_ctx *ctx) {
  return list_reduce(bucket, (reduce_t) _dict_entry_reducer, ctx);
}

void * _dict_reduce(dict_t *dict, reduce_t reducer, void *data, dict_reduce_type_t reducetype) {
  reduce_ctx     *ctx;
  void           *ret;

  ctx = reduce_ctx_create((void *) ((long) reducetype), data, (void_t) reducer);
  if (ctx) {
    ctx = array_reduce(dict -> buckets, (reduce_t) _dict_bucket_reducer, ctx);
    ret = ctx -> data;
    free(ctx);
    return ret;
  }
  return NULL;
}

void * _dict_visitor(void *e, reduce_ctx *ctx) {
  ((visit_t) ctx -> fnc)(e);
  return ctx;
}
  
dict_t * _dict_visit(dict_t *dict, visit_t visitor, dict_reduce_type_t visittype) {
  reduce_ctx *ctx;

  ctx = reduce_ctx_create(NULL, NULL, (void_t) visitor);
  if (dict) {
    _dict_reduce(dict, (reduce_t) _dict_visitor, ctx, visittype);
    free(ctx);
    return dict;
  } else {
    return NULL;
  }
}

list_t * _dict_append_reducer(void *elem, list_t *list) {
  list_append(list, elem);
  return list;
}

dict_t * _dict_put_all_reducer(entry_t *e, dict_t *dict) {
  dict_put(dict, e -> key, e -> value);
  return dict;
}

void ** _dict_entry_formatter(entry_t *e, void **ctx) {
  dict_t *dict = (dict_t *) ctx[0];
  str_t  *entries = (str_t *) ctx[1];
  char   *sep = (char *) ctx[2];
  char   *fmt = (char *) ctx[3];
  
  if (str_len(entries)) {
    str_append_chars(entries, sep);
  }
  str_append_chars(entries, fmt, e -> key, e -> value);
  return ctx;
}

// ---------------------------
// dict_t

dict_t * dict_create(cmp_t cmp) {
  dict_t *ret;
  dict_t *d;
  int     i, ok;
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
      d -> str = NULL;
      array_set_free(d -> buckets, (visit_t) list_free);

      for (i = 0; i < d -> num_buckets; i++) {
        l = list_create();
	list_set_free(l, (visit_t) _dictentry_free);
        if (!l) break;
        array_set(d -> buckets, i, l);
      }
      ok = (i == d -> num_buckets);
    }
    if (ok) {
      ret = d;
    } else {
      if (d -> buckets) {
        array_free(d -> buckets);
      }
      free(d);
    }
  }
  return ret;
}

dict_t * dict_set_hash(dict_t *dict, hash_t hash) {
  dict -> hash = hash;
  return dict;
}

dict_t * dict_set_free_key(dict_t *dict, free_t free_key) {
  dict -> free_key = free_key;
  return dict;
}

dict_t * dict_set_free_data(dict_t *dict, free_t free_data) {
  dict -> free_data = free_data;
  return dict;
}

dict_t * dict_set_tostring_key(dict_t *dict, tostring_t tostring_key) {
  dict -> tostring_key = tostring_key;
  return dict;
}

dict_t * dict_set_tostring_data(dict_t *dict, tostring_t tostring_data) {
  dict -> tostring_data = tostring_data;
  return dict;
}

int dict_size(dict_t *dict) {
  return dict -> size;
}

void dict_free(dict_t *dict) {
  if (dict) {
    array_free(dict -> buckets);
    free(dict -> str);
    free(dict);
  }
}

dict_t * dict_clear(dict_t *dict) {
  if (dict_notempty(dict)) {
    array_visit(dict -> buckets, (visit_t) list_clear);
    dict -> size = 0;
  }
  return dict;
}

dict_t * dict_put(dict_t *dict, void *key, void *data) {
  dictentry_t *entry;
  dictentry_t *e;
  int          bucket;
  float        lf;
  dict_t      *ret;
  int          had_key;

  lf = dict -> size / dict -> num_buckets;
  if (lf > dict -> loadfactor) {
    if (!_dict_rehash(dict)) {
      return NULL;
    }
  }

  entry = _dictentry_create(dict, key, data);
  if (!entry) {
    return NULL;
  }

  had_key = dict_has_key(dict, key);
  ret = _dict_add_to_bucket(dict -> buckets, entry);
  if (ret && !had_key) {
    dict -> size++;
  }
  return ret;
}

/**
 * Removes the entry associated with the given key from the dict. The free
 * functions, if set, for both the key and the value are called.
 *
 * @param dict Dictionary to remove the entry from.
 * @param key Key to remove from the dictionary.
 *
 * @return The dictionary.
 */
dict_t * dict_remove(dict_t *dict, void *key) {
  dict_t *ret;

  ret = _dict_remove_from_bucket(dict, key);
  if (ret) {
    dict -> size--;
  }
  return ret;
}

void * dict_get(dict_t *dict, void *key) {
  dictentry_t *entry;
  void        *ret;
  
  entry = _dict_find_in_bucket(dict, key);
  return (entry) ? entry -> value : NULL;
}

/**
 * Removes the entry for the given key from the dict, and returns the value
 * associated with that key. The free function, if set, for the key us called,
 * but not the free function for the value.
 *
 * @param dict Dictionary to remove the entry from.
 * @param key Key to remove from the dictionary.
 *
 * @return The value associated with the key prior to removal, or NULL if the
 * key was not present in the dictionary.
 */
void * dict_pop(dict_t *dict, void *key) {
  dictentry_t *entry;
  void        *ret;

  entry = _dict_find_in_bucket(dict, key);
  ret = (entry) ? entry -> value : NULL;
  if (entry) {
    entry -> value = NULL;
    dict_remove(dict, key);
  }
  return ret;
}

int dict_has_key(dict_t *dict, void *key) {
  return _dict_find_in_bucket(dict, key) != NULL;
}

void * dict_reduce(dict_t *dict, reduce_t reducer, void *data) {
  return _dict_reduce(dict, reducer, data, DRTEntries);
}

void * dict_reduce_keys(dict_t *dict, reduce_t reducer, void *data) {
  return _dict_reduce(dict, reducer, data, DRTKeys);
}

void * dict_reduce_values(dict_t *dict, reduce_t reducer, void *data) {
  return _dict_reduce(dict, reducer, data, DRTValues);
}

void * dict_reduce_chars(dict_t *dict, reduce_t reducer, void *data) {
  return _dict_reduce(dict, reducer, data, DRTStringString);
}

void * _dict_reduce_dictentries(dict_t *dict, reduce_t reducer, void *data) {
  return _dict_reduce(dict, reducer, data, DRTDictEntries);
}

dict_t * dict_visit(dict_t *dict, visit_t visitor) {
  return _dict_visit(dict, visitor, DRTEntries);
}

dict_t * dict_visit_keys(dict_t *dict, visit_t visitor) {
  return _dict_visit(dict, visitor, DRTKeys);
}

dict_t * dict_visit_values(dict_t *dict, visit_t visitor) {
  return _dict_visit(dict, visitor, DRTValues);
}

dict_t * _dict_visit_dictentries(dict_t *dict, visit_t visitor) {
  return _dict_visit(dict, visitor, DRTDictEntries);
}

list_t * dict_keys(dict_t *dict) {
  list_t *ret;
  
  ret = list_create();
  if (ret) {
    ret = (list_t *) dict_reduce_keys(dict, (reduce_t) _dict_append_reducer, ret);
  }
  return ret;
}

list_t * dict_values(dict_t *dict) {
  list_t *ret;
  
  ret = list_create();
  if (ret) {
    ret = (list_t *) dict_reduce_values(dict, (reduce_t) _dict_append_reducer, ret);
  }
  return ret;
}

list_t * dict_items(dict_t *dict) {
  list_t *ret;
  
  ret = list_create();
  list_set_free(ret, (visit_t) entry_free);
  _dict_reduce(dict, (reduce_t) _dict_append_reducer, ret, DRTEntriesNoFree);
  return ret;
}

dict_t * dict_put_all(dict_t *dict, dict_t *other) {
  return dict_reduce(other, (reduce_t) _dict_put_all_reducer, dict);
}

str_t * dict_tostr_custom(dict_t *dict, char *open, char *fmt, char *sep, char *close) {
  str_t      *ret;
  str_t      *entries;
  void       *ctx[4];

  assert(dict -> tostring_key);
  assert(dict -> tostring_data);
  
  ret = str_copy_chars(open);
  entries = str_create(0);
  ctx[0] = dict;
  ctx[1] = entries;
  ctx[2] = sep;
  ctx[3] = fmt;
  dict_reduce_chars(dict, (reduce_t) _dict_entry_formatter, ctx);
  str_append(ret, entries);
  str_append_chars(ret, close);
  str_free(entries);
  return ret;
}

str_t * dict_tostr(dict_t *dict) {
  return dict_tostr_custom(dict, "{ ", "\"%s\": %s", ", ", " }");
}

char * dict_tostring_custom(dict_t *dict, char *open, char *fmt, char *sep, char *close) {
  str_t *s;
  
  free(dict -> str);
  s = dict_tostr_custom(dict, open, fmt, sep, close);
  dict -> str = str_reassign(s);
  return dict -> str;
}

char * dict_tostring(dict_t *dict) {
  return dict_tostring_custom(dict, "{ ", "\"%s\": %s", ", ", " }");
}

str_t * dict_dump(dict_t *dict, char *title) {
  list_t         *bucket;
  int             cap, len, i, j;
  dictentry_t    *entry;
  str_t          *ret;
  
  ret = str_copy_chars("dict_dump -- %s\n", title);
  str_append_chars(ret, "==============================================\n");
  str_append_chars(ret, "Size: %d\n", dict_size(dict));
  cap = array_capacity(dict -> buckets);
  str_append_chars(ret, "Buckets: %d\n", cap);
  for (i = 0; i < cap; i++) {
    bucket = (list_t *) array_get((array_t *) dict -> buckets, i);
    len = list_size(bucket);
    str_append_chars(ret, "  Bucket: %d bucket size: %d\n", i, len);
    if (len > 0) {
      debug("  --------------------------------------------\n");
      for (list_start(bucket); list_has_next(bucket); ) {
        entry = (dictentry_t *) list_next(bucket);
        str_append_chars(ret, "    %s (%u) --> %s\n", 
                         (char *) entry -> key, entry -> hash, 
                         (char *) entry -> value);
      }
      str_append_chars(ret, "\n");
    }
  }
  return ret;
}
