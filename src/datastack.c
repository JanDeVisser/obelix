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

#include <datastack.h>

static void _stack_list_visitor(data_t *);

/*
 * datastack_t - static functions
 */

void _stack_list_visitor(data_t *entry) {
  info("   . %s", data_tostring(entry));
}

/*
 * datastack_t - public functions
 */

datastack_t * datastack_create(char *name) {
  datastack_t *ret;

  ret = NEW(datastack_t);
  ret -> name = strdup(name);
  ret -> debug = 0;
  ret -> list = list_create();
  list_set_free(ret -> list, (free_t) data_free);
  list_set_tostring(ret -> list, (tostring_t) data_tostring);
  list_set_hash(ret -> list, (hash_t) data_hash);
  list_set_cmp(ret -> list, (cmp_t) data_cmp);
  return ret;
}

datastack_t * datastack_set_debug(datastack_t *stack, int debug) {
  stack -> debug = debug;
  return stack;
}

void datastack_free(datastack_t *stack) {
  if (stack) {
    list_free(stack -> list);
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
  return list_size(stack -> list);
}

data_t * datastack_pop(datastack_t *stack) {
  data_t *ret;

  ret = (data_t *) list_pop(stack -> list);
  if (stack -> debug) {
    debug("  - %s", data_tostring(ret));
  }
  return ret;
}

data_t * datastack_peek(datastack_t *stack) {
  return (data_t *) list_tail(stack -> list);
}

datastack_t * datastack_push(datastack_t *stack, data_t *data) {
  list_push(stack -> list, data);
  if (stack -> debug) {
    info("After push:");
    datastack_list(stack);
  }
  return stack;
}

datastack_t * datastack_push_int(datastack_t *stack, long value) {
  return datastack_push(stack, data_create_int(value));
}

datastack_t * datastack_push_string(datastack_t *stack, char *value) {
  return datastack_push(stack, data_create_string(value));
}

datastack_t * datastack_push_float(datastack_t *stack, double value) {
  return datastack_push(stack, data_create_float(value));
}

datastack_t * datastack_list(datastack_t *stack) {
  info("-- Stack '%s' ---------------------------------------------", stack -> name);
  list_visit(stack -> list, (visit_t) _stack_list_visitor);
  info("------------------------------------------------------------------");
  return stack;
}

datastack_t * datastack_clear(datastack_t *stack) {
  list_clear(stack -> list);
  return stack;
}
