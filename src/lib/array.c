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

#include <array.h>
#include <core.h>
#include <str.h>

#define ARRAY_DEF_CAPACITY    8

static int       _array_resize(array_t *, int);
static void *    _array_reduce(array_t *, reduce_t, void *, reduce_type_t);
static array_t * _array_append(array_t *, void *);

// ---------------------------
// array_t static functions

int _array_resize(array_t *array, int mincap) {
  void **newcontents;
  int    newcap;

  mincap = (mincap > 0) ? mincap : ARRAY_DEF_CAPACITY;
  if (mincap <= array -> capacity) {
    return TRUE;
  }
  newcap = array -> capacity ? array -> capacity : mincap;

  while(newcap < mincap) {
    newcap *= 2;
  }
  newcontents = (void **) resize_ptrarray(array -> contents,
                                          newcap,
                                          array -> capacity);
  if (newcontents) {
    array -> contents= newcontents;
    array -> capacity = newcap;
  }
  return newcontents != NULL;
}

void * _array_reduce(array_t *array, reduce_t reducer, void *data, reduce_type_t type) {
  free_t      f;
  int         ix;
  void       *elem;

  f = (type == RTStrs) ? (free_t) data_free : NULL;
  for (ix = 0; ix < array_size(array); ix++) {
    elem = array -> contents[ix];
    switch (type) {
      case RTChars:
        elem =  array -> tostring(elem);
        break;
      case RTStrs:
        elem =  str_wrap(array -> tostring(elem));
        break;
    }
    data = reducer(elem, data);
    if (f) {
      f(elem);
    }
  }
  return data;
}

/*
 * Since array_push is a macro, we need a function we can feed into array_reduce
 * for array_add_all.
 */
array_t * _array_append(array_t *array, void *data) {
  array_push(array, data);
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
    a -> contents = NULL;
    a -> refs = 1;
    a -> str = NULL;
    array_set_cmp(a, NULL);
    array_set_hash(a, NULL);
    array_set_tostring(a, NULL);
    array_set_free(a, NULL);
    if (_array_resize(a, capacity)) {
      ret = a;
    }
    if (!ret) {
      free(a);
    }
  }
  return ret;
}

array_t * array_copy(array_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
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
  int      len;

  if (!str || !str[0]) {
    return str_array_create(0);
  }
  ptr = str;
  for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
    count++;
    ptr = sepptr + seplen;
  }
  ret = str_array_create(count);
  ptr = str;
  for (sepptr = strstr(ptr, sep); sepptr; sepptr = strstr(ptr, sep)) {
    len = sepptr - ptr;
    c = stralloc(len);
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
  array_set_cmp(ret, array -> cmp);
  array_set_hash(ret, array -> hash);
  array_set_tostring(ret, array -> tostring);
  for (ix = 0; ix < num; ix++) {
    array_set(ret, ix, array_get(array, from + ix));
  }
  return ret;
}

array_t * array_set_free(array_t *array, free_t freefnc) {
  array -> freefnc = freefnc;
  return array;
}

array_t * array_set_cmp(array_t *array, cmp_t cmp) {
  array -> cmp = cmp;
  return array;
}

array_t * array_set_hash(array_t *array, hash_t hash) {
  array -> hash = hash;
  return array;
}

array_t * array_set_tostring(array_t *array, tostring_t tostring) {
  array -> tostring = tostring;
  return array;
}

unsigned int array_hash(array_t *array) {
  unsigned int hash;
  reduce_ctx *ctx;

  if (!array -> hash) {
    hash = hashptr(array);
  } else {
    ctx = NEW(reduce_ctx);
    ctx -> fnc = (void_t) array -> hash;
    ctx -> longdata = 0;
    array_reduce(array, (reduce_t) collection_hash_reducer, ctx);
    hash = (unsigned int) ctx -> longdata;
    free(ctx);
  }
  return hash;
}

int array_capacity(array_t *array) {
  return array -> capacity;
}

