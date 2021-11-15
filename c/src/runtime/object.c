/*
 * /obelix/src/object.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include "libruntime.h"

static inline void   _object_init(void);

static object_t *    _object_new(object_t *, va_list);
static void          _object_free(object_t *);
static char *        _object_allocstring(object_t *);
static data_t *      _object_cast(object_t *, int);
static int           _object_len(object_t *);

static data_t *      _object_mth_create(data_t *, char *, arguments_t *);
static data_t *      _object_mth_new(data_t *, char *, arguments_t *);

static data_t *      _object_call_attribute(object_t *, char *, arguments_t *);
static object_t *    _object_set_all_reducer(entry_t *, object_t *);

  /* ----------------------------------------------------------------------- */

_unused_ static vtable_t _vtable_Object[] = {
  { .id = FunctionNew,         .fnc = (void_t) _object_new },
  { .id = FunctionCmp,         .fnc = (void_t) object_cmp },
  { .id = FunctionCast,        .fnc = (void_t) _object_cast },
  { .id = FunctionFree,        .fnc = (void_t) _object_free },
  { .id = FunctionAllocString, .fnc = (void_t) _object_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) object_hash },
  { .id = FunctionCall,        .fnc = (void_t) object_call },
  { .id = FunctionResolve,     .fnc = (void_t) object_get },
  { .id = FunctionSet,         .fnc = (void_t) object_set },
  { .id = FunctionLen,         .fnc = (void_t) _object_len },
  { .id = FunctionEnter,       .fnc = (void_t) object_ctx_enter },
  { .id = FunctionLeave,       .fnc = (void_t) object_ctx_leave },
  { .id = FunctionNone,        .fnc = NULL }
};

