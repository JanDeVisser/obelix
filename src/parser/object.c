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

#include <stdio.h>
#include <string.h>

#include <boundmethod.h>
#include <closure.h>
#include <exception.h>
#include <logging.h>
#include <native.h>
#include <object.h>
#include <script.h>

static void          _data_init_object(void) __attribute__((constructor));
static data_t *      _object_cast(object_t *, int);
static data_t *      _data_call_object(data_t *, array_t *, dict_t *);
static data_t *      _data_resolve_object(data_t *, char *);
static data_t *      _data_set_object(data_t *, char *, data_t *);
static int           _data_len_object(data_t *);
static data_t *      _data_ctx_enter(data_t *);
static data_t *      _data_ctx_leave(data_t *, data_t *);

static data_t *      _object_create(data_t *, char *, array_t *, dict_t *);
static data_t *      _object_new(data_t *, char *, array_t *, dict_t *);

static data_t *      _object_get(object_t *, char *);
static data_t *      _object_call_attribute(object_t *, char *, array_t *, dict_t *);
static object_t *    _object_set_all_reducer(entry_t *, object_t *);

extern void          _object_free(object_t *);
extern char *        _object_allocstring(object_t *);

  /* ----------------------------------------------------------------------- */

static vtable_t _vtable_object[] = {
  { .id = FunctionFactory,     .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,         .fnc = (void_t) object_cmp },
  { .id = FunctionCast,        .fnc = (void_t) _object_cast },
  { .id = FunctionFree,        .fnc = (void_t) _object_free },
  { .id = FunctionAllocString, .fnc = (void_t) _object_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) object_hash },

  xxxxxxxxxxxxxxxx
  { .id = FunctionResolve,     .fnc = (void_t) _data_resolve_object },
  { .id = FunctionCall,        .fnc = (void_t) _data_call_object },
  { .id = FunctionSet,         .fnc = (void_t) _data_set_object },
  { .id = FunctionLen,         .fnc = (void_t) _data_len_object },
  { .id = FunctionEnter,       .fnc = (void_t) _data_ctx_enter },
  { .id = FunctionLeave,       .fnc = (void_t) _data_ctx_leave },
  { .id = FunctionNone,        .fnc = NULL }
};

static typedescr_t _typedescr_object = {
  .type =      Object,
  .type_name = "object",
  .vtable =    _vtable_object
};

