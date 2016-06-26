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

#define INIT_BUCKETS       4
#define INIT_BUCKET_SIZE   4


typedef enum _dict_reduce_type {
  DRTEntries,
  DRTEntriesNoFree,
  DRTDictEntries,
  DRTKeys,
  DRTValues,
  DRTStringString
} dict_reduce_type_t;

static void             _dictentry_free(dictentry_t *);
static unsigned int     _dictentry_hash(dictentry_t *);
static int              _dictentry_cmp_key(dictentry_t *, void *);

static entry_t *        _entry_from_dictentry(dictentry_t *);
static entry_t *        _entry_from_entry(entry_t *);

static bucket_t *       _bucket_create(dict_t *, int);
static void             _bucket_free(bucket_t *);
static void             _bucket_clear(bucket_t *);
static bucket_t *       _bucket_expand(bucket_t *);
static bucket_t *       _bucket_copy_entry(bucket_t *, dictentry_t *);
static bucket_t *       _bucket_init_entry(bucket_t *, void *, void *);
static void *           _bucket_reduce(bucket_t *, reduce_t, void *);
static void             _bucket_remove(bucket_t *, int);
static dictentry_t *    _bucket_find_entry(bucket_t *, void *);
static int              _bucket_position_of(bucket_t *, void *);
static bucket_t *       _bucket_put(bucket_t *, void *, void *);

static unsigned int     _dict_hash(dict_t *, void *);
static int              _dict_cmp_keys(dict_t *, void *, void *);
static dict_t *         _dict_rehash(dict_t *);
static bucket_t *       _dict_get_bucket(dict_t *, void *);
static dict_t *         _dict_add_to_bucket(dict_t *, void *, void *);
static dictentry_t *    _dict_find_in_bucket(dict_t *, void *);
static dict_t *         _dict_remove_from_bucket(dict_t *, void *);
static list_t *         _dict_append_reducer(void *, list_t *);
static void *           _dict_visitor(void *, reduce_ctx *);
static dict_t *         _dict_visit(dict_t *, visit_t, dict_reduce_type_t);
static void *           _dict_entry_reducer(dictentry_t *, reduce_ctx *);
static void *           _dict_reduce(dict_t *, reduce_t, void *, dict_reduce_type_t);
static dict_t *         _dict_put_all_reducer(entry_t *, dict_t *);
static void **          _dict_entry_formatter(entry_t *, void **);
static void             _dict_visit_buckets(dict_t *, visit_t);

// -- D I C T E N T R Y  S T A T I C  F U N C T I O N S ------------------- */

void _dictentry_free(dictentry_t *entry) {
  if (entry) {
    if (entry -> dict -> key_type.free && entry -> key) {
      entry -> dict -> key_type.free(entry -> key);
    }
    if (entry -> dict -> data_type.free && entry -> value) {
      entry -> dict -> data_type.free(entry -> value);
    }
  }
}

unsigned int _dictentry_hash(dictentry_t *entry) {
  return _dict_hash(entry -> dict, entry -> key);
}

int _dictentry_cmp_key(dictentry_t *entry, void *other) {
  return _dict_cmp_keys(entry -> dict, entry -> key, other);
}

// -- E N T R Y  S T A T I C  F U N C T I O N S --------------------------- */

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
  assert(e -> dict -> key_type.tostring);
  assert(e -> dict -> data_type.tostring);
  ret -> key = strdup(e -> dict -> key_type.tostring(e -> key));
  ret -> value = strdup(e -> dict -> data_type.tostring(e -> value));
  return ret;
}

void entry_free(entry_t *e) {
  if (e) {
    free(e);
  }
}

/* -- B U C K E T  M A N A G E M E N T ------------------------------------ */


static bucket_t * _bucket_create(dict_t *dict, int ix) {
  bucket_t *ret = (bucket_t *) new(sizeof(bucket_t) + INIT_BUCKET_SIZE * sizeof(dictentry_t));
  
  ret -> dict = dict;
  ret -> ix = ix;
  ret -> capacity = INIT_BUCKET_SIZE;
  ret -> size = 0;
  dict -> buckets[ix] = ret;
  return ret;
}

void _bucket_free(bucket_t *bucket) {
  _bucket_clear(bucket);
  free(bucket);
}

void _bucket_clear(bucket_t *bucket) {
  int ix;
  
  for (ix = 0; ix < bucket -> size; ix++) {
    _dictentry_free(&(bucket -> entries[ix]));
  }
  bucket -> size = 0;
}

