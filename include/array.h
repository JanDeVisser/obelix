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
  list_t *      list;
  listnode_t ** index;
  int           capacity;
  char *        str;
  int           refs;
} array_t;

extern array_t *    array_create(int);
extern array_t *    array_copy(array_t *);
extern array_t *    array_split(char *, char *);
extern array_t *    array_slice(array_t *, int, int);
extern array_t *    array_set_free(array_t *, visit_t);
extern array_t *    array_set_cmp(array_t *, cmp_t);
extern array_t *    array_set_hash(array_t *, hash_t);
extern array_t *    array_set_tostring(array_t *, tostring_t);
extern void         array_free(array_t *);
extern array_t *    array_clear(array_t *);
extern unsigned int array_hash(array_t *);
extern int          array_capacity(array_t *);
extern int          array_set(array_t *, int, void *);
extern int          array_push(array_t *, void *);
extern void *       array_get(array_t *, int);
extern void *       array_reduce(array_t *, reduce_t, void *);
extern void *       array_reduce_chars(array_t *, reduce_t, void *);
extern void *       array_reduce_str(array_t *, reduce_t, void *);
extern array_t *    array_visit(array_t *, visit_t);
extern array_t *    array_add_all(array_t *, array_t *);
extern str_t *      array_tostr(array_t *);
extern char *       array_tostring(array_t *);
extern void         array_debug(array_t *, char *);

#define array_size(a)            ((a) -> list -> size)

#define array_set_int(a, i, v)   array_put((a), (i), (void *)((long) (v)))
#define array_get_int(a, i)      ((long) array_get((a), (i)));
#define array_push(a, d)         array_set((a), -1, (d))
#define array_empty(a)           (array_size((a)) == 0)
#define array_notempty(a)        (array_size((a)) > 0)

#define str_array_create(i)     array_set_tostring( \
                                  array_set_free( \
                                    array_create((i)), \
                                    (free_t) free), \
                                  (tostring_t) chars)
#define str_array_get(a, i)     ((char *) array_get((a), (i)))
#define array_join(a, g)        (_str_join((g), (a), (obj_reduce_t) array_reduce_chars))

#endif /* __ARRAY_H__ */
