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

#include <array.h>
#include <core.h>
#include <str.h>

#define ARRAY_DEFAULT_CAPACITY    8

static int       _array_resize(array_t *, int);
static void *    _array_reduce(array_t *, reduce_t, void *, reduce_type_t);
static array_t * _array_append(void *, array_t *);

// ---------------------------
// array_t static functions

int _array_resize(array_t *array, int minimum_capacity) {
  void **new_contents;
  int    new_capacity;

  minimum_capacity = (minimum_capacity > 0)
    ? minimum_capacity
    : ARRAY_DEFAULT_CAPACITY;
  if (minimum_capacity <= array-> capacity) {
    return TRUE;
  }
  new_capacity = array-> capacity ? array-> capacity : minimum_capacity;

  while(new_capacity < minimum_capacity) {
    new_capacity *= 2;
  }
  new_contents = (void **) resize_ptrarray(array-> contents,
                                           new_capacity,
                                           array -> capacity);
  if (new_contents) {
    array -> contents = new_contents;
    array -> capacity = new_capacity;
  }
  return new_contents != NULL;
}

void * _array_reduce(array_t *array, reduce_t reducer, void *data, reduce_type_t type) {
  free_t      f;
  int         ix;
  void       *elem;
  const void *e;

  f = (type == RTStrs) ? (free_t) data_free : NULL;
  for (ix = 0; ix < array_size(array); ix++) {
    e = array -> contents[ix];
    switch (type) {
      case RTChars:
        assert(array->type.tostring);
        elem =  array -> type.tostring(e);
        break;
      case RTStrs:
        assert(array->type.tostring);
        elem =  str_wrap(array -> type.tostring(e));
        break;
      case RTObjects:
        elem = (void *) e;
        f = NULL;
        break;
      default:
        assert(0);
    }
    data = reducer(elem, data);
    if (f) {
      f(elem);
    }
  }
  return data;
}

array_t * _array_append(void *data, array_t *array) {
  if (array->type.copy) {
    data = array->type.copy(data);
  }
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
    a -> str = NULL;
    if (_array_resize(a, capacity)) {
      ret = a;
    }
    if (!ret) {
      free(a);
    }
  }
  return ret;
}

array_t * str_array_create(int sz) {
  return array_set_type(array_create(sz), coretype(CTString));
}

array_t * array_copy(const array_t *src) {
  array_t *copy = NULL;

  if (!src) {
    return NULL;
  }
  copy = array_create(array_capacity(src));
  if (!copy) {
    return NULL;
  }
  array_add_all(copy, src);
  return copy;
}

/**
 * Creates a new <code>array_t</code> from the components of <code>str</code>.
 * The string is split at the occurrences of <code>sep</code>. The individual
 * components are copied and the return array is set up to free these strings
 * when the array is freed.
 *
 * @param string The string to be split.
 * @param sep The separator string.
 *
 * @return A new array with the components of the string as elements.
 */
