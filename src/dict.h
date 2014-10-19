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

#include "list.h"
#include "array.h"

typedef struct _dict {
  cmp_t    cmp;
  hash_t   hash;
  visit_t  free_key;
  visit_t  free_data;
  array_t *buckets;
  int      num_buckets;
  int      size;
  float    loadfactor;
} dict_t;

typedef struct _entry {
  dict_t *dict;
  void   *key;
  void   *value;
  int     hash;
} entry_t;

extern dict_t * dict_create(cmp_t); /* No hash - Use key point mod something */
extern void     dict_set_hash(dict_t *, hash_t);
extern void     dict_set_free_key(dict_t *, visit_t);
extern void     dict_set_free_data(dict_t *, visit_t);
extern void     dict_free(dict_t *);
extern void     dict_clear(dict_t *);
extern int      dict_put(dict_t *, void *, void *);
extern void *   dict_get(dict_t *, void *);
extern void *   dict_remove(dict_t *, void *);
extern int      dict_size(dict_t *);
extern list_t * dict_keys(dict_t *);
extern list_t * dict_values(dict_t *);
extern list_t * dict_items(dict_t *);
extern int      dict_has_key(dict_t *, void *);
extern void *   dict_reduce(dict_t *, reduce_t, void *);
extern void     dict_visit(dict_t *, visit_t);

#endif /* __DICT_H__ */
