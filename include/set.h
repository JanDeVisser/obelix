/*
 * set.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __SET_H__
#define __SET_H__

#include <dict.h>
#include <str.h>

typedef struct _set {
  dict_t *dict;
  char   *str;
} set_t;

OBLCORE_IMPEXP set_t *  set_create(cmp_t cmp);
OBLCORE_IMPEXP set_t *  set_clone(set_t *);
OBLCORE_IMPEXP set_t *  set_copy(set_t *);
OBLCORE_IMPEXP set_t *  set_set_type(set_t *, type_t *);
OBLCORE_IMPEXP set_t *  set_set_free(set_t *, free_t);
OBLCORE_IMPEXP set_t *  set_set_hash(set_t *, hash_t);
OBLCORE_IMPEXP set_t *  set_set_copy(set_t *, copy_t);
OBLCORE_IMPEXP set_t *  set_set_tostring(set_t *, tostring_t);
OBLCORE_IMPEXP void     set_free(set_t *);
OBLCORE_IMPEXP set_t *  set_add(set_t *, void *);
OBLCORE_IMPEXP set_t *  set_remove(set_t *, void *);
OBLCORE_IMPEXP int      set_has(set_t *, void *);
OBLCORE_IMPEXP int      set_size(set_t *);
OBLCORE_IMPEXP void *   set_reduce(set_t *, reduce_t, void *);
OBLCORE_IMPEXP void *   set_reduce_chars(set_t *, reduce_t, void *);
OBLCORE_IMPEXP void *   set_reduce_str(set_t *, reduce_t, void *);
OBLCORE_IMPEXP set_t *  set_visit(set_t *, visit_t);
OBLCORE_IMPEXP set_t *  set_clear(set_t *);
OBLCORE_IMPEXP set_t *  set_intersect(set_t *, set_t *);
OBLCORE_IMPEXP set_t *  set_union(set_t *, set_t *);
OBLCORE_IMPEXP set_t *  set_minus(set_t *, set_t *);
OBLCORE_IMPEXP int      set_disjoint(set_t *, set_t *);
OBLCORE_IMPEXP int      set_subsetof(set_t *, set_t *);
OBLCORE_IMPEXP int      set_cmp(set_t *, set_t *);
OBLCORE_IMPEXP void *   set_find(set_t *, cmp_t, void *);
OBLCORE_IMPEXP str_t *  set_tostr(set_t *);
OBLCORE_IMPEXP char *   set_tostring(set_t *);

#define intset_create()       (set_set_type(set_create(NULL), &type_int))
#define strset_create()       (set_set_type(set_create(NULL), &type_str))
#define set_add_int(s, i)     set_add((s), (void *)((intptr_t) (i)))
#define set_has_int(s, i)     set_has((s), (void *)((intptr_t) (i)))
#define set_remove_int(s, i)  set_remove((s), (void *)((intptr_t) (i)))
#define set_empty(s)          (set_size((s)) == 0)

#endif /* __SET_H__ */
