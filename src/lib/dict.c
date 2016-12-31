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

#include "libcore.h"
#include <dict.h>
#include <str.h>

typedef enum _dict_reduce_type {
  DRTEntries,
  DRTEntriesNoFree,
  DRTDictEntries,
  DRTKeys,
  DRTValues,
  DRTStringString
} dict_reduce_type_t;

static void             _dictentry_free(dictentry_t *);
static unsigned int     _dictentry_hash(const dictentry_t *);
static int              _dictentry_cmp_key(const dictentry_t *, const void *);

#define _dictentry_entry(e)  (&((e) -> entry))

static entry_t *        _entry_from_dictentry(const dictentry_t *);
static entry_t *        _entry_from_entry(const entry_t *);

static bucket_t *       _bucket_initialize(bucket_t *, dict_t *, int);
static void             _bucket_clear(bucket_t *);
static bucket_t *       _bucket_expand(bucket_t *);
static bucket_t *       _bucket_copy_entry(bucket_t *, const dictentry_t *);
static bucket_t *       _bucket_init_entry(bucket_t *, void *, void *);
static void *           _bucket_reduce(bucket_t *, reduce_t, void *);
static void             _bucket_remove(bucket_t *, int);
static dictentry_t *    _bucket_find_entry(const bucket_t *, const void *);
static int              _bucket_position_of(const bucket_t *, const void *);
static int              _bucket_put(bucket_t *, void *, void *);

#define _bucket_get_entry(b, e)   (((b) -> capacity == INIT_BUCKET_SIZE) \
                                    ? &((b) -> entries[(e)]) \
                                    : &((b) -> refentries[(e)]))

static unsigned int     _dict_hash(const dict_t *, const void *);
static int              _dict_cmp_keys(const dict_t *, const void *, const void *);
static dict_t *         _dict_rehash(dict_t *);
static bucket_t *       _dict_get_bucket_bykey(const dict_t *, const void *);
static int              _dict_add_to_bucket(dict_t *, void *, void *);
static dictentry_t *    _dict_find_in_bucket(const dict_t *, const void *);
static dict_t *         _dict_remove_from_bucket(dict_t *, void *);
static list_t *         _dict_append_reducer(void *, list_t *);
static void *           _dict_visitor(void *, reduce_ctx *);
static dict_t *         _dict_visit(dict_t *, visit_t, dict_reduce_type_t);
static void *           _dict_entry_reducer(dictentry_t *, reduce_ctx *);
static void *           _dict_reduce(dict_t *, reduce_t, void *, dict_reduce_type_t);
static dict_t *         _dict_put_all_reducer(entry_t *, dict_t *);
static void **          _dict_entry_formatter(entry_t *, void **);
static void             _dict_visit_buckets(dict_t *, visit_t);

#define _dict_get_bucket(d, b)   (((d) -> num_buckets == INIT_BUCKETS) \
                                    ? &((d) -> buckets[b]) \
                                    : &((d) -> refbuckets[b]))

// -- D I C T E N T R Y  S T A T I C  F U N C T I O N S ------------------- */

void _dictentry_free(dictentry_t *entry) {
  if (entry) {
    if (entry -> dict -> key_type.free && entry -> entry.key) {
      entry -> dict -> key_type.free(entry -> entry.key);
    }
    if (entry -> dict -> data_type.free && entry -> entry.value) {
      entry -> dict -> data_type.free(entry -> entry.value);
    }
  }
}

_unused_ unsigned int _dictentry_hash(const dictentry_t *entry) {
  return _dict_hash(entry -> dict, entry -> entry.key);
}

_unused_ int _dictentry_cmp_key(const dictentry_t *entry, const void *other) {
  return _dict_cmp_keys(entry -> dict, entry -> entry.key, other);
}

// -- E N T R Y  S T A T I C  F U N C T I O N S --------------------------- */

entry_t * _entry_from_dictentry(const dictentry_t *e) {
  entry_t *ret;

  ret = NEW(entry_t);
  ret -> key = e -> entry.key;
  ret -> value = e -> entry.value;
  return ret;
}

_unused_ entry_t * _entry_from_entry(const entry_t *e) {
  entry_t *ret;

  ret = NEW(entry_t);
  ret -> key = e -> key;
  ret -> value = e -> value;
  return ret;
}

entry_t * _entry_from_strings(const dictentry_t *e) {
  entry_t *ret;

  ret = NEW(entry_t);
  assert(e -> dict -> key_type.tostring);
  assert(e -> dict -> data_type.tostring);
  ret -> key = strdup(e -> dict -> key_type.tostring(e -> entry.key));
  ret -> value = strdup(e -> dict -> data_type.tostring(e -> entry.value));
  return ret;
}

