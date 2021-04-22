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

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _array {
  void **contents;
  int capacity;
  int size;
  int curix;
  type_t type;
  char *str;
  int refs;
} array_t;

struct _str;

extern array_t *array_create(int);
extern array_t *array_copy(array_t *);
extern array_t *array_split(const char *, const char *);
extern array_t *array_slice(const array_t *, int, int);
extern array_t *array_set_type(array_t *, const type_t *);
extern array_t *array_set_free(array_t *, free_t);
extern array_t *array_set_cmp(array_t *, cmp_t);
extern array_t *array_set_hash(array_t *, hash_t);
extern array_t *array_set_tostring(array_t *, tostring_t);
extern void array_free(array_t *);
extern array_t *array_clear(array_t *);
extern unsigned int array_hash(const array_t *);
extern int array_capacity(const array_t *);
extern int array_set(array_t *, int, void *);
extern void *array_get(const array_t *, int);
extern void *array_pop(array_t *);
extern void *array_remove(array_t *, int);
extern void *array_reduce(array_t *, reduce_t, void *);
extern void *array_reduce_chars(array_t *, reduce_t, void *);
extern void *array_reduce_str(array_t *, reduce_t, void *);
extern array_t *array_visit(array_t *, visit_t);
extern array_t *array_add_all(array_t *, array_t *);
extern struct _str *array_tostr(const array_t *);
extern char *array_tostring(array_t *);
extern void array_debug(array_t *, const char *);
extern void *array_find(array_t *, cmp_t, void *);

extern array_t *array_start(array_t *);
extern array_t *array_end(array_t *);
extern void *array_current(array_t *);
extern int array_has_next(array_t *);
extern int array_has_prev(array_t *);
extern void *array_next(array_t *);
extern void *array_prev(array_t *);

static inline int array_size(const array_t *a) {
  return (a) ? a->size : -1;
}

static inline int array_set_int(array_t *a, int i, intptr_t v) {
  return array_set(a, i, (void *) v);
}

static inline intptr_t array_get_int(const array_t *a, int i) {
  return (intptr_t) array_get(a, i);
}

static inline int array_push(array_t *a, void *d) {
  return array_set(a, -1, d);
}

static inline int array_empty(const array_t *a) {
  return array_size(a) == 0;
}

static inline int array_notempty(const array_t *a) {
  return array_size(a) > 0;
}

static inline array_t *str_array_create(int sz) {
  return array_set_type(array_create(sz), coretype(CTString));
}

static inline char *str_array_get(const array_t *a, int ix) {
  return (char *) array_get(a, ix);
}

#define array_join(a, g)        (_str_join((g), (a), (obj_reduce_t) array_reduce_chars))

#ifdef __cplusplus
}
#endif

#endif /* __ARRAY_H__ */
