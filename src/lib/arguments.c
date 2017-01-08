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

#include <arguments.h>
#include <exception.h>
#include <str.h>

static void            _arguments_init(void);
static arguments_t *   _arguments_new(arguments_t *, va_list);
static void            _arguments_free(arguments_t *);
static char *          _arguments_tostring(arguments_t *);
static data_t *        _arguments_cast(arguments_t *, int);
static data_t *        _arguments_resolve(arguments_t *, char *);
static dictionary_t *  _arguments_serialize(arguments_t *);
static arguments_t *   _arguments_deserialize(dictionary_t *);

/* ----------------------------------------------------------------------- */

static vtable_t _vtable_Arguments[] = {
  { .id = FunctionNew,          .fnc = (void_t) _arguments_new },
  { .id = FunctionCast,         .fnc = (void_t) _arguments_cast },
  { .id = FunctionFree,         .fnc = (void_t) _arguments_free },
  { .id = FunctionAllocString,  .fnc = (void_t) _arguments_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _arguments_resolve },
  { .id = FunctionSerialize,    .fnc = (void_t) _arguments_serialize },
  { .id = FunctionDeserialize,  .fnc = (void_t) _arguments_deserialize },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methods_Arguments[] = {
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

void _arguments_free(arguments_t *arguments) {
  if (arguments) {
    dictionary_free(arguments -> kwargs);
    datalist_free(arguments -> args);
  }
}

char * _arguments_tostring(arguments_t *arguments) {
  char *buf;

  asprintf(&buf, "%s, %s",
      datalist_tostring(arguments -> args),
      dictionary_tostring(arguments -> kwargs));
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

  return (strtoint(name, &l))
    ? arguments_get_kwarg(arguments, name)
    : arguments_get_arg(arguments, l);
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

/* ----------------------------------------------------------------------- */

arguments_t * arguments_create(array_t *args, dict_t *kwargs) {
  _arguments_init();
  return (arguments_t *) data_create(Arguments, args, kwargs);
}

arguments_t * arguments_create_from_cmdline(int argc, char **argv) {
  array_t *args = array_create(argc);

  for (int ix = 0; ix < argc; ix++) {
    array_push(args, str_copy_chars(argv[ix]));
  }
  return arguments_create(args, NULL);
}

data_t * arguments_get_arg(arguments_t *arguments, int ix) {
  if (!arguments -> args || (ix < 0) || (ix >= datalist_size(arguments -> args))) {
    return data_exception(ErrorRange, "Index out of range");
  } else {
    return data_copy(datalist_get(arguments -> args, ix));
  }
}

data_t * arguments_get_kwarg(arguments_t *arguments, char *name) {
  return dictionary_get(arguments -> kwargs, name);
}