bucket_t * _bucket_expand(bucket_t *bucket) {
  bucket = resize_block(bucket,
                        sizeof(bucket_t) + (2 * bucket -> capacity) * sizeof(dictentry_t),
                        sizeof(bucket_t) + bucket -> capacity * sizeof(dictentry_t));
  bucket -> capacity *= 2;
  bucket -> dict -> buckets[bucket -> ix] = bucket;
  return bucket;
}

bucket_t * _bucket_copy_entry(bucket_t *bucket, dictentry_t *entry) {
  if (bucket -> size >= bucket -> capacity) {
    bucket = _bucket_expand(bucket);
  }
  memcpy(&bucket -> entries[bucket -> size], entry, sizeof(dictentry_t));
  bucket -> entries[bucket -> size].ix = bucket -> size;
  bucket -> size++;
  return bucket;
}

bucket_t * _bucket_init_entry(bucket_t *bucket, void *key, void *value) {
  dictentry_t *entry;
  
  if (bucket -> size >= bucket -> capacity) {
    bucket = _bucket_expand(bucket);
  }
  entry = &(bucket -> entries[bucket -> size]);
  entry -> dict = bucket -> dict;
  entry -> key = key;
  entry -> value = value;
  entry -> hash = _dict_hash(bucket -> dict, key);
  entry -> ix = bucket -> size;
  bucket -> size++;
  return bucket;
}

void * _bucket_reduce(bucket_t *bucket, reduce_t reducer, void *data) {
  int ix;
  
  for (ix = 0; ix < bucket -> size; ix++) {
    data = reducer(&(bucket -> entries[ix]), data);
  }
  return data;
}

void _bucket_remove(bucket_t *bucket, int ix) {
  int i;
  
  assert(bucket -> size > ix);
  _dictentry_free(&(bucket -> entries[ix]));
  bucket -> size--;
  if (ix < bucket -> size) {
    memmove(&(bucket -> entries[ix]), &(bucket -> entries[ix+1]), 
            (bucket -> size - ix) * sizeof(dictentry_t));
    for (i = ix; i < bucket -> size; i++) {
      bucket -> entries[i].ix = i;
    }
  }
}

int _bucket_position_of(bucket_t *bucket, void *key) {
  dictentry_t *e = _bucket_find_entry(bucket, key);
  
  return (e) ? e -> ix : -1;
}

dictentry_t * _bucket_find_entry(bucket_t *bucket, void *key) {
  dictentry_t *e;
  int          ix;

  for (ix = 0; ix < bucket -> size; ix++) {
    e = (dictentry_t *) &(bucket -> entries[ix]);
    if (_dict_cmp_keys(bucket -> dict, e -> key, key) == 0) {
      return e;
    }
  }
  return NULL;
}

bucket_t * _bucket_put(bucket_t *bucket, void *key, void *value) {
  dictentry_t *e;
  
  e = _bucket_find_entry(bucket, key);
  if (e) {
    if (e -> dict -> key_type.free && e -> key) {
      e -> dict -> key_type.free(e -> key);
    }
    e -> key = key;
    if (e -> dict -> data_type.free && e -> value) {
      e -> dict -> data_type.free(e -> value);
    }
    e -> value = value;
  } else {
    _bucket_init_entry(bucket, key, value);
  }
  return bucket;
}

/* -- D I C T  S T A T I C  F U N C T I O N S ----------------------------- */

unsigned int _dict_hash(dict_t *dict, void *key) {
  return (dict -> key_type.hash)
    ? dict -> key_type.hash(key)
    : hash(&key, sizeof(void *));
}

int _dict_cmp_keys(dict_t *dict, void *key1, void *key2) {
  return (dict -> key_type.cmp)
    ? dict -> key_type.cmp(key1, key2)
    : (intptr_t) key1 - (intptr_t) key2;
}

dict_t * _dict_rehash(dict_t *dict) {
  bucket_t    **old_buckets;
  int           i;
  int           j;
  int           old;
  int           bucket_num;
  bucket_t     *bucket;
  bucket_t     *new_bucket;
  dictentry_t  *entry;
  
  old_buckets = dict ->  buckets;
  old = dict -> num_buckets;
  dict -> num_buckets = (old) ? old * 2 : INIT_BUCKETS;;
  dict -> buckets = (bucket_t **) new_ptrarray(dict -> num_buckets);

  for (i = 0; i < dict -> num_buckets; i++) {
    dict -> buckets[i] = _bucket_create(dict, i);
  }

  if (old_buckets) {
    for (i = 0; i < old; i++) {
      bucket = old_buckets[i];
      for (j = 0; j < bucket -> size; j++) {
        entry = &(bucket -> entries[j]);
        bucket_num = (int) (entry -> hash % (unsigned int) dict -> num_buckets);
        new_bucket = dict -> buckets[bucket_num];
        _bucket_copy_entry(new_bucket, entry);
      }
      free(bucket);
    }
    free(old_buckets);
  }  
  return dict;
}