_unused_ static methoddescr_t _methods_Object[] = {
  { .type = Any,    .name = "object", .method = _object_mth_create, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = Any,    .name = "new",    .method = _object_mth_new,    .argtypes = { Any, Any, Any },          .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,     .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int          Object = -1;
_unused_ int object_debug = 0;

/* ----------------------------------------------------------------------- */

void _object_init(void) {
  if (Object < 1) {
    logging_register_module(object);
    typedescr_register_with_methods(Object, object_t);
  }
}

/* ----------------------------------------------------------------------- */

object_t * _object_new(object_t *obj, va_list args) {
  data_t         *constructor = va_arg(args, data_t *);
  object_t       *constructor_obj = NULL;
  data_t         *c = NULL;
  bound_method_t *bm;
  dictionary_t   *tmpl = NULL;

  debug(object, "new '%s'", data_tostring(constructor));
  obj -> constructing = FALSE;
  obj -> variables = dictionary_create(NULL);
  obj -> ptr = NULL;
  if (data_is_script(constructor)) {
    bm = script_bind(data_as_script(constructor), obj);
    c = (data_t *) bound_method_copy(bm);
    tmpl = data_as_script(constructor) -> functions;
  } else if (data_is_object(constructor)) {
    constructor_obj = data_as_object(constructor);
    bm = data_as_bound_method(constructor_obj -> constructor);
    if (bm) {
      bm = script_bind(bm -> script, obj);
      c = (data_t *) bound_method_copy(bm);
    }
    tmpl = constructor_obj -> variables;
  }
  obj -> constructor = c;
  if (tmpl) {
    dictionary_reduce(tmpl, _object_set_all_reducer, obj);
  }
  return obj;
}

void _object_free(object_t *object) {
  if (object) {
    data_free(_object_call_attribute(object, "__finalize__", NULL));
    dictionary_free(object -> variables);
    data_free(object -> constructor);
    data_free(object -> retval);
  }
}

char * _object_allocstring(object_t *object) {
  data_t  *data = NULL;
  char    *buf;

  if (!object -> constructing) {
    data = object_get(object, "name");
    if (!data) {
      data = _object_call_attribute(object, "__str__", NULL);
    }
  }
  if (!data) {
    if (object -> constructor) {
      asprintf(&buf, "<%s object at %p>",
               data_tostring(object -> constructor), object);
    } else {
      asprintf(&buf, "<anon object at %p>", object);
    }
  } else {
    buf = strdup(data_tostring(data));
  }
  data_free(data);
  return buf;
}

data_t * _object_cast(object_t *obj, int totype) {
  data_t   *ret = NULL;

  switch (totype) {
    case Bool:
      ret = int_as_bool(obj && dictionary_size(obj -> variables));
      break;
  }
  return ret;
}

int _object_len(object_t *obj) {
  return dictionary_size(obj -> variables);
}

/* ----------------------------------------------------------------------- */

data_t * _object_mth_create(data_t *self, char *name, arguments_t *args) {
  object_t *obj;

  (void) self;
  (void) name;
  _object_init();
  obj = (object_t *) data_create(Object, NULL);
  arguments_reduce_kwargs(args, _object_set_all_reducer, obj);
  return (data_t *) obj;
}

data_t * _object_mth_new(data_t *self, char *fncname, arguments_t *args) {
  data_t      *ret;
  name_t      *name = NULL;
  data_t      *n;
  script_t    *script = NULL;
  arguments_t *shifted;

  (void) fncname;
  _object_init();
  shifted = arguments_shift(args, &n);
  if (data_is_name(n)) {
    name = name_copy(data_as_name(n));
  } else if (data_type(n) == String) {
    name = name_parse(data_tostring(n));
  }
  if (name) {
    data_free(n);
    n = data_resolve(self, name);
  }
  if (!data_is_exception(n)) {
    if (data_is_object(n)) {
      script = data_as_bound_method(data_as_object(n) -> constructor) -> script;
    } else if (data_is_closure(n)) {
      script = data_as_closure(n) -> script;
    } else if (data_is_script(n)) {
      script = data_as_script(n);
    } else if (data_is_bound_method(n)) {
      script = data_as_bound_method(n) -> script;
    }
    if (script) {
      debug(object, "'%s'.new(%s, %s)", data_tostring(n), arguments_tostring(shifted));
      ret = script_create_object(script, shifted);
      assert(data_is_object(ret) || data_is_exception(ret));
    } else {
      ret = data_exception(ErrorType,
          "Cannot use '%s' of type '%s' as an object factory",
          data_tostring(n), data_typename(n));
    }
  } else {
    ret = data_copy(n);
  }
  arguments_free(shifted);
  name_free(name);
  data_free(n);
  return ret;
}

/* ----------------------------------------------------------------------- */

data_t * _object_call_attribute(object_t *object, char *name, arguments_t *args) {
  data_t *func;
  data_t *ret = NULL;

  func = object_get(object, name);
  if (data_is_callable(func)) {
    ret = data_call(func, args);
  }
  data_free(func);
  return ret;
}

object_t * _object_set_all_reducer(entry_t *e, object_t *object) {
  object_set(object, (char *) e -> key, (data_t *) e -> value);
  return object;
}

/* ----------------------------------------------------------------------- */

object_t * object_create(data_t *constructor) {
  _object_init();
  return (object_t *) data_create(Object, constructor);
}

object_t * object_bind_all(object_t *object, data_t *template) {
  dictionary_t *variables = NULL;

  if (data_is_script(template)) {
    variables = data_as_script(template) -> functions;
  } else if (data_is_object(template)) {
    variables = data_as_object(template) -> variables;
  } else if (data_is_closure(template)) {
    variables = data_as_closure(template) -> script -> functions;
  }
  if (variables) {
    dictionary_reduce(variables, _object_set_all_reducer, object);
  }
  return object;
}

data_t * object_get(object_t *object, char *name) {
  data_t *ret;

  ret = dictionary_get(object -> variables, name);
  if (ret) {
    ret = data_copy(ret);
  } else if (!strcmp(name, "$constructing")) {
    ret = int_as_bool(object -> constructing);
  }
  return ret;
}

data_t * object_set(object_t *object, char *name, data_t *value) {
  bound_method_t *bm = NULL;

  debug(object, "object_set('%s', '%s', '%s')",
    object_tostring(object), name, data_tostring(value));
  if (data_is_script(value)) {
    bm = script_bind(data_as_script(value), object);
  } else if (data_is_bound_method(value)) {
    bm = script_bind(data_as_bound_method(value) -> script, object);
  } else if (data_is_closure(value)) {
    bm = script_bind(data_as_closure(value) -> script, object);
  }
  if (bm) {
    value = (data_t *) bound_method_copy(bm);
  }
  dictionary_set(object -> variables, name, value);
  debug(object, "   After set('%s') -> variables = %s",
    object_tostring(object), dictionary_tostring(object -> variables));
  return value;
}

_unused_ int object_has(object_t *object, char *name) {
  int ret;
  ret = dictionary_has(object -> variables, name);
  debug(object, "   object_has('%s', '%s'): %d", object_tostring(object), name, ret);
  return ret;
}

data_t * object_call(object_t *object, arguments_t *args) {
  data_t *ret;

  debug(object, "object_call('%s', %s)", object_tostring(object), arguments_tostring(args));
  ret = _object_call_attribute(object, "__call__", args);
  if (!ret || data_is_exception(ret)) {
    data_free(ret);
    ret = data_exception(ErrorNotCallable, "Object '%s' is not callable",
                     object_tostring(object));
  }
  debug(object, "object_call returns '%s'", data_tostring(ret));
  return ret;
}

unsigned int object_hash(object_t *object) {
  data_t       *data;
  unsigned int  ret;

  data = _object_call_attribute(object, "__hash__", NULL);
  ret = (data && (data_type(data) == Int)) ? (unsigned int) data_intval(data) : hashptr(object);
  data_free(data);
  return ret;
}

int object_cmp(object_t *o1, object_t *o2) {
  data_t      *data;
  arguments_t *args;
  int          ret;

  args = arguments_create_args(1, (data_t *) object_copy(o2));
  data = _object_call_attribute(o1, "__cmp__", args);
  ret = (data && !data_is_exception(data))
      ? data_intval(data)
      : (int) (((intptr_t) o1) - ((intptr_t) o2));
  data_free(data);
  arguments_free(args);
  return ret;
}

data_t * object_ctx_enter(object_t *object) {
  data_t *ret = NULL;

  debug(object, "'%s'.__enter__", object_tostring(object));
  ret = _object_call_attribute(object, "__enter__", NULL);
  if (ret && !data_is_exception(ret)) {
    ret = NULL;
  }
  return ret;
}

data_t *  object_ctx_leave(object_t *object, data_t *arg) {
  arguments_t *args;
  data_t      *ret = NULL;

  debug(object, "'%s'.__exit__('%s')",
    object_tostring(object), data_tostring(arg));
  args = arguments_create_args(1, arg);
  if (data_is_exception(arg)) {
    ret = _object_call_attribute(object, "__catch__", args);
  }
  if (ret && data_is_exception(ret)) {
    /*
     * If __catch__ returns an exception, this exception is passed to
     * __exit__ instead of the original exception.
     */
    data_free(arg);
    arguments_free(args);
    args = arguments_create_args(1, ret);
  }
  data_free(ret);
  ret = _object_call_attribute(object, "__exit__", args);
  arguments_free(args);
  return ret;
}
