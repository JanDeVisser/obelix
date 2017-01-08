/*
 * /obelix/src/datastack.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "libcore.h"
#include <datastack.h>
#include <str.h>

typedef struct _bookmark {
  int depth;
} bookmark_t;

typedef struct _counter {
  int count;
} counter_t;

static bookmark_t * _bookmark_create(datastack_t *);
static void         _bookmark_free(bookmark_t *);
static int          _bookmark_depth(bookmark_t *);

static counter_t *  _counter_create();
static void         _counter_free(counter_t *);
static int          _counter_count(counter_t *);
static counter_t *  _counter_incr(counter_t *);

static void         _stack_list_visitor(data_t *);

/* ------------------------------------------------------------------------ */

bookmark_t * _bookmark_create(datastack_t *stack) {
  bookmark_t *ret = NEW(bookmark_t);

  ret -> depth = datastack_depth(stack);
  return ret;
}

void _bookmark_free(bookmark_t *bookmark) {
  if (bookmark) {
    free(bookmark);
  }
}

int _bookmark_depth(bookmark_t *bookmark) {
  return bookmark -> depth;
}

/* ------------------------------------------------------------------------ */

counter_t * _counter_create(void) {
  counter_t *ret = NEW(counter_t);

  ret -> count = 0;
  return ret;
}

void _counter_free(counter_t *counter) {
  if (counter) {
    free(counter);
  }
}

_unused_ int _counter_count(counter_t *counter) {
  return counter -> count;
}

counter_t * _counter_incr(counter_t *counter) {
  counter -> count++;
  return counter;
}

/* ------------------------------------------------------------------------ */

void _stack_list_visitor(data_t *entry) {
  _debug("   . %-40.40s [%-10.10s]", data_tostring(entry), data_typename(entry));
}

/* ------------------------------------------------------------------------ */

datastack_t * datastack_create(char *name) {
  datastack_t *ret;

  ret = NEW(datastack_t);
  ret -> name = strdup(name);
  ret -> debug = 0;
  ret -> list = data_array_create(4);
  ret -> bookmarks = NULL;
  ret -> counters = NULL;
  return ret;
}

datastack_t * datastack_set_debug(datastack_t *stack, int debug) {
  stack -> debug = debug;
  return stack;
}

void datastack_free(datastack_t *stack) {
  if (stack) {
    array_free(stack -> list);
    array_free(stack -> bookmarks);
    array_free(stack -> counters);
    free(stack -> name);
    free(stack);
  }
}

char * datastack_tostring(datastack_t *stack) {
  return stack -> name;
}

int datastack_hash(datastack_t *stack) {
  return strhash(stack -> name);
}

int datastack_cmp(datastack_t *s1, datastack_t *s2) {
  return strcmp(s1 -> name, s2 -> name);
}

int datastack_depth(datastack_t *stack) {
  return array_size(stack -> list);
}

data_t * datastack_pop(datastack_t *stack) {
  data_t *ret;

  ret = (data_t *) array_pop(stack -> list);
  if (stack -> debug) {
    _debug("  - %s", data_tostring(ret));
  }
  return ret;
}

data_t * datastack_peek_deep(datastack_t *stack, int depth) {
  return (data_t *) array_get(stack -> list, -1 - depth);
}

data_t * datastack_peek(datastack_t *stack) {
  return datastack_peek_deep(stack, 0);
}

datastack_t * _datastack_push(datastack_t *stack, data_t *data) {
  array_push(stack -> list, data);
  if (stack -> debug) {
    _debug("After push:");
    datastack_list(stack);
  }
  return stack;
}

datastack_t * datastack_list(datastack_t *stack) {
  _debug("-- Stack '%s' ---------------------------------------------", stack -> name);
  array_visit(stack -> list, (visit_t) _stack_list_visitor);
  _debug("------------------------------------------------------------------");
  return stack;
}

data_t * datastack_find(datastack_t *stack, cmp_t comparator, void *target) {
  int     ix;
  data_t *data;

  for (ix = array_size(stack -> list) - 1; ix >= 0; ix--) {
    data = data_array_get(stack -> list, ix);
    if (!comparator(data, target)) {
      return data;
    }
  }
  return NULL;
}

int datastack_find_type(data_t *data, long type) {
  return (data_hastype(data, (int) type)) ? 0 : 1;
}

datastack_t * datastack_clear(datastack_t *stack) {
  array_clear(stack -> list);
  return stack;
}

/* ------------------------------------------------------------------------ */

datastack_t * datastack_bookmark(datastack_t *stack) {
  if (!stack -> bookmarks) {
    stack -> bookmarks = array_create(4);
    array_set_free(stack -> bookmarks, (free_t) _bookmark_free);
  }
  array_push(stack -> bookmarks, _bookmark_create(stack));
  return stack;
}

array_t * datastack_rollup(datastack_t *stack) {
  bookmark_t *bookmark;
  data_t     *data;
  array_t    *ret;
  int         num;
  int         ix;

  if (!stack -> bookmarks || array_empty(stack -> bookmarks)) {
    return NULL;
  }
  bookmark = (bookmark_t *) array_pop(stack -> bookmarks);
  assert(_bookmark_depth(bookmark) <= datastack_depth(stack));
  num = datastack_depth(stack) - _bookmark_depth(bookmark);
  ret = data_array_create(num);
  for (ix = num - 1; ix >= 0; ix--) {
    data = datastack_pop(stack);
    array_set(ret, ix, data_deserialize(data));
    data_free(data);
  }
  _bookmark_free(bookmark);
  return ret;
}

name_t * datastack_rollup_name(datastack_t *stack) {
  array_t *arr;
  int      ix;
  data_t  *data;
  name_t  *ret;

  arr = datastack_rollup(stack);
  ret = name_create(0);
  for (ix = 0; ix < array_size(arr); ix++) {
    data = data_array_get(arr, ix);
    name_extend_data(ret, data);
  }
  array_free(arr);
  return ret;
}

/* ------------------------------------------------------------------------ */

datastack_t * datastack_new_counter(datastack_t *stack) {
  if (!stack -> counters) {
    stack -> counters = array_create(4);
    array_set_free(stack -> counters, (free_t) _counter_free);
  }
  array_push(stack -> counters, _counter_create());
  return stack;
}

datastack_t * datastack_increment(datastack_t *stack) {
  counter_t *counter;

  if (!stack -> counters || list_empty(stack -> counters)) {
    return NULL;
  }
  counter = (counter_t *) array_get(stack -> counters, -1);
  _counter_incr(counter);
  return stack;
}

int datastack_count(datastack_t *stack) {
  counter_t *counter;
  int        count;

  if (!stack -> counters || array_empty(stack -> counters)) {
    return -1;
  }
  counter = (counter_t *) array_pop(stack -> counters);
  count = counter -> count;
  _counter_free(counter);
  return count;
}

int datastack_current_count(datastack_t *stack) {
  counter_t *counter;
  int        count;

  if (!stack -> counters || array_empty(stack -> counters)) {
    return -1;
  }
  counter = (counter_t *) array_get(stack -> counters, -1);
  count = counter -> count;
  return count;
}