void array_free(array_t *array) {
  if (array) {
    array -> refs--;
    if (array -> refs <= 0) {
      array_clear(array);
      free(array -> contents);
      free(array -> str);
      free(array);
    }
  }
}

array_t * array_clear(array_t *array) {
  int ix;

  if (array -> freefnc) {
    for (ix = 0; ix < array_size(array); ix++) {
      if (array -> contents[ix]) {
        array -> freefnc(array -> contents[ix]);
      }
    }
  }
  memset(array -> contents, 0, array -> capacity * sizeof(void *));
  array -> size = 0;
  return array;
}

int array_set(array_t *array, int ix, void *data) {
  if (ix < 0) {
    ix = array -> size;
  }
  if (!_array_resize(array, ix + 1)) {
    return FALSE;
  }
  if (array -> freefnc && array -> contents[ix]) {
    array -> freefnc(array -> contents[ix]);
  }
  array -> contents[ix] = data;
  if (ix >= array_size(array)) {
    array -> size = ix + 1;
  }
  return TRUE;
}

void * array_get(array_t *array, int ix) {
  errno = 0;
  if (ix < 0) {
    ix = array_size(array) + ix;
  }
  if ((ix < 0) || (ix >= array_size(array))) {
    errno = EFAULT;
    return NULL;
  }
  return array -> contents[ix];
}

void * array_pop(array_t *array) {
  void *ret = NULL;

  if (array -> size) {
    array -> size--;
    ret = array -> contents[array -> size];
    array -> contents[array -> size] = NULL;
  }
  return ret;
}

void * array_reduce(array_t *array, reduce_t reduce, void *data) {
  return _array_reduce(array, (reduce_t) reduce, data, RTObjects);
}

void * array_reduce_chars(array_t *array, reduce_t reduce, void *data) {
  assert(array -> tostring);
  return _array_reduce(array, (reduce_t) reduce, data, RTChars);
}

void * array_reduce_str(array_t *array, reduce_t reduce, void *data) {
  assert(array -> tostring);
  return _array_reduce(array, (reduce_t) reduce, data, RTStrs);
}

array_t *array_visit(array_t *array, visit_t visitor) {
  array_reduce(array, (reduce_t) collection_visitor, visitor);
  return array;
}

array_t * array_add_all(array_t *array, array_t *other) {
  reduce_ctx *ctx = NEW(reduce_ctx);

  ctx -> fnc = (void_t) _array_append;
  ctx -> obj = array;
  ctx = array_reduce(other, (reduce_t) collection_add_all_reducer, ctx);
  free(ctx);
  return array;
}

str_t * array_tostr(array_t *array) {
  str_t *ret;
  str_t *s;

  if (array) {
    if (!array -> size) {
      ret = str_wrap("[]");
    } else {
      ret = str_copy_chars("[ ");
      s = array_join(array, ", ");
      str_append(ret, s);
      str_free(s);
      str_append_chars(ret, " ]");
    }
  } else {
    ret = str_wrap("Null");
  }
  return ret;
}

char * array_tostring(array_t *array) {
  str_t *str;

  if (array) {
    str = array_tostr(array);
    array -> str = strdup(str_chars(str));
    str_free(str);
    return array -> str;
  } else {
    return NULL;
  }
}

void array_debug(array_t *array, char *msg) {
  debug(msg, array_tostring(array));
}


array_t * array_start(array_t *array) {
  array -> curix = (array_size(array)) ? 0 : -1;
  return array;
}

array_t * array_end(array_t *array) {
  array -> curix = array_size(array) - 1;
  return array;
}

void * array_current(array_t *array) {
  return (array -> curix >= 0) ? array -> contents[array -> curix] : 0;
}

int array_has_next(array_t *array) {
  return (array -> curix >= 0) && (array -> curix < array_size(array));
}

int array_has_prev(array_t *array) {
  return (array -> curix > 0) && (array -> curix <= array_size(array) - 1);
}

void * array_next(array_t *array) {
  return array -> contents[array -> curix++];
}

void * array_prev(array_t *array) {
  return array -> contents[array -> curix--];
}

