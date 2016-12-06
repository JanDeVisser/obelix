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
#include <dictionary.h>
#include <exception.h>
#include <nvp.h>
#include <str.h>

typedef struct _dictionaryiter {
  data_t          _d;
  dictionary_t   *dictionary;
  dictiterator_t *di;
} dictionaryiter_t;

static dictionary_t *     _dictionary_new(dictionary_t *, va_list);
extern void               _dictionary_free(dictionary_t *);
extern char *             _dictionary_allocstring(dictionary_t *);
static data_t *           _dictionary_cast(dictionary_t *, int);
static data_t *           _dictionary_resolve(dictionary_t *, char *);
static dictionaryiter_t * _dictionary_iter(dictionary_t *);
static data_t *           _dictionary_create(data_t *, char *, array_t *, dict_t *);
static dictionary_t *     _dictionary_set_all_reducer(data_t *, dictionary_t *);
static dictionary_t *     _dictionary_from_dict_reducer(entry_t *, dictionary_t *);

/* ----------------------------------------------------------------------- */

static vtable_t _vtable_Dictionary[] = {
  { .id = FunctionNew,         .fnc = (void_t) _dictionary_new },
  { .id = FunctionCast,        .fnc = (void_t) _dictionary_cast },
  { .id = FunctionFree,        .fnc = (void_t) _dictionary_free },
  { .id = FunctionAllocString, .fnc = (void_t) _dictionary_allocstring },
  { .id = FunctionResolve,     .fnc = (void_t) _dictionary_resolve },
  { .id = FunctionSet,         .fnc = (void_t) dictionary_set },
  { .id = FunctionLen,         .fnc = (void_t) dictionary_size },
  { .id = FunctionIter,        .fnc = (void_t) _dictionary_iter },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Dictionary[] = {
  { .type = Any,    .name = "dictionary", .method = _dictionary_create, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = NoType, .name = NULL,         .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

static dictionaryiter_t * _dictionaryiter_new(dictionaryiter_t *, dictionary_t *);
static void               _dictionaryiter_free(dictionaryiter_t *);
static data_t *           _dictionaryiter_has_next(dictionaryiter_t *);
static data_t *           _dictionaryiter_next(dictionaryiter_t *);

static vtable_t _vtable_DictionaryIter[] = {
  { .id = FunctionNew,         .fnc = (void_t) _dictionaryiter_new },
  { .id = FunctionFree,        .fnc = (void_t) _dictionaryiter_free },
  { .id = FunctionHasNext,     .fnc = (void_t) _dictionaryiter_has_next },
  { .id = FunctionNext,        .fnc = (void_t) _dictionaryiter_next },
  { .id = FunctionNone,        .fnc = NULL }
};

int Dictionary = -1;
static int DictionaryIter = -1;
extern int data_debug;

/* ----------------------------------------------------------------------- */

void dictionary_init(void) {
  if (Dictionary < 1) {
    typedescr_register_with_methods(Dictionary, dictionary_t);
    typedescr_register(DictionaryIter, dictionaryiter_t);
  }
}

/* ----------------------------------------------------------------------- */

dictionary_t * _dictionary_new(dictionary_t *dictionary, va_list args) {
  data_t *template = va_arg(args, data_t *);

  dictionary -> attributes = strdata_dict_create();
  if (data_is_iterable(template)) {
    data_reduce_with_fnc(template,
      (reduce_t) _dictionary_set_all_reducer, (data_t *) dictionary);
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

data_t * _dictionary_resolve(dictionary_t *dictionary, char *name) {
  return (data_t *) dict_get(dictionary -> attributes, name);
}

dictionaryiter_t * _dictionary_iter(dictionary_t *dict) {
  return (dictionaryiter_t *) data_create(DictionaryIter, dict);
}

/* ----------------------------------------------------------------------- */

data_t * _dictionary_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  dictionary_t *obj;

  (void) self;
  (void) name;
  dictionary_init();
  obj = dictionary_create(NULL);
  dict_reduce(kwargs, (reduce_t) _dictionary_set_all_reducer, obj);
  return (data_t *) obj;
}

/* ----------------------------------------------------------------------- */

dictionary_t * _dictionary_set_all_reducer(data_t *value, dictionary_t *dictionary) {
  nvp_t  *nvp;
  data_t *argv;

  if (data_is_nvp(value)) {
    nvp = (nvp_t *) value;
    dictionary_set(dictionary,
      data_tostring(nvp -> name), data_copy(nvp -> value));
  } else {
    argv = dictionary_get(dictionary, "$");
    if (!argv) {
      argv = data_create_list(NULL);
      dictionary_set(dictionary, "$", argv);
    } else {
      assert(data_is_list(argv));
    }
    data_list_push(argv, data_copy(value));
    data_free(argv);
  }
  return dictionary;
}

dictionary_t * _dictionary_from_dict_reducer(entry_t *entry, dictionary_t *dictionary) {
  dictionary_set(
    dictionary,
    (char *) entry -> key,
    (data_t *) str_copy_chars((char *) entry -> value));
  return dictionary;
}

/* ----------------------------------------------------------------------- */

dictionary_t * dictionary_create(data_t *template) {
  dictionary_init();
  return (dictionary_t *) data_create(Dictionary, template);
}

dictionary_t * dictionary_create_from_dict(dict_t *dict) {
  return dict_reduce_chars(
    dict, (reduce_t) _dictionary_from_dict_reducer, dictionary_create(NULL));
}

data_t * dictionary_get(dictionary_t *dictionary, char *name) {
  return data_copy(_dictionary_resolve(dictionary,name));
}

data_t * dictionary_pop(dictionary_t *dictionary, char *name) {
  return (data_t *) dict_pop(dictionary -> attributes, name);
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

int dictionary_size(dictionary_t *obj) {
  return dict_size(obj -> attributes);
}

/* ----------------------------------------------------------------------- */

dictionaryiter_t * _dictionaryiter_new(dictionaryiter_t *iter, dictionary_t *dict) {
  iter -> dictionary = dictionary_copy(dict);
  iter -> di = di_create(dict -> attributes);
  return iter;
}

void _dictionaryiter_free(dictionaryiter_t *iter) {
  if (iter) {
    di_free(iter -> di);
    dictionary_free(iter -> dictionary);
  }
}

data_t * _dictionaryiter_has_next(dictionaryiter_t *iter) {
  return (data_t *) bool_get(di_has_next(iter -> di));
}

data_t * _dictionaryiter_next(dictionaryiter_t *iter) {
  entry_t *e = di_next(iter -> di);
  data_t  *ret;

  if (e) {
    ret = (data_t *) nvp_create(str_to_data((char *) e -> key), data_copy(e -> value));
  } else {
    ret = (data_t *) data_exception(ErrorExhausted, "Iterator exhausted");
  }
  return ret;
}
