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

#include <array.h>
#include <list.h>

typedef struct _dict {
  cmp_t       cmp;
  hash_t      hash;
  visit_t     free_key;
  visit_t     free_data;
  tostring_t  tostring_key;
  tostring_t  tostring_data;
  array_t    *buckets;
  int         num_buckets;
  int         size;
  float       loadfactor;
} dict_t;

typedef struct _entry {
  void         *key;
  void         *value;
} entry_t;

extern void     entry_free(entry_t *e);

extern dict_t * dict_create(cmp_t); /* No hash - Use key point mod something */
extern dict_t * dict_set_hash(dict_t *, hash_t);
extern dict_t * dict_set_free_key(dict_t *, free_t);
extern dict_t * dict_set_free_data(dict_t *, free_t);
extern dict_t * dict_set_tostring_key(dict_t *, tostring_t);
extern dict_t * dict_set_tostring_data(dict_t *, tostring_t);
extern void     dict_free(dict_t *);
extern dict_t * dict_clear(dict_t *);
extern dict_t * dict_put(dict_t *, void *, void *);
extern int      dict_has_key(dict_t *, void *);
extern void *   dict_get(dict_t *, void *);
extern dict_t * dict_remove(dict_t *, void *);
extern int      dict_size(dict_t *);
extern list_t * dict_keys(dict_t *);
extern list_t * dict_values(dict_t *);
extern list_t * dict_items(dict_t *);
extern int      dict_has_key(dict_t *, void *);
extern void *   dict_reduce(dict_t *, reduce_t, void *);
extern void *   dict_reduce_keys(dict_t *, reduce_t, void *);
extern void *   dict_reduce_values(dict_t *, reduce_t, void *);
extern void *   _dict_reduce_dictentries(dict_t *, reduce_t, void *);
extern dict_t * dict_visit(dict_t *, visit_t);
extern dict_t * dict_visit_keys(dict_t *, visit_t);
extern dict_t * dict_visit_values(dict_t *, visit_t);
extern dict_t * _dict_visit_dictentries(dict_t *, visit_t);
extern dict_t * dict_put_all(dict_t *, dict_t *);

extern void     dict_dump(dict_t *, char *);

#define strstr_dict_create()    dict_set_free_data( \
                                  dict_set_free_key( \
                                    dict_set_hash( \
                                      dict_create((cmp_t) strcmp),\
                                      (hash_t) strhash), \
                                    (free_t) free), \
                                  (free_t) free)
#define strvoid_dict_create()   dict_set_free_key( \
                                  dict_set_hash( \
                                    dict_create((cmp_t) strcmp),\
                                    (hash_t) strhash), \
                                (free_t) free)
#define intdict_create()        dict_create(NULL)
#define dict_put_int(d, i, v)   dict_put((d), (void *)((long) (i)), (v))
#define dict_get_int(d, i)      dict_get((d), (void *)((long) (i)))
#define dict_has_int(d, i)      dict_has_key((d), (void *)((long) (i)))
#define dict_remove_int(d, i)   dict_remove((d), (void *)((long) (i)))
#define dict_empty(d)           (dict_size((d)) == 0)
#define dict_notempty(d)        (dict_size((d)) > 0)



#endif /* __DICT_H__ */
