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

#include "list.h"

typedef struct _array {
  list_t *      list;
  listnode_t ** index;
  int           size;
} array_t;

extern array_t * array_create(int initsize);
extern void      array_free(array_t *, visit_t);
extern void      array_clear(array_t *, visit_t);
extern int       array_size(array_t *);
extern int       array_set(array_t *, int, void *);
extern void *    array_get(array_t *, int);
extern void *    array_reduce(array_t *, reduce_t, void *);
extern void      array_visit(array_t *, visit_t);

#endif /* __ARRAY_H__ */