void entry_free(entry_t *e) {
  if (e) {
    free(e);
  }
}

/* -- B U C K E T  M A N A G E M E N T ------------------------------------ */


static bucket_t * _bucket_initialize(bucket_t *bucket, dict_t *dict, int num) {
  int ix;

  for (ix = 0; ix < num; ix++, bucket++) {
    bucket -> dict = dict;
    bucket -> ix = ix;
    bucket -> capacity = INIT_BUCKET_SIZE;
    bucket -> size = 0;
  }
  return bucket;
}

void _bucket_clear(bucket_t *bucket) {
  int ix;

  if (bucket) {
    for (ix = 0; ix < bucket -> size; ix++) {
      _dictentry_free(_bucket_get_entry(bucket, ix));
    }
    if (bucket -> capacity > INIT_BUCKET_SIZE) {
      free(bucket -> refentries);
      bucket -> capacity = INIT_BUCKET_SIZE;
    }
    bucket -> size = 0;
  }
}

bucket_t * _bucket_expand(bucket_t *bucket) {
  dictentry_t *new_entries;

  new_entries = NEWARR(2 * bucket -> capacity, dictentry_t);
  if (bucket -> capacity == INIT_BUCKET_SIZE) {
    memcpy(new_entries, bucket -> entries, bucket -> capacity * sizeof(dictentry_t));
  } else {
    memcpy(new_entries, bucket -> refentries, bucket -> capacity * sizeof(dictentry_t));
  }
  bucket -> refentries = new_entries;
  bucket -> capacity *= 2;
  return bucket;
}

bucket_t * _bucket_copy_entry(bucket_t *bucket, const dictentry_t *entry) {
  dictentry_t *e;

  if (bucket -> size >= bucket -> capacity) {
    bucket = _bucket_expand(bucket);
  }

  e = _bucket_get_entry(bucket, bucket -> size);
  memcpy(e, entry, sizeof(dictentry_t));
  e -> ix = bucket -> size;
  bucket -> size++;
  return bucket;
}

bucket_t * _bucket_init_entry(bucket_t *bucket, void *key, void *value) {
  dictentry_t *entry;

  if (bucket -> size >= bucket -> capacity) {
    bucket = _bucket_expand(bucket);
  }
  entry = _bucket_get_entry(bucket, bucket -> size);
  entry -> dict = bucket -> dict;
  entry -> entry.key = key;
  entry -> entry.value = value;
  //entry -> hash = _dict_hash(bucket -> dict, key);
  entry -> ix = bucket -> size;
  bucket -> size++;
  return bucket;
}

void * _bucket_reduce(bucket_t *bucket, reduce_t reducer, void *data) {
  int ix;

  for (ix = 0; ix < bucket -> size; ix++) {
    data = reducer(_bucket_get_entry(bucket, ix), data);
  }
  return data;
}

void _bucket_remove(bucket_t *bucket, int ix) {
  int          i;
  dictentry_t *entry;

  assert(bucket -> size > ix);
  entry = _bucket_get_entry(bucket, ix);
  _dictentry_free(entry);
  bucket -> size--;
  if (ix < bucket -> size) {
    memmove(entry, entry + 1, (bucket -> size - ix) * sizeof(dictentry_t));
    for (i = ix; i < bucket -> size; i++) {
      _bucket_get_entry(bucket, i) -> ix = i;
    }
  }
}

int _bucket_position_of(const bucket_t *bucket, const void *key) {
  dictentry_t *e = _bucket_find_entry(bucket, key);

  return (e) ? e -> ix : -1;
}

dictentry_t * _bucket_find_entry(const bucket_t *bucket, const void *key) {
  dictentry_t *e;
  int          ix;

  for (ix = 0; ix < bucket -> size; ix++) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    e = _bucket_get_entry(bucket, ix);
#pragma GCC diagnostic pop
    if (_dict_cmp_keys(bucket -> dict, e -> entry.key, key) == 0) {
      return e;
    }
  }
  return NULL;
}

/**
 * @return 0 if the key was overwritten, 1 if the key was added.
 */
int _bucket_put(bucket_t *bucket, void *key, void *value) {
  dictentry_t *e;

  e = _bucket_find_entry(bucket, key);
  if (e) {
    if (e -> dict -> data_type.free && e -> entry.value) {
      e -> dict -> data_type.free(e -> entry.value);
    }
    e -> entry.value = value;
    return 0;
  } else {
    _bucket_init_entry(bucket, key, value);
    return 1;
  }
}

