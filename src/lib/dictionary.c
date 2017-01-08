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
static void               _dictionary_free(dictionary_t *);
static char *             _dictionary_tostring(dictionary_t *);
static data_t *           _dictionary_cast(dictionary_t *, int);
static data_t *           _dictionary_resolve(dictionary_t *, char *);
static dictionaryiter_t * _dictionary_iter(dictionary_t *);
static char *             _dictionary_encode(dictionary_t *);
static dictionary_t *     _dictionary_serialize(dictionary_t *);
static data_t *           _dictionary_deserialize(dictionary_t *);

static data_t *           _dictionary_create(data_t *, char *, array_t *, dict_t *);

static dictionary_t *     _dictionary_serializer(entry_t *, dictionary_t *);
static dictionary_t *     _dictionary_set_all_reducer(data_t *, dictionary_t *);
static dictionary_t *     _dictionary_from_dict_reducer(entry_t *, dictionary_t *);

/* ----------------------------------------------------------------------- */

static vtable_t _vtable_Dictionary[] = {
  { .id = FunctionNew,         .fnc = (void_t) _dictionary_new },
  { .id = FunctionCast,        .fnc = (void_t) _dictionary_cast },
  { .id = FunctionFree,        .fnc = (void_t) _dictionary_free },
  { .id = FunctionToString,    .fnc = (void_t) _dictionary_tostring },
  { .id = FunctionResolve,     .fnc = (void_t) _dictionary_resolve },
  { .id = FunctionSet,         .fnc = (void_t) dictionary_set },
  { .id = FunctionLen,         .fnc = (void_t) dictionary_size },
  { .id = FunctionIter,        .fnc = (void_t) _dictionary_iter },
  { .id = FunctionEncode,      .fnc = (void_t) _dictionary_encode },
  { .id = FunctionSerialize,   .fnc = (void_t) _dictionary_serialize },
  { .id = FunctionDeserialize, .fnc = (void_t) _dictionary_deserialize },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Dictionary[] = {
  { .type = Any,    .name = "dictionary", .method = _dictionary_create, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = NoType, .name = NULL,         .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

static dictionaryiter_t * _dictionaryiter_new(dictionaryiter_t *, va_list);
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

char * _dictionary_tostring(dictionary_t *dictionary) {
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

list_t * _dictionary_serialize_and_encode_entry(entry_t *e, list_t *encoded) {
  intptr_t  bufsz;
  data_t   *key;
  data_t   *value;
  nvp_t    *nvp;

  bufsz = (intptr_t) ((pointer_t *) list_head(encoded)) -> ptr;
  key = (data_t *) str_wrap((char *) e -> key);
  value = data_copy((data_t *) e -> value);
  nvp = nvp_create(
      data_serialize(key),
      (data_t *) str_adopt(data_encode(value)));
  list_append(encoded, nvp);
  if (bufsz > 4) {
    bufsz += 2; /* for ', ' */
  }
  bufsz += str_len((str_t *) nvp -> name) +
      str_len((str_t *) nvp -> value) + 2; /* for ': ' */
  ((pointer_t *) list_head(encoded)) -> ptr = (void *) bufsz;
  data_free(key);
  data_free(value);
  return encoded;
}

char * _dictionary_encode(dictionary_t *dict) {
  list_t   *encoded = data_list_create();
  intptr_t  bufsz;
  nvp_t    *nvp;
  char     *ret;
  char     *ptr;
  int       ix;

  /*
   * The size of the the string buffer to allocate will be kept in a pointer_t
   * which is the first entry in the list. The reducer will read and update it,
   * and the end result will be shifted off the list when encoding is done.
   *
   * The buffer is initialized with 4 = strlen("{  }")
   */
  list_append(encoded, ptr_create(0, (void *)(intptr_t) 4));
  dict_reduce(dict -> attributes,
      (reduce_t) _dictionary_serialize_and_encode_entry,
      encoded);

  bufsz = (intptr_t) ((pointer_t *) list_shift(encoded)) -> ptr;
  if (list_empty(encoded)) {
    ret = strdup("{}");
  } else {
    ret = ptr = stralloc(bufsz);
    strcpy(ptr, "{ ");
    ptr += 2;
    ix = 0;
    while (list_notempty(encoded)) {
      if (ix++ > 0) {
        strcpy(ptr, ", ");
        ptr += 2;
      }
      nvp = (nvp_t *) list_shift(encoded);
      strcpy(ptr, str_chars((str_t *) nvp -> name));
      ptr += str_len((str_t *) nvp -> name);
      strcpy(ptr, ": ");
      ptr += 2;
      strcpy(ptr, str_chars((str_t *) nvp -> value));
      ptr += str_len((str_t *) nvp -> value);
      nvp_free(nvp);
    }
    strcpy(ptr, " }");
  }
  list_free(encoded);
  return ret;
}

data_t * _dictionary_deserialize(dictionary_t *dict) {
  data_t      *typename;
  typedescr_t *type;
  data_t      *ret = NULL;
  data_t      *value;
  void_t       deserialize;

  if ((typename = dictionary_get(dict, "__obl_type__"))) {
    type = typedescr_get_byname(data_tostring(typename));
    if (type) {
      deserialize = typedescr_get_function(type, FunctionDeserialize);
      if (deserialize) {
        ret = ((data_t * (*)(data_t *)) deserialize)((data_t *) dict);
      } else if (dictionary_has(dict, "value")) {
        value = dictionary_get(dict, "value");
        ret = data_parse(typetype(type), data_tostring(value));
        data_free(value);
      }
    }
  }
  if (!ret) {
    ret = data_copy((data_t *) dict);
  }
  return ret;
}


dictionary_t * _dictionary_serializer(entry_t *entry, dictionary_t *dictionary) {
  dictionary_set(dictionary,
      (char *) entry -> key, data_serialize(data_as_data(entry -> value)));
  return dictionary;
}

dictionary_t * _dictionary_serialize(dictionary_t *dictionary) {
  return dict_reduce(
      dictionary -> attributes, (reduce_t) _dictionary_serializer,
      dictionary_create(NULL));
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
  nvp_t      *nvp;
  datalist_t *argv;

  if (data_is_nvp(value)) {
    nvp = (nvp_t *) value;
    dictionary_set(dictionary,
      data_tostring(nvp -> name), data_copy(nvp -> value));
  } else {
    argv = data_as_list(dictionary_get(dictionary, "$"));
    if (!argv) {
      argv = datalist_create(NULL);
      dictionary_set(dictionary, "$", (data_t *) argv);
    } else {
      assert(data_is_list(argv));
    }
    datalist_push(argv, data_copy(value));
    datalist_free(argv);
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
  dictionary_t *ret;

  ret = dictionary_create(NULL);
  if (ret) {
    dict_reduce_chars(
      dict, (reduce_t) _dictionary_from_dict_reducer, ret);
  }
  return ret;
}

data_t * dictionary_get(dictionary_t *dictionary, char *name) {
  return data_copy(_dictionary_resolve(dictionary,name));
}

data_t * dictionary_pop(dictionary_t *dictionary, char *name) {
  return (data_t *) dict_pop(dictionary -> attributes, name);
}

data_t * _dictionary_set(dictionary_t *dictionary, char *name, data_t *value) {
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

dictionaryiter_t * _dictionaryiter_new(dictionaryiter_t *iter, va_list args) {
  dictionary_t *dict = va_arg(args, dictionary_t *);

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
