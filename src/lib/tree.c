/*
 * tree.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <stdlib.h>

#include <tree.h>

static reduce_ctx * _tree_visitor(tree_t *, reduce_ctx *);
static reduce_ctx * _tree_reducer(tree_t *, reduce_ctx *);

// --------------------
// tree_t static methods

void _tree_free(tree_t *tree) {
  if (tree) {
    list_free(tree -> down);
    if (tree -> free_data && tree -> data) {
      tree -> free_data(tree -> data);
    }
    free(tree);
  }
}

reduce_ctx * _tree_visitor(tree_t *tree, reduce_ctx *ctx) {
  tree_visit(tree, ((visit_t) ctx -> fnc));
  return ctx;
}

reduce_ctx * _tree_reducer(tree_t *tree, reduce_ctx *ctx) {
  ctx -> data = tree_reduce(tree, ((reduce_t) ctx -> fnc), ctx -> data);
  return ctx;
}

// -------------------
// tree_t

tree_t * tree_create(void *data) {
  tree_t *ret;

  ret = NEW(tree_t);
  if (ret) {
    ret -> down = list_create();
    if (ret -> down) {
      ret -> up = NULL;
      ret -> data = data;
    } else {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void tree_free(tree_t *tree) {
  tree_visit(tree, (free_t) _tree_free);
}

tree_t * tree_up(tree_t *tree) {
  return tree -> up;
}

void * tree_get(tree_t *tree) {
  return tree -> data;
}

void tree_set(tree_t *tree, void *data) {
  tree -> data = data;
}

listiterator_t * tree_down(tree_t *tree) {
  return li_create(tree -> down);
}

tree_t * tree_append(tree_t *tree, void *data) {
  tree_t *node;

  node = tree_create(data);
  if (node) {
    node -> up = tree;
    list_append(tree -> down, node);
  }
  return node;
}

tree_t * tree_visit(tree_t *tree, visit_t visitor) {
  reduce_ctx *ctx;

  ctx = reduce_ctx_create(NULL, NULL, (void_t) visitor);
  if (ctx) {
    list_reduce(tree -> down, (reduce_t) _tree_visitor, ctx);
    free(ctx);
    visitor(tree -> data);
    return tree;
  }
  return NULL;
}

void * tree_reduce(tree_t *tree, reduce_t reducer, void *data) {
  reduce_ctx *ctx;
  void       *ret;

  ctx = reduce_ctx_create(NULL, data, (void_t) reducer);
  if (ctx) {
    ctx -> data = list_reduce(tree -> down, (reduce_t) _tree_reducer, ctx);
    ret = reducer(tree -> data, ctx -> data);
    free(ctx);
    return ret;
  }
  return NULL;
}