/* -- D I C T  S T A T I C  F U N C T I O N S ----------------------------- */

unsigned int _dict_hash(const dict_t *dict, const void *key) {
  return (dict -> key_type.hash)
    ? dict -> key_type.hash(key)
    : hash(&key, sizeof(void *));
}

int _dict_cmp_keys(const dict_t *dict, const void *key1, const void *key2) {
  return (dict -> key_type.cmp)
    ? dict -> key_type.cmp(key1, key2)
    : (intptr_t) key1 - (intptr_t) key2;
}

dict_t * _dict_rehash(dict_t *dict) {
  bucket_t     *old_buckets;
  bucket_t     *new_buckets = NULL;
  int           i;
  int           j;
  int           old;
  int           bucket_num;
  bucket_t     *bucket;
  dictentry_t  *entry;

  old = dict -> num_buckets;
  switch (old) {
    case 0:
      old_buckets = NULL;
      new_buckets = dict -> buckets;
      break;
    case INIT_BUCKETS:
      old_buckets = dict -> buckets;
      break;
    default:
      old_buckets = dict -> refbuckets;
      break;
  }
  dict -> num_buckets = (old) ? old * 2 : INIT_BUCKETS;
  if (!new_buckets) {
    dict -> refbuckets = NEWARR(dict -> num_buckets, bucket_t);
    new_buckets = dict -> refbuckets;
  }

  _bucket_initialize(new_buckets, dict, dict -> num_buckets);

  if (old_buckets) {
    for (i = 0; i < old; i++) {
      bucket = old_buckets + i;
      for (j = 0; j < bucket -> size; j++) {
        entry = _bucket_get_entry(bucket, j);
        bucket_num = (int) (_dict_hash(dict, entry -> entry.key) % ((unsigned int) dict -> num_buckets));
        _bucket_copy_entry(new_buckets + bucket_num, entry);
      }
    }
    if (old_buckets != dict -> buckets) {
      free(old_buckets);
    }
  }
  return dict;
}

bucket_t * _dict_get_bucket_bykey(const dict_t *dict, const void *key) {
  unsigned int  hash;
  bucket_t     *ret;

  hash = _dict_hash(dict, key);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  ret = _dict_get_bucket(dict, hash % dict -> num_buckets);
#pragma GCC diagnostic pop
  return ret;
}

int _dict_add_to_bucket(dict_t *dict, void *key, void *value) {
  bucket_t    *bucket;

  bucket = _dict_get_bucket_bykey(dict, key);
  return _bucket_put(bucket, key, value);
}

dictentry_t * _dict_find_in_bucket(const dict_t *dict, const void *key) {
  bucket_t *bucket;

  bucket = _dict_get_bucket_bykey(dict, key);
  return _bucket_find_entry(bucket, key);
}

dict_t * _dict_remove_from_bucket(dict_t *dict, void *key) {
  bucket_t *bucket;
  int       ix;
  dict_t   *ret;

  bucket = _dict_get_bucket_bykey(dict, key);
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
      elem = _dictentry_entry(e);
      break;
    case DRTEntriesNoFree:
      elem = _entry_from_dictentry(e);
      break;
    case DRTDictEntries:
      elem = e;
      break;
    case DRTKeys:
      elem = e -> entry.key;
      break;
    case DRTValues:
      elem = e -> entry.value;
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
    if (type == DRTStringString) {
      entry = (entry_t *) elem;
      free(entry -> key);
      free(entry -> value);
      free(entry);
    }
  }
  return ctx;
}

void * _dict_reduce(dict_t *dict, reduce_t reducer, void *data, dict_reduce_type_t reducetype) {
  reduce_ctx  ctx;
  void       *ret = NULL;
  bucket_t   *bucket;
  int         ix;

  reduce_ctx_initialize(&ctx, (void *) ((intptr_t) reducetype), data, (void_t) reducer);
  for (ix = 0; ix < dict -> num_buckets; ix++) {
    bucket = _dict_get_bucket(dict, ix);
    _bucket_reduce(bucket, (reduce_t) _dict_entry_reducer, &ctx);
  }
  ret = ctx.data;
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
    visitor(_dict_get_bucket(dict, ix));
  }
}

/* -- D I C T  P U B L I C  I N T E R F A C E ----------------------------- */

dict_t * dict_create(cmp_t cmp) {
  dict_t  *d;

  d = NEW(dict_t);
  d -> num_buckets = 0;
  d -> key_type.cmp = cmp;
  d -> size = 0;
  d -> loadfactor = 0.75; /* TODO: Make configurable */
  d -> str = NULL;
  return d;
}

