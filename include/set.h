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
  dict_t  *dict;
} set_t;

extern set_t *  set_create(cmp_t cmp);
extern set_t *  set_set_free(set_t *, free_t);
extern set_t *  set_set_hash(set_t *, hash_t);
extern set_t *  set_set_tostring(set_t *, tostring_t);
extern void     set_free(set_t *);
extern set_t *  set_add(set_t *, void *);
extern set_t *  set_remove(set_t *, void *);
extern int      set_has(set_t *, void *);
extern int      set_size(set_t *);
extern void *   set_reduce(set_t *, reduce_t, void *);
extern void *   set_reduce_chars(set_t *, reduce_t, void *);
extern void *   set_reduce_str(set_t *, reduce_t, void *);
extern set_t *  set_visit(set_t *, visit_t);
extern set_t *  set_clear(set_t *);
extern set_t *  set_intersect(set_t *, set_t *);
extern set_t *  set_union(set_t *, set_t *);
extern set_t *  set_minus(set_t *, set_t *);
extern int      set_disjoint(set_t *, set_t *);
extern int      set_subsetof(set_t *, set_t *);
extern int      set_cmp(set_t *, set_t *);
extern str_t *  set_tostr(set_t *);

#define intset_create()       set_create(NULL)
#define set_add_int(s, i)     set_add((s), (void *)((long) (i)))
#define set_has_int(s, i)     set_has((s), (void *)((long) (i)))
#define set_remove_int(s, i)  set_remove((s), (void *)((long) (i)))
#define set_empty(s)          (set_size((s)) == 0)

#endif /* __SET_H__ */