bucket_t * _dict_get_bucket(dict_t *dict, void *key) {
  unsigned int hash;

  hash = _dict_hash(dict, key);
  return (bucket_t *) dict -> buckets[hash % dict -> num_buckets];
}

dict_t * _dict_add_to_bucket(dict_t *dict, void *key, void *value) {
  bucket_t    *bucket;

  bucket = _dict_get_bucket(dict, key);
  _bucket_put(bucket, key, value);
  return dict;
}

dictentry_t * _dict_find_in_bucket(dict_t *dict, void *key) {
  bucket_t *bucket;

  bucket = _dict_get_bucket(dict, key);
  return _bucket_find_entry(bucket, key);
}

dict_t * _dict_remove_from_bucket(dict_t *dict, void *key) {
  bucket_t *bucket;
  int       ix;
  dict_t   *ret;

  bucket = _dict_get_bucket(dict, key);
  ret = NULL;
  if (bucket) {
    ix = _bucket_position_of(bucket, key);
    if (ix >= 0) {
      _bucket_remove(bucket, ix);
      ret = dict;
    }
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

  if (e) {
    type = (dict_reduce_type_t) ((int) ((intptr_t) ctx -> user));
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
  }
  return ctx;
}

void * _dict_reduce(dict_t *dict, reduce_t reducer, void *data, dict_reduce_type_t reducetype) {
  reduce_ctx *ctx;
  void       *ret = NULL;
  bucket_t   *bucket;
  int         ix;

  ctx = reduce_ctx_create((void *) ((intptr_t) reducetype), data, (void_t) reducer);
  for (ix = 0; ix < dict -> num_buckets; ix++) {
    bucket = (bucket_t *) dict -> buckets[ix];
    _bucket_reduce(bucket, (reduce_t) _dict_entry_reducer, ctx);
  }
  ret = ctx -> data;
  free(ctx);
  return ret;
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
  void *key;
  void *value;

  key = (dict -> key_type.copy && e -> key) ? e -> key : dict -> key_type.copy(e -> key);
  value = (dict -> data_type.copy && e -> value) ? e -> value : dict -> data_type.copy(e -> value);
  dict_put(dict, key, value);
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
  str_append_printf(entries, fmt, e -> key, e -> value);
  return ctx;
}

void _dict_visit_buckets(dict_t *dict, visit_t visitor) {
  int ix;
  
  for (ix = 0; ix < dict -> num_buckets; ix++) {
    visitor(dict -> buckets[ix]);
  }
}

/* -- D I C T  P U B L I C  I N T E R F A C E ----------------------------- */

dict_t * dict_create(cmp_t cmp) {
  dict_t  *d;

  d = NEW(dict_t);
  d -> buckets = NULL;
  d -> num_buckets = 0;
  d -> key_type.cmp = cmp;
  d -> size = 0;
  d -> loadfactor = 0.75; /* TODO: Make configurable */
  d -> str = NULL;
  return d;
}

dict_t * dict_clone(dict_t *dict) {
  dict_t *ret = dict_create(NULL);
  
  dict_set_key_type(ret, &(dict -> key_type));
  dict_set_data_type(ret, &(dict -> data_type));
  return ret;
}

dict_t * dict_copy(dict_t *dict) {
  dict_t *ret;

  ret = dict_clone(dict);
  dict_put_all(ret, dict);
  return ret;
}

dict_t * dict_set_key_type(dict_t *dict, type_t *type) {
  type_copy(&(dict -> key_type), type);
  return dict;
}

dict_t * dict_set_data_type(dict_t *dict, type_t *type) {
  type_copy(&(dict -> data_type), type);
  return dict;
}

dict_t * dict_set_hash(dict_t *dict, hash_t hash) {
  dict -> key_type.hash = hash;
  return dict;
}

dict_t * dict_set_free_key(dict_t *dict, free_t free_key) {
  dict -> key_type.free = free_key;
  return dict;
}

dict_t * dict_set_free_data(dict_t *dict, free_t free_data) {
  dict -> data_type.free = free_data;
  return dict;
}

dict_t * dict_set_copy_key(dict_t *dict, copy_t copy_key) {
  dict -> key_type.copy = copy_key;
  return dict;
}

dict_t * dict_set_copy_data(dict_t *dict, copy_t copy_data) {
  dict -> data_type.copy = copy_data;
  return dict;
}

dict_t * dict_set_tostring_key(dict_t *dict, tostring_t tostring_key) {
  dict -> key_type.tostring = tostring_key;
  return dict;
}

dict_t * dict_set_tostring_data(dict_t *dict, tostring_t tostring_data) {
  dict -> data_type.tostring = tostring_data;
  return dict;
}

int dict_size(dict_t *dict) {
  return dict -> size;
}

void dict_free(dict_t *dict) {
  if (dict) {
    _dict_visit_buckets(dict, (visit_t) _bucket_free);
    free(dict -> buckets);
    free(dict -> str);
    free(dict);
  }
}

dict_t * dict_clear(dict_t *dict) {
  if (dict_notempty(dict)) {
    _dict_visit_buckets(dict, (visit_t) _bucket_clear);
    dict -> size = 0;
  }
  return dict;
}

dict_t * dict_put(dict_t *dict, void *key, void *data) {
  dict_t *ret;
  int     had_key;

  if (!dict -> num_buckets || 
      ((float) (dict -> size + 1) / (float) dict -> num_buckets) > dict -> loadfactor) {
    _dict_rehash(dict);
  }

  had_key = dict_has_key(dict, key);
  ret = _dict_add_to_bucket(dict, key, data);
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
 * @return The dictionary if an entry with the given key was found and deleted,
 * NULL otherwise.
 */
dict_t * dict_remove(dict_t *dict, void *key) {
  dict_t *ret = NULL;

  if (dict -> size) {
    ret = _dict_remove_from_bucket(dict, key);
    if (ret) {
      dict -> size--;
    }
  }
  return ret;
}

void * dict_get(dict_t *dict, void *key) {
  dictentry_t *entry;

  if (dict -> size) {
    entry = _dict_find_in_bucket(dict, key);
    return (entry) ? entry -> value : NULL;
  } else {
    return NULL;
  }
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

  if (dict -> size) {
    entry = _dict_find_in_bucket(dict, key);
    ret = (entry) ? entry -> value : NULL;
    if (entry) {
      entry -> value = NULL;
      dict_remove(dict, key);
    }
    return ret;
  } else {
    return NULL;
  }
}

int dict_has_key(dict_t *dict, void *key) {
  return dict -> size && _dict_find_in_bucket(dict, key) != NULL;
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

  assert(dict -> key_type.tostring);
  assert(dict -> data_type.tostring);
  
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
  return dict_tostr_custom(dict, "{\n", "  \"%s\": %s", ",\n", "\n}");
}

char * dict_tostring_custom(dict_t *dict, char *open, char *fmt, char *sep, char *close) {
  str_t *s;
  
  free(dict -> str);
  s = dict_tostr_custom(dict, open, fmt, sep, close);
  dict -> str = str_reassign(s);
  return dict -> str;
}

char * dict_tostring(dict_t *dict) {
  return dict_tostring_custom(dict, "{\n", "  \"%s\": %s", ",\n", "\n}");
}

str_t * dict_dump(dict_t *dict, char *title) {
  bucket_t    *bucket;
  int          len, i, j;
  dictentry_t *entry;
  str_t       *ret;
  
  ret = str_printf("dict_dump -- %s\n", title);
  str_append_printf(ret, "==============================================\n");
  str_append_printf(ret, "Size: %d\n", dict_size(dict));
  str_append_printf(ret, "Buckets: %d\n", dict -> num_buckets);
  for (i = 0; i < dict -> num_buckets; i++) {
    bucket = (bucket_t *) dict -> buckets[i];
    len = bucket -> size;
    str_append_printf(ret, "  Bucket: %d bucket size: %d\n", i, len);
    if (len > 0) {
      debug("  --------------------------------------------\n");
      for (j = 0; j < len; j++) {
        entry = &(bucket -> entries[j]);
        str_append_printf(ret, "    %s (%u) --> %s\n", 
                         (char *) entry -> key, entry -> hash, 
                         (char *) entry -> value);
      }
      str_append_chars(ret, "\n");
    }
  }
  return ret;
}
