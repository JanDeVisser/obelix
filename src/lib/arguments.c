/*
 * /obelix/src/lib/arguments.c - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#include "libcore.h"

#include <data.h>

static void            _arguments_init(void);
static arguments_t *   _arguments_new(arguments_t *, va_list);
static void            _arguments_free(arguments_t *);
static char *          _arguments_tostring(arguments_t *);
static data_t *        _arguments_cast(arguments_t *, int);
static data_t *        _arguments_resolve(arguments_t *, char *);
static arguments_t *   _arguments_set(arguments_t *, char *, data_t *);
static dictionary_t *  _arguments_serialize(arguments_t *);
static arguments_t *   _arguments_deserialize(dictionary_t *);
static void *          _arguments_reduce_children(arguments_t *, reduce_t, void *);

/* ----------------------------------------------------------------------- */

_unused_ static vtable_t _vtable_Arguments[] = {
  { .id = FunctionNew,          .fnc = (void_t) _arguments_new },
  { .id = FunctionCast,         .fnc = (void_t) _arguments_cast },
  { .id = FunctionAllocString,  .fnc = (void_t) _arguments_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _arguments_resolve },
  { .id = FunctionSet,          .fnc = (void_t) _arguments_set },
  { .id = FunctionLen,          .fnc = (void_t) arguments_args_size },
  { .id = FunctionSerialize,    .fnc = (void_t) _arguments_serialize },
  { .id = FunctionDeserialize,  .fnc = (void_t) _arguments_deserialize },
  { .id = FunctionReduce,       .fnc = (void_t) _arguments_reduce_children },
  { .id = FunctionNone,         .fnc = NULL }
};

