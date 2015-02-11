/*
 * /obelix/src/types/name.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <name.h>
#include <str.h>

static name_t * _name_extend(name_t *, array_t *);

name_t * _name_extend(name_t *name, array_t *components) {
  int ix;
  
  for (ix = 0; ix < array_size(components); ix++) {
    array_push(name -> name, str_array_get(components, ix));
  }
  return name;
}

name_t * name_create(int count, ...) {
  va_list components;
  
  va_start(components, count);
  return name_vcreate(count, components);
}

name_t * name_vcreate(int count, va_list components) {
  name_t *ret = NEW(name_t);
  int     ix;
  char   *c;
  
  ret -> name = str_array_create(count);
  for (ix = 0; ix < count; ix++) {
    c = va_arg(components, char *);
    array_push(ret -> name, strdup(c));
  }
  return ret;
}

name_t * name_copy(name_t *src) {
  name_t *ret = NEW(name_t);
  
  ret -> name = str_array_create(name_size(src));
  return _name_extend(ret, src -> name);
}

void name_free(name_t *name) {
  if (name) {
    array_free(name -> name);
    free(name -> str);
    free(name);
  }
}

int name_size(name_t *name) {
  return array_size(name -> name);
}

char * name_first(name_t *name) {
  return (name_size(name) > 0) ? str_array_get(name -> name, 0) : NULL;
}

char * name_last(name_t *name) {
  return (name_size(name) > 0) ? str_array_get(name -> name, -1) : NULL;
}

name_t * name_tail(name_t *name) {
  name_t  *ret = NEW(name_t);
  array_t *tail = array_slice(name -> name, 1, -1);
  
  ret -> name = str_array_create(name_size(name) - 1);
  _name_extend(ret, tail);
  array_free(tail);
  return ret;
}

name_t * name_head(name_t *name) {
  name_t  *ret = NEW(name_t);
  array_t *head = array_slice(name -> name, 0, -2);

  ret -> name = str_array_create(name_size(name) - 1);
  _name_extend(ret, head);
  array_free(head);
  return ret;
}

char * name_tostring(name_t *name) {
  str_t *s;
  
  free(name -> str);
  s = array_join(name -> name, ".");
  name -> str = strdup(str_chars(s));
  str_free(s);
  return name -> str;
}