array_t * array_split(const char *string, const char *separator) {
  const char    *ptr;
  char          *sepptr;
  char          *c;
  array_t       *ret;
  int            count = 1;
  unsigned long  seplen = strlen(separator);
  long           len;

  if (!string || !string[0]) {
    return str_array_create(0);
  }
  ptr = string;
  for (sepptr = strstr(ptr, separator); sepptr; sepptr = strstr(ptr, separator)) {
    count++;
    ptr = sepptr + seplen;
  }
  ret = str_array_create(count);
  ptr = string;
  for (sepptr = strstr(ptr, separator); sepptr; sepptr = strstr(ptr, separator)) {
    len = sepptr - ptr;
    c = stralloc(len);
    if (len) {
      strncpy(c, ptr, len);
    }
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
array_t * array_slice(const array_t *array, int from, int num) {
  array_t *ret;
  int      ix;

  if (num <= 0) {
    num = array_size(array) - from + num + 1;
  }
  if ((from + num) > array_size(array)) {
    num = array_size(array) - from;
  }
  ret = array_create(num);
  array_set_type(ret, &(array -> type));
  array_set_free(ret, NULL);
  for (ix = 0; ix < num; ix++) {
    array_set(ret, ix, array_get(array, from + ix));
  }
  return ret;
}

array_t* array_set_type(array_t *array, const type_t *type) {
  type_copy(&(array -> type), type);
  return array;
}


array_t * array_set_free(array_t *array, free_t freefnc) {
  array -> type.free = freefnc;
  return array;
}

array_t * array_set_cmp(array_t *array, cmp_t cmp) {
  array -> type.cmp = cmp;
  return array;
}

array_t * array_set_hash(array_t *array, hash_t hash) {
  array -> type.hash = hash;
  return array;
}

array_t * array_set_tostring(array_t *array, tostring_t tostring) {
  array -> type.tostring = tostring;
  return array;
}

int array_size(const array_t * a) {
  return (a) ? a->size : -1;
}

int array_empty(const array_t * a) {
  return array_size(a) == 0;
}

int array_not_empty(const array_t * a) {
  return array_size(a) > 0;
}

unsigned int array_hash(const array_t *array) {
  unsigned int hash;
  reduce_ctx *ctx;

  if (!array -> type.hash) {
    hash = hashptr(array);
  } else {
    ctx = NEW(reduce_ctx);
    ctx -> fnc = (void_t) array -> type.hash;
    ctx -> longdata = 0;
    array_reduce((array_t *) array, (reduce_t) collection_hash_reducer, ctx);
    hash = (unsigned int) ctx -> longdata;
    free(ctx);
  }
  return hash;
}

int array_capacity(const array_t *array) {
  return array -> capacity;
}

void array_free(array_t *array) {
  if (array) {
    array_clear(array);
    free(array -> contents);
    free(array -> str);
    free(array);
  }
}

array_t * array_clear(array_t *array) {
  int ix;

  if (array -> type.free) {
    for (ix = 0; ix < array_size(array); ix++) {
      if (array -> contents[ix]) {
        array -> type.free((void *) array -> contents[ix]);
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
  if (array -> type.free && array -> contents[ix]) {
    array -> type.free((void *) array -> contents[ix]);
  }
  if (array->type.copy) {
    data = array->type.copy(data);
  }
  array -> contents[ix] = data;
  if (ix >= array_size(array)) {
    array -> size = ix + 1;
  }
  return TRUE;
}

int array_set_int(array_t * array, int ix, intptr_t value) {
  return array_set(array, ix, (void *) value);
}

array_t * array_add_all(array_t *array, const array_t *other) {
  array_reduce((array_t *) other, (reduce_t) _array_append, array);
  return array;
}

void * array_get(const array_t *array, int ix) {
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

intptr_t array_get_int(const array_t * a, int i) {
  return (intptr_t) array_get(a, i);
}

char * str_array_get(const array_t * a, int ix) {
  return (char *) array_get(a, ix);
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

int array_push(array_t * array, void *data) {
  return array_set(array, -1, data);
}

void * array_remove(array_t *array, int ix) {
  void *ret;

  if ((ix < 0) || (ix >= array_size(array))) {
    errno = EFAULT;
    return NULL;
  }
  if (ix == (array -> size - 1)) {
    return array_pop(array);
  } else {
    ret = array->contents[ix];
    memmove(array->contents + ix,
        array->contents + (ix + 1),
        (array->capacity - (ix + 1)) * sizeof(void *));
    array->size--;
  }
  return ret;
}

void * array_reduce(array_t *array, reduce_t reduce, void *data) {
  return _array_reduce(array, (reduce_t) reduce, data, RTObjects);
}

void * array_reduce_chars(array_t *array, reduce_t reduce, void *data) {
  assert(array -> type.tostring);
  return _array_reduce(array, (reduce_t) reduce, data, RTChars);
}

void * array_reduce_str(array_t *array, reduce_t reduce, void *data) {
  assert(array -> type.tostring);
  return _array_reduce(array, (reduce_t) reduce, data, RTStrs);
}

array_t *array_visit(array_t *array, visit_t visitor) {
  array_reduce(array, (reduce_t) collection_visitor, visitor);
  return array;
}

str_t * array_to_str(const array_t *array) {
  str_t *ret;
  str_t *s;

  if (array) {
    if (!array -> size) {
      ret = str_wrap("[]");
    } else {
      ret = str("[ ");
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
    str = array_to_str(array);
    free(array -> str);
    array -> str = strdup(str_chars(str));
    str_free(str);
    return array -> str;
  } else {
    return NULL;
  }
}

void array_debug(array_t *array, const char *msg) {
  mdebug(core, msg, array_tostring(array));
}

void * array_find(const array_t *array, cmp_t filter, void *what) {
  int   ix;

  for (ix = 0; ix < array -> size; ix++) {
    if (!filter(array -> contents[ix], what)) {
      return array -> contents[ix];
    }
  }
  return NULL;
}

str_t * array_join(const array_t *array, const char *glue) {
  return _str_join(glue, array, (obj_reduce_t) array_reduce_chars);
}

/* ------------------------------------------------------------------------ */


array_t * array_start(array_t *array) {
  array -> cur_ix = (array_size(array)) ? 0 : -1;
  return array;
}

array_t * array_end(array_t *array) {
  array -> cur_ix = array_size(array) - 1;
  return array;
}

void * array_current(const array_t *array) {
  return (array -> cur_ix >= 0) ? array-> contents[array-> cur_ix] : 0;
}

int array_has_next(const array_t *array) {
  return (array -> cur_ix >= 0) && (array-> cur_ix < array_size(array));
}

int array_has_prev(const array_t *array) {
  return (array -> cur_ix > 0) && (array-> cur_ix <= array_size(array) - 1);
}

void * array_next(array_t *array) {
  return array -> contents[array -> cur_ix++];
}

void * array_prev(array_t *array) {
  return array -> contents[array -> cur_ix--];
}
