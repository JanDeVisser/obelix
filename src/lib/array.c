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

#include <core.h>
#include <array.h>

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
  newindex = (listnode_t **) resize_ptrarray(array -> index, newcap, array -> capacity);
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

/**
 * Creates a new <code>array_t</code> from the components of <code>str</code>.
 * The string is split at the occurrences of <code>sep</code>. The individual
 * components are copied and the return array is set up to free these strings
 * when the array is freed.
 * 
 * @param str The string to be split.
 * @param sep The separator string.
 * 
 * @return A new array with the components of the string as elements.
 */
array_t * array_split(char *str, char *sep) {
  char    *ptr;
  char    *sepptr;
  char    *c;
  array_t *ret;
  int      count = 1;
  int      seplen = strlen(sep);

  ptr = str;
  for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
    count++;
    ptr = sepptr + seplen;
  }
  ret = str_array_create(count);
  ptr = str;
  for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
    int len = sepptr - ptr;
    c = (char *) new(len + 1);
    strncpy(c, ptr, len);
    c[len] = 0;
    array_push(ret, c);
    ptr = sepptr + seplen;
  }
  array_push(ret, strdup(ptr));
  return ret;
}

/**
 * Returns a new array that is a subset of an existing array. Array elements
 * are copied as-is; no free functions are being called. The return array
 * has the same hash, cmp, and tostring functions as the existing array, but
 * not the free function. This allows the slice array to be used for processing
 * activities and safely deleted at the end of those without destroying the
 * data held in it.
 *
 * @param array The array to return a subset of.
 * @param from The index to start copying from.
 * @param num The number of elements to copy. If less or equal to zero,
 * the tail of the array from the <i>from</i> index up to index 
 * <code>-num</code>will be copied.
 *
 * @return A newly created array with the same hash, cmp, and tostring
 * functions as <i>array</i>, but with no free function set, and the
 * element pointers copied from <i>array</i> according to the index
 * specification. If <i>from</i> is beyond the end of the array, or if no
 * elements would be copied, <b>NULL</b> is returned.
 */
array_t * array_slice(array_t *array, int from, int num) {
  array_t *ret;
  int      ix;

  if (num <= 0) {
    num = array_size(array) - from + num + 1;
  }
  if ((from + num) > array_size(array)) {
    num = array_size(array) - from;
  }
  ret = array_create(num);
  array_set_cmp(ret, array -> list -> cmp);
  array_set_hash(ret, array -> list -> hash);
  array_set_tostring(ret, array -> list -> tostring);
  for (ix = 0; ix < num; ix++) {
    array_set(ret, ix, array_get(array, from + ix));
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

array_t * array_set_hash(array_t *array, hash_t hash) {
  list_set_hash(array -> list, hash);
  return array;
}

array_t * array_set_tostring(array_t *array, tostring_t tostring) {
  list_set_tostring(array -> list, tostring);
  return array;
}

int array_size(array_t *array) {
  return array -> list -> size;
}

unsigned int array_hash(array_t *array) {
  return list_hash(array -> list);
}

int array_capacity(array_t *array) {
  return array -> capacity;
}

void array_free(array_t *array) {
  if (array) {
    list_free(array -> list);
    free(array -> index);
    free(array);
  }
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
  if (!_array_resize(array, ix + 1)) {
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
  if (ix < 0) {
    ix = array_size(array) + ix;
  }
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

void * array_reduce_chars(array_t *array, reduce_t reducer, void *data) {
  return list_reduce_chars(array -> list, reducer, data);
}

void * array_reduce_str(array_t *array, reduce_t reducer, void *data) {
  return list_reduce_str(array -> list, reducer, data);
}

array_t * array_add_all(array_t *array, array_t *other) {
  return list_reduce(other -> list, (reduce_t) _array_add_all_reducer, array);
}

str_t * array_tostr(array_t *array) {
  return list_tostr(array -> list);
}

void array_debug(array_t *array, char *msg) {
  str_t *str;

  str = array_tostr(array);
  debug(msg, str_chars(str));
  str_free(str);
}