static methoddescr_t _methoddescr_object[] = {
  { .type = Any,    .name = "object", .method = _object_create, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 1 },
  { .type = Any,    .name = "new",    .method = _object_new,    .argtypes = { Any, Any, Any },          .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,     .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int Object = -1;
int obj_debug = 0;

/* ----------------------------------------------------------------------- */

void _data_init_object(void) {
  logging_register_category("object", &obj_debug);
  Object = typedescr_create_and_register(Object, "object",
                                         _vtable_object, _methoddescr_object);
}

/* ----------------------------------------------------------------------- */

void _object_free(object_t *object) {
  if (object) {
    data_free(_object_call_attribute(object, "__finalize__", NULL, NULL));
    dict_free(object -> variables);
    data_free(object -> constructor);
    data_free(object -> retval);
  }
}

char * _object_allocstring(object_t *object) {
  data_t  *data = NULL;
  char    *buf;

  if (!constructing) {
    data = _object_get(object, "name");
    if (!data) {
      data = _object_call_attribute(object, "__str__", NULL, NULL);
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
      ret = data_create(Bool, obj && dict_size(obj -> variables));
      break;
  }
  return ret;
}

data_t * _data_call_object(data_t *self, array_t *args, dict_t *kwargs) {
  return object_call(data_objectval(self), args, kwargs);
}

data_t * _data_resolve_object(data_t *data, char *name) {
  return object_resolve(data_objectval(data), name);
}

data_t * _data_set_object(data_t *data, char *name, data_t *value) {
  return object_set(data_objectval(data), name, value);
}

int _data_len_object(data_t *data) {
  object_t *obj = data_objectval(data);
  return dict_size(obj -> variables);
}

data_t* _data_ctx_enter(data_t *data) {
  return object_ctx_enter(data_objectval(data));
}

data_t* _data_ctx_leave(data_t *data, data_t *param) {
  return object_ctx_leave(data_objectval(data), param);
}

data_t * data_create_object(object_t *object) {
  return data_create(Object, object);
}

/* ----------------------------------------------------------------------- */

data_t * _object_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t   *ret;
  object_t *obj;

  (void) self;
  (void) name;
  obj = object_create(NULL);
  dict_reduce(kwargs, (reduce_t) _object_set_all_reducer, obj);
  ret = data_create(Object, obj);
  return ret;
}

data_t * _object_new(data_t *self, char *fncname, array_t *args, dict_t *kwargs) {
  data_t      *ret;
  name_t      *name = NULL;
  data_t      *n;
  script_t    *script = NULL;
  array_t     *shifted;
  object_t    *obj;
  typedescr_t *type;

  (void) fncname;
  n = data_copy(data_array_get(args, 0));
  if (data_is_name(n)) {
    name = name_copy(data_nameval(n));
  } else if (data_type(n) == String) {
    name = name_parse(data_tostring(n));
  }
  if (name) {
    data_free(n);
    n = data_resolve(self, name);
  }
  if (!data_is_exception(n)) {
    if (data_is_object(n)) {
      script = data_boundmethodval(data_objectval(n) -> constructor) -> script;
    } else if (data_is_closure(n)) {
      script = data_closureval(n) -> script;
    } else if (data_is_script(n)) {
      script = data_scriptval(n);
    } else if (data_is_boundmethod(n)) {
      script = data_boundmethodval(n) -> script;
    }
    if (script) {
      shifted = array_slice(args, 1, 0);
      ret = script_create_object(script, shifted, kwargs);
      array_free(shifted);
      assert(data_is_object(ret));
    } else {
      ret = data_exception(ErrorType, "Cannot use '%s' of type 's' as an object factory",
                       data_tostring(n), data_typedescr(n) -> type_name);
    }
  } else {
    ret = data_copy(n);
  }
  name_free(name);
  data_free(n);
  return ret;
}

/* ----------------------------------------------------------------------- */

data_t * _object_get(object_t *object, char *name) {
  data_t *ret;

  ret = (data_t *) dict_get(object -> variables, name);
  if (ret) {
    ret = data_copy(ret);
  }
  return ret;
}

data_t * _object_call_attribute(object_t *object, char *name, array_t *args, dict_t *kwargs) {
  data_t *func;
  data_t *ret = NULL;

  func = _object_get(object, name);
  if (data_is_callable(func)) {
    ret = data_call(func, args, kwargs);
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
  object_t       *ret;
  object_t       *obj = NULL;
  data_t         *c = NULL;
  bound_method_t *bm;
  dict_t         *template = NULL;

  ret = data_new(Object, object_t);
  ret -> refs = 1;
  ret -> constructing = FALSE;
  ret -> variables = strdata_dict_create();
  ret -> ptr = NULL;
  ret -> str = NULL;
  ret -> debugstr = NULL;
  if (data_is_script(constructor)) {
    bm = script_bind(data_scriptval(constructor), ret);
    c = data_create(BoundMethod, bm);
    template = data_scriptval(constructor) -> functions;
  } else if (data_is_object(constructor)) {
    obj = data_objectval(constructor);
    bm = data_boundmethodval(obj -> constructor);
    if (bm) {
      bm = script_bind(bm -> script, ret);
      c = data_create(BoundMethod, bm);
    }
    template = obj -> variables;
  }
  ret -> constructor = c;
  if (template) {
    dict_reduce(template, (reduce_t) _object_set_all_reducer, ret);
  }
  return ret;
}

object_t * object_bind_all(object_t *object, data_t *template) {
  dict_t *variables = NULL;

  if (data_is_script(template)) {
    variables = data_scriptval(template) -> functions;
  } else if (data_is_object(template)) {
    variables = data_objectval(template) -> variables;
  } else if (data_is_closure(template)) {
    variables = data_closureval(template) -> script -> functions;
  }
  if (variables) {
    dict_reduce(variables, (reduce_t) _object_set_all_reducer, object);
  }
  return object;
}

data_t * object_get(object_t *object, char *name) {
  data_t *ret;

  ret = _object_get(object, name);
  if (!ret && !strcmp(name, "$constructing")) {
    ret = data_create(Bool, object -> constructing);
  }
  if (!ret) {
    ret = data_exception(ErrorName,
                     "Object '%s' has no attribute '%s'",
                     object_debugstr(object),
                     name);
  }
  return ret;
}

data_t * object_set(object_t *object, char *name, data_t *value) {
  bound_method_t *bm;

  if (data_is_script(value)) {
    bm = script_bind(data_scriptval(value), object);
    value = data_create(BoundMethod, bm);
  } else if (data_is_boundmethod(value)) {
    bm = script_bind(data_boundmethodval(value) -> script, object);
    value = data_create(BoundMethod, bm);
  } else if (data_is_closure(value)) {
    bm = script_bind(data_closureval(value) -> script, object);
    value = data_create(BoundMethod, bm);
  }
  dict_put(object -> variables, strdup(name), data_copy(value));
  if (obj_debug) {
    debug("   object_set('%s') -> variables = %s",
          object -> constructor ? data_tostring(object -> constructor) : "anon",
          dict_tostring(object -> variables));
  }
  return value;
}

int object_has(object_t *object, char *name) {
  int ret;
  ret = dict_has_key(object -> variables, name);
  if (obj_debug) {
    debug("   object_has('%s', '%s'): %d", object_debugstr(object), name, ret);
  }
  return ret;
}

data_t * object_call(object_t *object, array_t *args, dict_t *kwargs) {
  data_t *ret;

  ret = _object_call_attribute(object, "__call__", args, kwargs);
  if (!ret || data_is_exception(ret)) {
    data_free(ret);
    ret = data_exception(ErrorNotCallable, "Object '%s' is not callable",
                     object_tostring(object));
  }
  return ret;
}

unsigned int object_hash(object_t *object) {
  data_t       *data;
  unsigned int  ret;

  data = _object_call_attribute(object, "__hash__", NULL, NULL);
  ret = (data && (data_type(data) == Int)) ? data_intval(data) : hashptr(object);
  data_free(data);
  return ret;
}

int object_cmp(object_t *o1, object_t *o2) {
  data_t  *data;
  array_t *args;
  int      ret;

  args = data_array_create(1);
  array_set(args, 0, data_create(Object, o2));
  data = _object_call_attribute(o1, "__cmp__", args, NULL);
  ret = (data && !data_is_exception(data))
      ? data_intval(data)
      : (long) o1 - (long) o2;
  data_free(data);
  array_free(args);
  return ret;
}

data_t * object_resolve(object_t *object, char *name) {
  return (data_t *) dict_get(object -> variables, name);
}

data_t * object_ctx_enter(object_t *object) {
  data_t *ret = NULL;

  ret = _object_call_attribute(object, "__enter__", NULL, NULL);
  if (ret && !data_is_exception(ret)) {
    ret = NULL;
  }
  return ret;

}

data_t *  object_ctx_leave(object_t *object, data_t *param) {
  array_t  *params;
  data_t   *ret;

  params = data_array_create(1);
  array_push(params, data_copy(param));
  ret = _object_call_attribute(object, "__exit__", params, NULL);
  array_free(params);
  return ret;
}
