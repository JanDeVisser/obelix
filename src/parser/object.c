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

#include <error.h>
#include <object.h>
#include <script.h>

static typedescr_t typedescr_object;

static data_t *     _data_new_object(data_t *, va_list);
static data_t *     _data_copy_object(data_t *, data_t *);
static int          _data_cmp_object(data_t *, data_t *);
static char *       _data_tostring_object(data_t *);
static unsigned int _data_hash_object(data_t *);
static data_t *     _data_execute_object(data_t *, char *, array_t *, dict_t *);

/*
 * data_t Object type
 */

static typedescr_t typedescr_object = {
  type:                  Object,
  typecode:              "O",
  typename:              "object",
  new:      (new_t)      _data_new_object,
  copy:     (copydata_t) _data_copy_object,
  cmp:      (cmp_t)      _data_cmp_object,
  free:     (free_t)     object_free,
  tostring: (tostring_t) _data_tostring_object,
  hash:     (hash_t)     _data_hash_object,
  parse:                 NULL,
  cast:                  NULL,
  fallback: (method_t)   _data_execute_object
};

data_t * _data_new_object(data_t *ret, va_list arg) {
  object_t *object;

  object = va_arg(arg, object_t *);
  ret -> ptrval = object;
  ((object_t *) ret -> ptrval) -> refs++;
  return ret;
}

data_t * _data_copy_object(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((object_t *) target -> ptrval) -> refs++;
  return target;
}

int _data_cmp_object(data_t *d1, data_t *d2) {
  return object_cmp((object_t *) d1 -> ptrval, (object_t *) d2 -> ptrval);
}

char * _data_tostring_object(data_t *d) {
  return object_tostring((object_t *) d -> ptrval);
}

unsigned int _data_hash_object(data_t *d) {
  return object_hash((object_t *) d -> ptrval);
}

data_t * _data_execute_object(data_t *self, char *name, array_t *params, dict_t *kwargs) {
  object_t *o = (object_t *) self -> ptrval;
  return object_execute(o, name, params, kwargs);
}

data_t * data_create_object(object_t *object) {
  return data_create(Object, object);
}

/*
 * object_t public functions
 */

object_t * object_create(script_t *script) {
  static int  type_object = -1;
  object_t   *ret;

  if (type_object < 0) {
    type_object = typedescr_register(typedescr_int);
  }
  ret = NEW(object_t);
  ret -> script = script;
  ret -> ptr = NULL;
  ret -> variables = strdata_dict_create();
  dict_reduce(ret -> variables, (reduce_t) data_add_all_reducer, script -> functions);
  ret -> refs = 0;
}

void object_free(object_t *object) {
  if (object) {
    object -> refs--;
    if (--object -> refs <= 0) {
      object_execute(object, "__finalize__", NULL, NULL);
      dict_free(object -> variables);
      free(object -> str);
    }
  }
}

data_t * object_get(object_t *object, char *name) {
  data_t *ret;

  ret = (data_t *) dict_get(object -> variables, name);
  if (ret) {
    ret = data_copy(ret);
  } else {
    ret = data_error(ErrorName,
                     "Object '%s' of type '%s' has no attribute '%s'",
                     object_tostring(object),
                     script_tostring(object -> script),
                     name);
  }
  return ret;
}

object_t * object_set(object_t *object, char *name, data_t *value) {
  dict_put(object -> variables, strdup(name), data_copy(value));
  return object;
}

data_t * object_execute(object_t *object, char *name, array_t *args, dict_t *kwargs) {
  script_t *script;
  data_t   *func;
  data_t   *ret;
  data_t   *self;

  func = object_get(object, name);
  if (!data_is_error(func)) {
    if (data_type(func) != Script)  {
      ret = data_error(ErrorName,
                       "Attribute '%s' of object '%s' of type '%s' is not callable",
                       name,
                       object_tostring(object),
                       script_tostring(object -> script));
    } else {
      script = (script_t *) func -> ptrval;
      self = data_create_object(object);
      ret = script_execute(script, self, args, kwargs);
      data_free(self);
    }
    data_free(func);
  } else {
    ret = func;
  }
  return ret;
}

char * object_tostring(object_t *object) {
  data_t   *data;

  data = object_execute(object, "__str__", NULL, NULL);
  if (data -> type != Error) {
    object -> str = strdup((char *) data -> ptrval);
  } else {
    object -> str = (char *) new(snprintf(NULL, 0, "<%s object at %p>",
                                          script_tostring(object -> script),
                                          object));
    sprintf(object -> str, "<%s object at %p>",
            script_tostring(object -> script),
            object);
  }
  data_free(data);
  return object -> str;
}

unsigned int object_hash(object_t *object) {
  data_t       *data;
  unsigned int  ret;

  data = object_execute(object, "__hash__", NULL, NULL);
  ret = (data -> type != Error)
      ? (unsigned int) data -> intval
      : hashptr(object);
  data_free(data);
  return ret;
}

int object_cmp(object_t *o1, object_t *o2) {
  data_t  *data;
  array_t *args;
  int      ret;

  args = data_array_create(1);
  array_set(args, 0, data_create(Object, o2));
  data = object_execute(o1, "__cmp__", args, NULL);
  ret = (data -> type != Error)
      ? data -> intval
      : (long) o1 - (long) o2;
  data_free(data);
  array_free(args);
  return ret;
}

/**
 * Resolves a name path rooted in the specified object.
 *
 * @param object The root object for name resolution.
 * @param name The name to resolve.
 *
 * @return A data_t object containing the object referenced by the
 * second-to-last component in the name path. The last component
 * is an attribute of that object, and can itself reference an
 * object again, but does not necessarily need to exist or could
 * reference a data_t of a different type. If the name array only
 * holds one element, the object specified is wrapped. If the path
 * is broken along the way, or is empty, a data_error(ErrorName) is returned.
 * Note that the caller is responsible for freeing the data_t.
 */
data_t * object_resolve(object_t *object, array_t *name) {
  data_t   *ret;
  object_t *o;
  int       ix;

  if (!name || !array_size(name)) {
    return data_error(ErrorName, "Empty name");
  }
  o = object;
  for (ix = 0; o && (ix < array_size(name) - 1); ix++) {
    char *n = (char *) (((data_t *) array_get(name, ix)) -> ptrval);
    ret = object_get(o, n);
    if (data_is_error(ret)) {
      break;
    }
    if (data_type(ret) != Object) {
      data_free(ret);
      ret = data_error(ErrorName,
                       "Attribute '%s' of object '%s' of type '%s' is not an object",
                       n,
                       object_tostring(o),
                       script_tostring(object -> script));
      break;
    }
    o = (object_t *) (ret -> ptrval);
    data_free(ret);
  }
  if (!data_is_error(ret)) {
    ret = data_create_object(o) ;
  }
}
