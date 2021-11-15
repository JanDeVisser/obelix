/*
 * dict.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __DICT_H__
#define __DICT_H__

#include <string.h>

#include <array.h>
#include <list.h>

#define INIT_BUCKETS       4
#define INIT_BUCKET_SIZE   4

typedef struct _entry {
  void         *key;
  void         *value;
} entry_t;

typedef struct _dictentry {
  struct _dict *dict;
  entry_t       entry;
  //unsigned int  hash;
  int           ix;
} dictentry_t;

typedef struct _bucket {
  struct _dict *dict;
  int           ix;
  int           capacity;
  int           size;
  union {
    dictentry_t   entries[INIT_BUCKET_SIZE];
    dictentry_t  *refentries;
  };
} bucket_t;

typedef struct _dict {
  type_t     key_type;
  type_t     data_type;
  int        num_buckets;
  union {
    bucket_t   buckets[INIT_BUCKETS];
    bucket_t  *refbuckets;
  };
  int        size;
  float      loadfactor;
  char      *str;
} dict_t;

typedef struct _dictiterator {
  dict_t  *dict;
  int      current_bucket;
  int      current_entry;
  entry_t  current;
} dictiterator_t;

extern void          entry_free(entry_t *e);

extern dict_t *      dict_create(cmp_t); /* No hash - Use key point mod something */
extern dict_t *      dict_clone(const dict_t *);
extern dict_t *      dict_copy(const dict_t *);
extern dict_t *      dict_set_key_type(dict_t *, const type_t *);
extern dict_t *      dict_set_data_type(dict_t *, const type_t *);
extern dict_t *      dict_set_hash(dict_t *, hash_t);
extern dict_t *      dict_set_free_key(dict_t *, free_t);
extern dict_t *      dict_set_free_data(dict_t *, free_t);
extern dict_t *      dict_set_copy_key(dict_t *, copy_t);
extern dict_t *      dict_set_copy_data(dict_t *, copy_t);
extern dict_t *      dict_set_tostring_key(dict_t *, tostring_t);
extern dict_t *      dict_set_tostring_data(dict_t *, tostring_t);
extern void          dict_free(dict_t *);
extern dict_t *      dict_clear(dict_t *);
extern dict_t *      dict_put(dict_t *, void *, void *);
extern int           dict_has_key(const dict_t *, const void *);
extern void *        dict_get(const dict_t *, const void *);
extern dict_t *      dict_remove(dict_t *, void *);
extern void *        dict_pop(dict_t *, void *);
extern int           dict_size(const dict_t *);
extern list_t *      dict_keys(dict_t *);
extern list_t *      dict_values(dict_t *);
extern list_t *      dict_items(dict_t *);
extern void *        dict_reduce(dict_t *, reduce_t, void *);
extern void *        dict_reduce_keys(dict_t *, reduce_t, void *);
extern void *        dict_reduce_values(dict_t *, reduce_t, void *);
extern void *        dict_reduce_chars(dict_t *, reduce_t, void *);
extern void *        _dict_reduce_dictentries(dict_t *, reduce_t, void *);
extern dict_t *      dict_visit(dict_t *, visit_t);
extern dict_t *      dict_visit_keys(dict_t *, visit_t);
extern dict_t *      dict_visit_values(dict_t *, visit_t);
extern dict_t *      _dict_visit_dictentries(dict_t *, visit_t);
extern dict_t *      dict_put_all(dict_t *, const dict_t *);
extern struct _str * dict_tostr(dict_t *);
extern struct _str * dict_tostr_custom(dict_t *, const char *,
                                               const char *, const char *,
                                               const char *);
extern char *        dict_tostring(dict_t *);
extern char *        dict_tostring_custom(dict_t *, const char *,
                                                  const char *, const char *,
                                                  const char *);

extern struct _str * dict_dump(const dict_t *, const char *);

#define strvoid_dict_create()   (dict_set_key_type(dict_create(NULL), coretype(CTString)))
#define strdict_create()        (strvoid_dict_create())
#define strint_dict_create()    (dict_set_data_type(strvoid_dict_create(), coretype(CTInteger)))
#define strstr_dict_create()    (dict_set_data_type(strvoid_dict_create(), coretype(CTString)))

#define intvoid_dict_create()   (dict_set_key_type(dict_create(NULL), coretype(CTInteger)))
#define intdict_create()        (intvoid_dict_create())
#define intint_dict_create()    (dict_set_data_type(intvoid_dict_create(), coretype(CTInteger)))
#define intstr_dict_create()    (dict_set_data_type(intvoid_dict_create(), coretype(CTString)))

#define dict_put_int(d, i, v)   dict_put((d), (void *)((intptr_t) (i)), (v))
#define dict_get_int(d, i)      dict_get((d), (void *)((intptr_t) (i)))
#define dict_has_int(d, i)      dict_has_key((d), (void *)((intptr_t) (i)))
#define dict_remove_int(d, i)   dict_remove((d), (void *)((intptr_t) (i)))
#define dict_empty(d)           (dict_size((d)) == 0)
#define dict_notempty(d)        (dict_size((d)) > 0)

extern dictiterator_t * di_create(dict_t *);
extern void             di_free(dictiterator_t *);
extern void             di_head(dictiterator_t *);
extern void             di_tail(dictiterator_t *);
extern entry_t *        di_current(dictiterator_t *);
extern int              di_has_next(dictiterator_t *);
extern int              di_has_prev(dictiterator_t *);
extern entry_t *        di_next(dictiterator_t *);
extern entry_t *        di_prev(dictiterator_t *);
extern int              di_atstart(dictiterator_t *);
extern int              di_atend(dictiterator_t *);

#endif /* __DICT_H__ */
