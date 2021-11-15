/*
 * tree.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __TREE_H__
#define __TREE_H__

#include <list.h>

typedef struct _tree {
  struct _tree *up;
  list_t       *down;
  void         *data;
  free_t        free_data;
} tree_t;

extern tree_t *         tree_create(void *);
extern void             tree_free(tree_t *);
extern tree_t *         tree_up(tree_t *);
extern void *           tree_get(tree_t *);
extern void             tree_set(tree_t *, void *);
extern listiterator_t * tree_down(tree_t *);
extern tree_t *         tree_append(tree_t *, void *);
extern tree_t *         tree_visit(tree_t *, visit_t);
extern void *           tree_reduce(tree_t *, reduce_t, void *);

#endif /* __TREE_H__ */