_unused_ static methoddescr_t _methods_Arguments[] = {
  { .type = NoType, .name = NULL,         .method = NULL,             .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int Arguments = -1;

/* ----------------------------------------------------------------------- */

void _arguments_init(void) {
  if (Arguments < 1) {
    typedescr_register(Arguments, arguments_t);
  }
}

/* ----------------------------------------------------------------------- */

arguments_t * _arguments_new(arguments_t *arguments, va_list args) {
  array_t *argv = va_arg(args, array_t *);
  dict_t  *kwargs = va_arg(args, dict_t *);

  arguments -> args = datalist_create(argv);
  arguments -> kwargs = dictionary_create_from_dict(kwargs);
  return arguments;
}

char * _arguments_tostring(arguments_t *arguments) {
  char *buf;

  if (datalist_size(arguments -> args) && dictionary_size(arguments -> kwargs)) {
    asprintf(&buf, "%s, %s",
        datalist_tostring(arguments -> args),
        dict_tostring_custom(arguments -> kwargs -> attributes, "", "%s=%s", ", ", ""));
  } else if (datalist_size(arguments -> args) && !dictionary_size(arguments -> kwargs)) {
    asprintf(&buf, "%s", datalist_tostring(arguments -> args));
  } else if (!datalist_size(arguments -> args) && dictionary_size(arguments -> kwargs)) {
    asprintf(&buf, "%s",
        dict_tostring_custom(arguments -> kwargs -> attributes, "", "%s=%s", ", ", ""));
  } else {
    asprintf(&buf, "%s", "");
  }
  return buf;
}

data_t * _arguments_cast(arguments_t *obj, int totype) {
  data_t   *ret = NULL;

  switch (totype) {
    case Bool:
      ret = int_as_bool(obj && ((obj -> args && datalist_size(obj -> args)) ||
                                (obj -> kwargs && dictionary_size(obj -> kwargs))));
      break;
  }
  return ret;
}

data_t * _arguments_resolve(arguments_t *arguments, char *name) {
  long l;

  if (!strcmp(name, "kwargs")) {
    return data_as_data(arguments->kwargs);
  } else if (!strcmp(name, "args")) {
    return data_as_data(arguments->args);
  } else {
    return (strtoint(name, &l))
           ? arguments_get_kwarg(arguments, name)
           : arguments_get_arg(arguments, (int) l);
  }
}

arguments_t * _arguments_set(arguments_t *arguments, char *name, data_t *value) {
  long l;

  return (strtoint(name, &l))
         ? arguments_set_kwarg(arguments, name, value)
         : arguments_set_arg(arguments, (int) l, value);
}

arguments_t * _arguments_deserialize(dictionary_t *dict) {
  arguments_t *args = data_new(Arguments, arguments_t);

  args -> args = (datalist_t *) dictionary_get(dict, "args");
  args -> kwargs = (dictionary_t *) dictionary_get(dict, "kwargs");
  return args;
}

dictionary_t * _arguments_serialize(arguments_t *arguments) {
  dictionary_t *ret = dictionary_create(NULL);

  dictionary_set(ret, "args", data_serialize((data_t *) arguments -> args));
  dictionary_set(ret, "kwargs", data_serialize((data_t *) arguments -> kwargs));
  return ret;
}

void * _arguments_reduce_children(arguments_t *args, reduce_t reducer, void *ctx) {
  return reducer(args->kwargs, reducer(args->args, ctx));
}

/* ----------------------------------------------------------------------- */

arguments_t * arguments_create(array_t *args, dict_t *kwargs) {
  _arguments_init();
  return (arguments_t *) data_create(Arguments, args, kwargs);
}

arguments_t * arguments_create_args(int num, ...) {
  arguments_t *args = arguments_create(NULL, NULL);
  data_t      *data;
  va_list      list;

  va_start(list, num);
  for (int ix = 0; ix < num; ix++) {
    data = va_arg(list, data_t *);
    datalist_push(args -> args, data);
  }
  va_end(list);
  return args;
}

static datalist_t * _arguments_copy_args(data_t *entry, datalist_t *args) {
  datalist_push(args, entry);
  return args;
}

static dictionary_t * _arguments_copy_kwargs(entry_t *entry, dictionary_t *kwargs) {
  dictionary_set(kwargs, (char *) entry -> key, (data_t *) entry -> value);
  return kwargs;
}

arguments_t * arguments_deepcopy(const arguments_t *src) {
  arguments_t *dest = arguments_create(NULL, NULL);

  array_reduce(data_as_array(src -> args),
      (reduce_t) _arguments_copy_args, dest -> args);
  dictionary_reduce(src -> kwargs, (reduce_t) _arguments_copy_kwargs, dest -> kwargs);
  return dest;
}

arguments_t * arguments_create_from_cmdline(int argc, char **argv) {
  arguments_t *ret = arguments_create(NULL, NULL);

  for (int ix = 0; ix < argc; ix++) {
    datalist_push(ret -> args, str(argv[ix]));
  }
  return ret;
}

data_t * arguments_get_arg(const arguments_t *arguments, int ix) {
  if (!arguments -> args ||
      (ix < -datalist_size(arguments -> args)) ||
      (ix >= datalist_size(arguments -> args))) {
    return data_exception(ErrorRange, "Index out of range");
  } else {
    return datalist_get(arguments -> args, ix);
  }
}

data_t * arguments_get_kwarg(const arguments_t *arguments, char *name) {
  return dictionary_get(arguments -> kwargs, name);
}

int arguments_has_kwarg(const arguments_t *arguments, char *name) {
  return dictionary_has(arguments -> kwargs, name);
}

arguments_t * _arguments_set_arg(arguments_t *arguments, int ix, data_t *data) {
  datalist_set(arguments -> args, ix, data);
  return arguments;
}

arguments_t * _arguments_set_kwarg(arguments_t *arguments, char *key, data_t *data) {
  dictionary_set(arguments -> kwargs, key, data);
  return arguments;
}

arguments_t * arguments_shift(const arguments_t *arguments, data_t **shifted) {
  arguments_t *ret = arguments_deepcopy(arguments);
  array_t     *args = data_as_array(ret -> args);
  data_t      *s = array_remove(args, 0);

  if (shifted) {
    *shifted = s;
  }
  return ret;
}

arguments_t * _arguments_push(arguments_t *arguments, data_t *data) {
  datalist_push(arguments -> args, data);
  return arguments;
}
