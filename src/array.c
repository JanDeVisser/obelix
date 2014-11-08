/*
 * array.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "core.h"
#include "array.h"

#define ARRAY_DEF_CAPACITY    8

static int       _array_resize(array_t *, int);
static array_t * _array_add_all_reducer(void *, array_t *);

// ---------------------------
// array_t static functions

int _array_resize(array_t *array, int mincap) {
  listnode_t **newindex;
  int newcap;
  int i;

  mincap = (mincap > 0) ? mincap : ARRAY_DEF_CAPACITY;
  if (mincap <= array -> capacity) {
    return TRUE;
  }
  newcap = array -> capacity ? array -> capacity : mincap;
  
  while(newcap < mincap) {
    newcap *= 2;
  }
  newindex = (listnode_t **) resize_ptrarray(array -> index, newcap);
  if (newindex) {
    array -> index = newindex;
    array -> capacity = newcap;
  }
  return newindex != NULL;
}

array_t * _array_add_all_reducer(void *data, array_t *array) {
  array_set(array, -1, data);
  return array;
}

// ---------------------------
// array_t

array_t * array_create(int capacity) {
  array_t *ret = NULL;
  array_t *a;

  a = NEW(array_t);
  if (a) {
    a -> capacity = 0;
    a -> index = NULL;
    a -> list = list_create();
    if (a -> list) {
      if (_array_resize(a, capacity)) {
        ret = a;
      } else {
        list_free(a -> list);
      }
    }
    if (!ret) {
      free(a);
    }
  }
  return ret;
}

array_t * array_set_free(array_t *array, visit_t freefnc) {
  list_set_free(array -> list, freefnc);
  return array;
}

array_t * array_set_cmp(array_t *array, cmp_t cmp) {
  list_set_cmp(array -> list, cmp);
  return array;
}

int array_size(array_t *array) {
  return array -> list -> size;
}

int array_capacity(array_t *array) {
  return array -> capacity;
}

void array_free(array_t *array) {
  list_free(array -> list);
  free(array -> index);  
  free(array);
}

array_t * array_clear(array_t *array) {
  list_clear(array -> list);
  memset(array -> index, 0, array -> capacity * sizeof(listnode_t *));
  return array;
}

int array_set(array_t *array, int ix, void *data) {
  int i;

  if (ix < 0) {
    ix = list_size(array -> list);
  }
  if (!_array_resize(array, ix)) {
    return FALSE;
  }
  for (i = list_size(array -> list); i <= ix; i++) {
    if (list_append(array -> list, NULL)) {
      array -> index[i] = array -> list -> tail -> prev;
    } else {
      return FALSE;
    }
  }
  array -> index[ix] -> data = data;
  return TRUE;
}

void * array_get(array_t *array, int ix) {
  errno = 0;
  if ((ix < 0) || (ix >= list_size(array -> list))) {
    errno = EFAULT;
    return NULL;
  }
  return array -> index[ix] -> data;
}

array_t * array_visit(array_t *array, visit_t visitor) {
  list_visit(array -> list, visitor);
  return array;
}

void * array_reduce(array_t *array, reduce_t reducer, void *data) {
  return list_reduce(array -> list, reducer, data);
}

array_t * array_add_all(array_t *array, array_t *other) {
  return list_reduce(other -> list, (reduce_t) _array_add_all_reducer, array);
}
