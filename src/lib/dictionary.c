/*
 * /obelix/src/dictionary.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <exception.h>
#include <logging.h>
#include <dictionary.h>

static inline void      _dictionary_init(void);
static dictionary_t *   _dictionary_new(dictionary_t *, va_list);
extern void             _dictionary_free(dictionary_t *);
extern char *           _dictionary_allocstring(dictionary_t *);
static data_t *         _dictionary_cast(dictionary_t *, int);
static int              _dictionary_len(dictionary_t *);
static data_t *         _dictionary_create(data_t *, char *, array_t *, dict_t *);
static dictionary_t *   _dictionary_set_all_reducer(entry_t *, dictionary_t *);

/* ----------------------------------------------------------------------- */

static vtable_t _vtable_dictionary[] = {
  { .id = FunctionNew,         .fnc = (void_t) _dictionary_new },
  { .id = FunctionCast,        .fnc = (void_t) _dictionary_cast },
  { .id = FunctionFree,        .fnc = (void_t) _dictionary_free },
  { .id = FunctionAllocString, .fnc = (void_t) _dictionary_allocstring },
  { .id = FunctionResolve,     .fnc = (void_t) dictionary_resolve },
  { .id = FunctionSet,         .fnc = (void_t) dictionary_set },
  { .id = FunctionLen,         .fnc = (void_t) _dictionary_len },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_dictionary[] = {
  { .type = Any,    .name = "dictionary", .method = _dictionary_create, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = NoType, .name = NULL,         .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int Dictionary = -1;
extern int data_debug;

/* ----------------------------------------------------------------------- */

void _dictionary_init(void) {
  if (Dictionary < 0) {
    Dictionary = typedescr_create_and_register(
      Dictionary, "dictionary", _vtable_dictionary, _methoddescr_dictionary);
    typedescr_set_size(Dictionary, dictionary_t);
  }
}

/* ----------------------------------------------------------------------- */

dictionary_t * _dictionary_new(dictionary_t *dictionary, va_list args) {
  data_t *template = va_arg(args, data_t *);

  dictionary -> attributes = strdata_dict_create();
  if (data_is_dictionary(template)) {
    dict_reduce(
      data_as_dictionary(template) -> attributes,
      (reduce_t) _dictionary_set_all_reducer,
      dictionary);
  }
  return dictionary;
}

void _dictionary_free(dictionary_t *dictionary) {
  if (dictionary) {
    dict_free(dictionary -> attributes);
  }
}

char * _dictionary_allocstring(dictionary_t *dictionary) {
  return dict_tostring(dictionary -> attributes);
}

data_t * _dictionary_cast(dictionary_t *obj, int totype) {
  data_t   *ret = NULL;

  switch (totype) {
    case Bool:
      ret = int_as_bool(obj && dict_size(obj -> attributes));
      break;
  }
  return ret;
}

int _dictionary_len(dictionary_t *obj) {
  return dict_size(obj -> attributes);
}

/* ----------------------------------------------------------------------- */

data_t * _dictionary_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  dictionary_t *obj;

  (void) self;
  (void) name;
  _dictionary_init();
  obj = dictionary_create(NULL);
  dict_reduce(kwargs, (reduce_t) _dictionary_set_all_reducer, obj);
  return (data_t *) obj;
}

/* ----------------------------------------------------------------------- */

dictionary_t * _dictionary_set_all_reducer(entry_t *e, dictionary_t *dictionary) {
  dictionary_set(dictionary, (char *) e -> key, (data_t *) e -> value);
  return dictionary;
}

/* ----------------------------------------------------------------------- */

dictionary_t * dictionary_create(data_t *template) {
  _dictionary_init();
  return (dictionary_t *) data_create(Dictionary, template);
}

data_t * dictionary_get(dictionary_t *dictionary, char *name) {
  return data_copy((data_t *) dict_get(dictionary -> attributes, name));
}

data_t * dictionary_set(dictionary_t *dictionary, char *name, data_t *value) {
  dict_put(dictionary -> attributes, strdup(name), data_copy(value));
  return value;
}

int dictionary_has(dictionary_t *dictionary, char *name) {
  int ret = dict_has_key(dictionary -> attributes, name);
  debug(data, "   dictionary_has('%s'): %d", name, ret);
  return ret;
}

data_t * dictionary_resolve(dictionary_t *dictionary, char *name) {
  return (data_t *) dict_get(dictionary -> attributes, name);
}
