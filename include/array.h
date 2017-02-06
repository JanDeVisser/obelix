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

typedef struct _array {
  void       **contents;
  int          capacity;
  int          size;
  int          curix;
  type_t       type;
  char        *str;
  int          refs;
} array_t;

struct _str;

OBLCORE_IMPEXP array_t *     array_create(int);
OBLCORE_IMPEXP array_t *     array_copy(array_t *);
OBLCORE_IMPEXP array_t *     array_split(const char *, const char *);
OBLCORE_IMPEXP array_t *     array_slice(const array_t *, int, int);
OBLCORE_IMPEXP array_t *     array_set_type(array_t *, const type_t *);
OBLCORE_IMPEXP array_t *     array_set_free(array_t *, free_t);
OBLCORE_IMPEXP array_t *     array_set_cmp(array_t *, cmp_t);
OBLCORE_IMPEXP array_t *     array_set_hash(array_t *, hash_t);
OBLCORE_IMPEXP array_t *     array_set_tostring(array_t *, tostring_t);
OBLCORE_IMPEXP void          array_free(array_t *);
OBLCORE_IMPEXP array_t *     array_clear(array_t *);
OBLCORE_IMPEXP unsigned int  array_hash(const array_t *);
OBLCORE_IMPEXP int           array_capacity(const array_t *);
OBLCORE_IMPEXP int           array_set(array_t *, int, void *);
OBLCORE_IMPEXP void *        array_get(const array_t *, int);
OBLCORE_IMPEXP void *        array_pop(array_t *);
OBLCORE_IMPEXP void *        array_remove(array_t *, int);
OBLCORE_IMPEXP void *        array_reduce(array_t *, reduce_t, void *);
OBLCORE_IMPEXP void *        array_reduce_chars(array_t *, reduce_t, void *);
OBLCORE_IMPEXP void *        array_reduce_str(array_t *, reduce_t, void *);
OBLCORE_IMPEXP array_t *     array_visit(array_t *, visit_t);
OBLCORE_IMPEXP array_t *     array_add_all(array_t *, array_t *);
OBLCORE_IMPEXP struct _str * array_tostr(const array_t *);
OBLCORE_IMPEXP char *        array_tostring(array_t *);
OBLCORE_IMPEXP void          array_debug(array_t *, const char *);
OBLCORE_IMPEXP void *        array_find(array_t *, cmp_t, void *);

OBLCORE_IMPEXP array_t *     array_start(array_t *);
OBLCORE_IMPEXP array_t *     array_end(array_t *);
OBLCORE_IMPEXP void *        array_current(array_t *);
OBLCORE_IMPEXP int           array_has_next(array_t *);
OBLCORE_IMPEXP int           array_has_prev(array_t *);
OBLCORE_IMPEXP void *        array_next(array_t *);
OBLCORE_IMPEXP void *        array_prev(array_t *);

static inline int array_size(const array_t *a) {
  return (a) ? a -> size : -1;
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

static inline array_t * str_array_create(int sz) {
  return array_set_type(array_create(sz), type_str);
}

static inline char * str_array_get(const array_t *a, int ix) {
  return (char *) array_get(a, ix);
}

#define array_join(a, g)        (_str_join((g), (a), (obj_reduce_t) array_reduce_chars))

#endif /* __ARRAY_H__ */