dict_t * dict_clone(const dict_t *dict) {
  dict_t *ret = dict_create(NULL);

  dict_set_key_type(ret, &(dict -> key_type));
  dict_set_data_type(ret, &(dict -> data_type));
  return ret;
}

dict_t * dict_copy(const dict_t *dict) {
  dict_t *ret;

  ret = dict_clone(dict);
  dict_put_all(ret, dict);
  return ret;
}

dict_t * dict_set_key_type(dict_t *dict, const type_t *type) {
  type_copy(&(dict -> key_type), type);
  return dict;
}

dict_t * dict_set_data_type(dict_t *dict, const type_t *type) {
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

int dict_size(const dict_t *dict) {
  return dict -> size;
}

void dict_free(dict_t *dict) {
  if (dict) {
    _dict_visit_buckets(dict, (visit_t) _bucket_clear);
    if (dict -> num_buckets > INIT_BUCKETS) {
      free(dict -> refbuckets);
    }
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
  if (!dict -> num_buckets ||
      ((float) (dict -> size + 1) / (float) dict -> num_buckets) > dict -> loadfactor) {
    _dict_rehash(dict);
  }
  dict -> size += _dict_add_to_bucket(dict, key, data);
  return dict;
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

void * dict_get(const dict_t *dict, const void *key) {
  dictentry_t *entry;

  if (dict -> size) {
    entry = _dict_find_in_bucket(dict, key);
    return (entry) ? entry -> entry.value : NULL;
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
    ret = (entry) ? entry -> entry.value : NULL;
    if (entry) {
      entry -> entry.value = NULL;
      dict_remove(dict, key);
    }
    return ret;
  } else {
    return NULL;
  }
}

int dict_has_key(const dict_t *dict, const void *key) {
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

dict_t * dict_put_all(dict_t *dict, const dict_t *other) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  return dict_reduce(other, (reduce_t) _dict_put_all_reducer, dict);
#pragma GCC diagnostic pop
}

str_t * dict_tostr_custom(dict_t *dict, const char *open, const char *fmt,
                          const char *sep, const char *close) {
  str_t      *ret;
  str_t      *entries;
  void       *ctx[4];

  if (!dict) {
    return NULL;
  }
  assert(dict -> key_type.tostring);
  assert(dict -> data_type.tostring);

  ret = str_copy_chars(open);
  entries = str_create(0);
  ctx[0] = dict;
  ctx[1] = entries;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  ctx[2] = sep;
  ctx[3] = fmt;
#pragma GCC diagnostic pop
  dict_reduce_chars(dict, (reduce_t) _dict_entry_formatter, ctx);
  str_append(ret, entries);
  str_append_chars(ret, close);
  str_free(entries);
  return ret;
}

str_t * dict_tostr(dict_t *dict) {
  return dict_tostr_custom(dict, "{\n", "  \"%s\": %s", ",\n", "\n}");
}

char * dict_tostring_custom(dict_t *dict, const char *open,
                            const char *fmt, const char *sep,
                            const char *close) {
  str_t *s;

  if (dict) {
    free(dict -> str);
    s = dict_tostr_custom(dict, open, fmt, sep, close);
    dict -> str = str_reassign(s);
    return dict -> str;
  } else {
    return NULL;
  }
}

char * dict_tostring(dict_t *dict) {
  return dict_tostring_custom(dict, "{\n", "  \"%s\": %s", ",\n", "\n}");
}

str_t * dict_dump(const dict_t *dict, const char *title) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  bucket_t    *bucket;
  int          len, i, j;
  dictentry_t *entry;
  str_t       *ret;

  ret = str_printf("dict_dump -- %s\n", title);
  str_append_printf(ret, "==============================================\n");
  str_append_printf(ret, "Size: %d\n", dict_size(dict));
  str_append_printf(ret, "Buckets: %d\n", dict -> num_buckets);
  for (i = 0; i < dict -> num_buckets; i++) {
    bucket = _dict_get_bucket(dict, i);
    len = bucket -> size;
    str_append_printf(ret, "  Bucket: %d bucket size: %d\n", i, len);
    if (len > 0) {
      mdebug(core, "  --------------------------------------------\n");
      for (j = 0; j < len; j++) {
        entry = &(bucket -> entries[j]);
        str_append_printf(ret, "    %s (%u) --> %s\n",
                         (char *) entry -> entry.key, _dict_hash(dict, entry -> entry.key),
                         (char *) entry -> entry.value);
      }
      str_append_chars(ret, "\n");
    }
  }
  return ret;
#pragma GCC diagnostic pop
}

/* -- D I C T I T E R A T O R --------------------------------------------- */

static int _di_bucket_has_next(dictiterator_t *, int, int);
static int _di_bucket_has_prev(dictiterator_t *, int, int);
static int _di_bucket_next(dictiterator_t *, int *);
static int _di_bucket_prev(dictiterator_t *, int *);

int _di_bucket_has_next(dictiterator_t *di, int bucket, int entry) {
  return ((bucket >= 0) &&
          (bucket < di -> dict -> num_buckets) &&
          _dict_get_bucket(di -> dict, bucket) -> size &&
          (entry < _dict_get_bucket(di -> dict, bucket) -> size - 1));
}

int _di_bucket_has_prev(dictiterator_t *di, int bucket, int entry) {
  return ((bucket >= 0) &&
          (bucket < di -> dict -> num_buckets) &&
          _dict_get_bucket(di -> dict, bucket) -> size &&
          (entry > 0) &&
          (entry <= _dict_get_bucket(di -> dict, bucket) -> size));
}

int _di_bucket_next(dictiterator_t *di, int *entry) {
  int bucket;
  int dummy;

  if (!entry) {
    entry = &dummy;
  }
  for (bucket = di -> current_bucket, *entry = di -> current_entry;
       (bucket < di -> dict -> num_buckets) &&
       !_di_bucket_has_next(di, bucket, *entry);
       bucket++, *entry = -1);
  return (bucket < di -> dict -> num_buckets) ? bucket : -1;
}

int _di_bucket_prev(dictiterator_t *di, int *entry) {
  int bucket = di -> current_bucket;
  int dummy;

  if (!entry) {
    entry = &dummy;
  }
  *entry = di -> current_entry;
  while ((bucket >= 0) && !_di_bucket_has_prev(di, bucket, *entry)) {
    if (--bucket >= 0) {
      *entry = _dict_get_bucket(di -> dict, bucket) -> size;
    }
  }
  return bucket;
}

/* ------------------------------------------------------------------------ */

dictiterator_t * di_create(dict_t *dict) {
  dictiterator_t *ret = NEW(dictiterator_t);

  ret -> dict = dict;
  di_head(ret);
  return ret;
}

void di_free(dictiterator_t *di) {
  if (di) {
    free(di);
  }
}

void di_head(dictiterator_t *di) {
  di -> current_bucket = -1;
  di -> current_entry = -1;
}

void di_tail(dictiterator_t *di) {
  di -> current_bucket = di -> dict -> num_buckets - 1;
  di -> current_entry = (di -> current_bucket >= 0)
    ? _dict_get_bucket(di -> dict, di -> current_bucket) -> size
    : -1;
}

entry_t * di_current(dictiterator_t *di) {
  bucket_t *bucket;

  if ((di -> current_bucket >= 0) && (di -> current_bucket < di -> dict -> num_buckets)) {
    bucket = _dict_get_bucket(di -> dict, di -> current_bucket);
    if (di -> current_entry < bucket -> size) {
      return _dictentry_entry(_bucket_get_entry(bucket, di -> current_entry));
    }
  }
  return NULL;
}

int di_has_next(dictiterator_t *di) {
  return _di_bucket_next(di, NULL) >= 0;
}

int di_has_prev(dictiterator_t *di) {
  return _di_bucket_prev(di, NULL) >= 0;
}

entry_t * di_next(dictiterator_t *di) {
  int      entry;
  int      bucket = _di_bucket_next(di, &entry);
  entry_t *ret = NULL;

  if (bucket >= 0) {
    di -> current_bucket = bucket;
    di -> current_entry = ++entry;
    ret = _dictentry_entry(
            _bucket_get_entry(
              _dict_get_bucket(di -> dict, bucket), di -> current_entry));
  }
  return ret;
}

entry_t * di_prev(dictiterator_t *di) {
  int      entry;
  int      bucket = _di_bucket_prev(di, &entry);
  entry_t *ret = NULL;

  if (bucket >= 0) {
    di -> current_bucket = bucket;
    di -> current_entry = --entry;
    ret = _dictentry_entry(
            _bucket_get_entry(
              _dict_get_bucket(di -> dict, bucket), di -> current_entry));
  }
  return ret;
}

int di_atstart(dictiterator_t *di) {
  return !di_has_prev(di);
}

int di_atend(dictiterator_t *di) {
  return !di_has_next(di);

}
