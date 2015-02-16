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

#include "data.h"

static void          _data_init_name(void) __attribute__((constructor));
static data_t *      _data_new_name(data_t *, va_list);
static data_t *      _data_copy_name(data_t *, data_t *);
static int           _data_cmp_name(data_t *, data_t *);
static char *        _data_tostring_name(data_t *);
static unsigned int  _data_hash_name(data_t *);
static data_t *      _data_resolve_name(data_t *, char *);

static name_t *      _name_create(int);
static name_t *      _name_extend(name_t *, array_t *);

vtable_t _vtable_name[] = {
  { .id = MethodNew,      .fnc = (void_t) _data_new_name },
  { .id = MethodCopy,     .fnc = (void_t) _data_copy_name },
  { .id = MethodCmp,      .fnc = (void_t) _data_cmp_name },
  { .id = MethodFree,     .fnc = (void_t) name_free },
  { .id = MethodToString, .fnc = (void_t) _data_tostring_name },
  { .id = MethodHash,     .fnc = (void_t) _data_hash_name },
  { .id = MethodResolve,  .fnc = (void_t) _data_resolve_name },
  { .id = MethodNone,     .fnc = NULL }
};

static typedescr_t typedescr_name = {
  .type       = Name,
  .type_name  = "name",
  .vtable     = _vtable_name
};

/* ----------------------------------------------------------------------- */

void _data_init_name(void) {
  typedescr_register(&typedescr_name);
}

data_t * _data_new_name(data_t *ret, va_list arg) {
  name_t *name;

  name = va_arg(arg, name_t *);
  ret -> ptrval = name;
  return ret;
}

data_t * _data_copy_name(data_t *target, data_t *src) {
  target -> ptrval = name_copy(data_nameval(src));
  return target;
}

int _data_cmp_name(data_t *d1, data_t *d2) {
  name_t *n1;
  name_t *n2;

  n1 = (name_t *) d1 -> ptrval;
  n2 = (name_t *) d2 -> ptrval;
  return name_cmp(n1, n2);
}

char * _data_tostring_name(data_t *d) {
  return name_tostring((name_t *) d -> ptrval);
}

unsigned int _data_hash_name(data_t *data) {
  return name_hash(data_nameval(data));
}

data_t * _data_resolve_name(data_t *data, char *name) {
  data_t *ix_data;
  int     ix;
  name_t *n = data_nameval(data);
  
  /*
   * TODO Isolate error-detecting parsing code from Int parse.
   */
  ix_data = data_parse(Int, name);
  if (ix_data) {
    ix = data_intval(ix_data);
    data_free(ix_data);
    return ((ix < 0) || (ix >= name_size(n)))
      ? data_create(String, name_get(n, ix))
      : NULL;
  } else {
    return NULL;
  }
}

/* ----------------------------------------------------------------------- */

name_t * _name_create(int count) {
  name_t *ret = NEW(name_t);
  
  ret -> name = str_array_create(count);
  ret -> str = NULL;
  return ret;
}

name_t * _name_extend(name_t *name, array_t *components) {
  int ix;
  
  for (ix = 0; ix < array_size(components); ix++) {
    array_push(name -> name, str_array_get(components, ix));
  }
  return name;
}

/* ----------------------------------------------------------------------- */

name_t * name_create(int count, ...) {
  name_t  *ret;
  va_list  components;
  
  va_start(components, count);
  ret = name_vcreate(count, components);
  va_end(components);
  return ret;
}

name_t * name_vcreate(int count, va_list components) {
  name_t *ret;
  int     ix;
  
  ret = _name_create(count);
  for (ix = 0; ix < count; ix++) {
    name_extend(ret, va_arg(components, char *));
  }
  return ret;
}

name_t * name_copy(name_t *src) {
  name_t *ret = _name_create(name_size(src));
  
  return _name_extend(ret, src -> name);
}

void name_free(name_t *name) {
  if (name) {
    array_free(name -> name);
    free(name -> str);
    free(name);
  }
}

name_t * name_extend(name_t *name, char *n) {
  array_push(name -> name, strdup(n));  
  return name;
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

char * name_get(name_t *name, int ix) {
  return str_array_get(name -> name, ix);
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

int name_cmp(name_t *n1, name_t *n2) {
  int ix;
  int cmp;
  
  if (name_size(n1) != name_size(n2)) {
    return name_size(n1) - name_size(n2);
  }
  for (ix = 0; ix < name_size(n1); ix++) {
    cmp = strcmp(name_get(n1, ix), name_get(n2, ix));
    if (cmp) {
      return cmp;
    }
  }
  return 0;
}

unsigned int name_hash(name_t *name) {
  unsigned int ret;
  int          ix;
  
  ret = hashptr(name);
  for (ix = 0; ix < name_size(name); ix++) {
    hashblend(ret, strhash(name_get(name, ix)));
  }
  return ret;
}
