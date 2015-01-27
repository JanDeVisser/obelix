/*
 * /obelix/src/data.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <error.h>
#include <list.h>
#include <object.h>

static typedescr_t   typedescr_any;
static methoddescr_t methoddescr_any[];


static void          _data_initialize_types(void);

static data_t *      _any_cmp(data_t *, char *, array_t *, dict_t *);
static data_t *      _any_hash(data_t *, char *, array_t *, dict_t *);

// -----------------------
// Data conversion support

static int          data_numtypes = 0;
static typedescr_t *descriptors = NULL;

#ifdef NDEBUG
static
#endif
int data_count = 0;

static typedescr_t typedescr_any = {
  type:                  Any,
  typecode:              "A",
  typename:              "any"
};

static methoddescr_t methoddescr_any[] = {
  { type: Any,    name: ">",    method: _any_cmp,  argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Any,    name: "<",    method: _any_cmp,  argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Any,    name: ">=",   method: _any_cmp,  argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Any,    name: "<=",   method: _any_cmp,  argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Any,    name: "==",   method: _any_cmp,  argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Any,    name: "!=",   method: _any_cmp,  argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: Any,    name: "hash", method: _any_hash, argtypes: { Any, NoType, NoType }, minargs: 1, varargs: 0 },
  { type: NoType, name: NULL,   method: NULL,      argtypes: { NoType, NoType, NoType },  minargs: 0, varargs: 0 }
};


/*
 * 'any' global methods
 */

data_t * _any_cmp(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *other = (data_t *) array_get(args, 0);
  data_t *ret;
  int     cmp = data_cmp(self, other);
  
  if (!strcmp(name, "==")) {
    ret = data_create(Bool, !cmp);
  } else if (!strcmp(name, "!=")) {
    ret = data_create(Bool, cmp);
  } else if (!strcmp(name, ">")) {
    ret = data_create(Bool, cmp > 0);
  } else if (!strcmp(name, ">=")) {
    ret = data_create(Bool, cmp >= 0);
  } else if (!strcmp(name, "<")) {
    ret = data_create(Bool, cmp < 0);
  } else if (!strcmp(name, "<=")) {
    ret = data_create(Bool, cmp <= 0);
  } else {
    assert(0);
    ret = data_create(Bool, 0);
  }
  /*
  debug("_any_cmp('%s' %s '%s') [cmp: %d] = %s", 
        data_debugstr(self), name, data_debugstr(other), cmp, data_debugstr(ret));
  */
  return ret;
}

data_t * _any_hash(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, data_hash(self));
}

/*
 * typedescr_t public functions
 */

int typedescr_register(typedescr_t descr) {
  typedescr_t *new_descriptors;
  int          newsz;
  int          cursz;
  typedescr_t *d;

  /* debug("Registering typedescr %s", typedescr_tostring(&descr)); */
  if (descr.type < 0) {
    descr.type = data_numtypes;
    /* debug("Assigned type %d", descr.type); */
  }
  assert((descr.type >= data_numtypes) || descriptors[descr.type].type == 0);
  if (descr.type >= data_numtypes) {
    newsz = (descr.type + 1) * sizeof(typedescr_t);
    cursz = data_numtypes * sizeof(typedescr_t);
    new_descriptors = (typedescr_t *) resize_block(descriptors, newsz, cursz);
    descriptors = new_descriptors;
    data_numtypes = descr.type + 1;
  }
  d = &descriptors[descr.type];
  memcpy(d, &descr, sizeof(typedescr_t));
  return d -> type;
}

typedescr_t * typedescr_get(int datatype) {
  typedescr_t *ret = NULL;
  
  if (datatype == Any) {
    return &typedescr_any;
  } else if ((datatype >= 0) && (datatype < data_numtypes)) {
    ret = &descriptors[datatype];
  }
  if (!ret) {
    error("Undefined type %d referenced. Expect crashes", datatype);
    return NULL;
  } else {
    return ret;
  }
}

void typedescr_register_methods(methoddescr_t methods[]) {
  methoddescr_t *method;
  typedescr_t   *type;

  for (method = methods; method -> type >= 0; method++) {
    type = typedescr_get(method -> type);
    assert(type);
    typedescr_register_method(type, method);
  }
}

void typedescr_register_method(typedescr_t *type, methoddescr_t *method) {
  assert(method -> name && method -> method);
  //assert((!method -> varargs) || (method -> minargs > 0));
  if (!type -> methods) {
    type -> methods = strvoid_dict_create();
  }
  dict_put(type -> methods, strdup(method -> name), method);
}

methoddescr_t * typedescr_get_method(typedescr_t *descr, char *name) {
  if (!descr -> methods) {
    descr -> methods = strvoid_dict_create();
  }
  return (methoddescr_t *) dict_get(descr -> methods, name);
}

char * typedescr_tostring(typedescr_t *type) {
  static char buf[100];
  
  snprintf(buf, 100, "'%s' [%s][%d]",
           type -> typename, type -> typecode, type -> type);
  return buf;
}

/*
 * data_t static functions
 */

static int _types_initialized = 0;

void _data_initialize_types(void) {
  if (!_types_initialized) {
    typedescr_register(typedescr_int);
    typedescr_register(typedescr_bool);
    typedescr_register(typedescr_float);
    typedescr_register(typedescr_str);
    typedescr_register(typedescr_ptr);
    typedescr_register(typedescr_fnc);
    typedescr_register(typedescr_list);
    typedescr_register(typedescr_error);
    
    typedescr_register_methods(methoddescr_any);
    typedescr_register_methods(methoddescr_int);
    typedescr_register_methods(methoddescr_bool);
    typedescr_register_methods(methoddescr_float);
    typedescr_register_methods(methoddescr_str);
    typedescr_register_methods(methoddescr_ptr);
    typedescr_register_methods(methoddescr_fnc);
    typedescr_register_methods(methoddescr_list);

    _types_initialized = 1;
  }
}

/*
 * data_t public functions
 */

data_t * data_create(int type, ...) {
  va_list      arg;
  data_t      *ret;
  void        *value;
  typedescr_t *descr;

  _data_initialize_types();
  descr = typedescr_get(type);
  
  ret = NEW(data_t);
  ret -> type = type;
  ret -> refs = 1;
  ret -> str = NULL;
  data_count++;
#ifndef NDEBUG
  ret -> debugstr = NULL;
#endif
  
  if (descr -> new) {
    va_start(arg, type);
    ret = descr -> new(ret, arg);
    va_end(arg);
  }
  /* debug("data_create(%s, ...) = %s", typedescr_tostring(descr), data_tostring(ret)); */
  return ret;
}

data_t * data_create_pointer(int sz, void *ptr) {
  return data_create(Pointer, sz, ptr);
}

data_t * data_null(void) {
  return data_create_pointer(0, NULL);
}

data_t * data_error(int code, char * msg, ...) {
  char    buf[256];
  char   *ptr;
  int     size;
  va_list args;
  va_list args_copy;
  data_t *ret;

  va_start(args, msg);
  va_copy(args_copy, args);
  size = vsnprintf(buf, 256, msg, args);
  if (size > 256) {
    ptr = (char *) new(size);
    vsprintf(ptr, msg, args_copy);
  } else {
    ptr = buf;
  }
  va_end(args);
  va_end(args_copy);
  ret = data_create(Error, code, ptr);
  if (size > 256) {
    free(ptr);
  }
  return ret;
}

data_t * data_create_string(char * value) {
  return data_create(String, value);
}

data_t * data_parse(int type, char *str) {
  typedescr_t *descr;

  _data_initialize_types();
  descr = typedescr_get(type);
  return (descr -> parse) ? descr -> parse(str) : NULL;
}

data_t * data_cast(data_t *data, int totype) {
  typedescr_t *descr;
  typedescr_t *totype_descr;

  assert(data);
  if (data_type(data) == totype) {
    return data_copy(data);
  } else {
    descr = typedescr_get(data_type(data));
    assert(descr);
    totype_descr = typedescr_get(totype);
    assert(totype_descr);
    if ((totype == String) && descr -> tostring) {
      return data_create(String, descr -> tostring(data));
    } else if ((data_type(data) == String) && totype_descr -> parse) {
      return totype_descr -> parse(data -> ptrval);
    } else if (descr -> cast) {
      return descr -> cast(data, totype);
    } else {
      return NULL;
    }
  }
}

data_t * data_promote(data_t *data) {
  typedescr_t *type = data_typedescr(data);
  
  return ((type -> promote_to != NoType) && (type -> promote_to != Error))
    ? data_cast(data, type -> promote_to)
    : NULL;
}

void data_free(data_t *data) {
  typedescr_t *type;

  if (data) {
    data -> refs--;
    if (data -> refs <= 0) {
      type = data_typedescr(data);
      if (type -> free) {
	type -> free(data -> ptrval);
      }
      free(data -> str);
#ifndef NDEBUG
      free(data -> debugstr);
#endif
      data_count--;
      free(data);
    }
  }
}

int data_type(data_t *data) {
  return data -> type;
}

typedescr_t * data_typedescr(data_t *data) {
  return typedescr_get(data_type(data));
}

int data_is_numeric(data_t *data) {
  return (data -> type == Int) || (data -> type == Float);
}

int data_hastype(data_t *data, int type) {
  if (type == Any) {
    return TRUE;
  } else if (type == Numeric) {
    return data_is_numeric(data);
  } else {
    return data_type(data) == type;
  }
}

data_t * data_copy(data_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
}

methoddescr_t * data_method(data_t *data, char *name) {
  return typedescr_get_method(data_typedescr(data), name);
}

data_t * data_execute_method(data_t *self, methoddescr_t *method, array_t *args, dict_t *kwargs) {
  typedescr_t *type;
  int          i;
  int          len;
  int          t;
  int          maxargs = -1;
  data_t      *arg;

  assert(method);
  assert(self);
  type = data_typedescr(self);
  
  len = (args) ? array_size(args) : 0;
  if (!method -> varargs) {
    maxargs = method -> minargs;
  }
  if (len < method -> minargs) {
    if (method -> varargs) {
      return data_error(ErrorArgCount, "%s.%s requires at least %d arguments",
                       type -> typename, method -> name, method -> minargs);
    } else {
      return data_error(ErrorArgCount, "%s.%s requires exactly %d arguments",
                       type -> typename, method -> name, method -> minargs);
    }
  } else if (!method -> varargs && (len > maxargs)) {
    if (maxargs == 0) {
      return data_error(ErrorArgCount, "%s.%s accepts no arguments",
                        type -> typename, method -> name);
    } else if (maxargs == 1) {
      return data_error(ErrorArgCount, "%s.%s accepts only one argument",
                        type -> typename, method -> name);
    } else {
      return data_error(ErrorArgCount, "%s.%s accepts only %d arguments",
                       type -> typename, method -> name, maxargs);
    }
  }
  for (i = 0; i < len; i++) {
    arg = array_get(args, i);
    t = (i < method -> minargs)
      ? method -> argtypes[i]
      : method -> argtypes[(method -> minargs) ? method -> minargs - 1 : 0];
    if (!data_hastype(arg, t)) {
      return data_error(ErrorType, "Type mismatch: Type of argument %d of %s.%s must be %d, not %d",
                        i+1, type -> typename, method -> name, t, data_type(arg));
    }
  }
  return method -> method(self, method -> name, args, kwargs);
}

data_t * data_execute(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  methoddescr_t *method;
  method_t       m;
  data_t        *ret = NULL;
  array_t       *args_shifted = NULL;

  if (!self && array_size(args)) {
    self = (data_t *) array_get(args, 0);
    args_shifted = array_slice(args, 1, -1);
    if (!args_shifted) {
      args_shifted = data_array_create(1);
    }
  }
  if (!self) {
    ret = data_error(ErrorArgCount, "No 'self' object specified for method '%s'", name);
  }

  method = data_method(self, name);
  if (method) {
    ret = data_execute_method(self, method, (args_shifted) ? args_shifted : args, kwargs);
  }
  if (!ret) {
    m = data_typedescr(self) -> fallback;
    if (m) {
      ret = m(self, name, (args_shifted) ? args_shifted : args, kwargs);
    }
  }
  if (!ret) {
    method = typedescr_get_method(typedescr_get(Any), name);
    if (method) {
      ret = data_execute_method(self, method, (args_shifted) ? args_shifted : args, kwargs);
    }
  }
  if (!ret) {
    ret = data_error(ErrorName, "data object '%s' has no method '%s",
                     data_tostring(self), name);
  }
  array_free(args_shifted);
  return ret;
}

data_t * data_resolve(data_t *data, arrat_t *name) {
  typedescr_t *type = data_typedescr(data);
  data_t      *ret = NULL;
  str_t       *name_str = NULL;
  
  assert(name && array_size(name));
  if (array_size(name) > 1) {
    if (!type -> resolve) {
      name_str = array_join(args, ".");
      ret = data_error(ErrorType,
                       "Cannot resolve name '%s' in %s '%s'", 
                       str_chars(name_str), type -> typename, object_tostring(data)
                      );
      str_free(name_str);
    } else {
      ret = type -> resolve(data, name);
    }
  } else {
    ret = data_copy(data);
  }
  return ret;
}

data_t * data_invoke(data_t *data, array_t *name, array_t *args, dict_t *kwargs) {
  data_t      *object;
  data_t      *ret = NULL;
  char        *method_name;
  
  object = data_resolve(data, name);
  if (!data_is_error(object)) {
    method_name = str_array_get(name, -1);
    ret = data_execute(object, method_name, args, kwargs);
  }
  data_free(object);
  return ret;
}

unsigned int data_hash(data_t *data) {
  typedescr_t *type = data_typedescr(data);

  return (type -> hash) ? type -> hash(data) : hashptr(data);
}

char * data_tostring(data_t *data) {
  int          len;
  char        *ret;
  typedescr_t *type;

  if (!data) {
    return "data:NULL";
  } else {
    free(data -> str);
    data -> str = NULL;
    type = data_typedescr(data);
    if (type -> tostring) {
      ret =  type -> tostring(data);
      if (ret && !data -> str) {
	data -> str = strdup(ret);
      }
    } else {
      len = snprintf(NULL, 0, "data:%s:%p", descriptors[data -> type].typecode, data);
      data -> str = new(len + 1);
      sprintf(data -> str, "data:%s:%p", descriptors[data -> type].typecode, data);
    }
    return data -> str;
  }
}

char * data_debugstr(data_t *data) {
#ifndef NDEBUG
  int          len;
  typedescr_t *type;

  free(data -> debugstr);
  type = data_typedescr(data);
  len = snprintf(NULL, 0, "%c %s", 
		 type -> typecode[0], 
		 data_tostring(data));
  data -> debugstr = (char *) new(len + 1);
  sprintf(data -> debugstr, "%c %s", 
	  type -> typecode[0], 
	  data_tostring(data));
  return data -> debugstr;
#else
  return data_tostring(data);
#endif
}

int data_cmp(data_t *d1, data_t *d2) {
  typedescr_t *type;
  data_t      *p1 = NULL;
  data_t      *p2 = NULL;
  int          ret;
  
  /* debug("data_cmp: comparing '%s' and '%s'", data_debugstr(d1), data_debugstr(d2)); */
  if (!d1 && !d2) {
    return 0;
  } else if (!d1 || !d2) {
    return (!d1) ? -1 : 1;
  } else if (d1 -> type != d2 -> type) {
    p1 = data_promote(d1);
    /*
    if (p1) {
      debug("data_cmp: promoted d1 to '%s'", data_debugstr(p1));
    }
    */
    if (p1 && (p1 -> type == d2 -> type)) {
      ret = data_cmp(p1, d2);
    } else {
      p2 = data_promote(d2);
      /*
      if (p2) {
        debug("data_cmp: promoted d2 to '%s'", data_debugstr(p2));
      }
      */
      if (p2 && (d1 -> type == p2 -> type)) {
	ret = data_cmp(d1, p2);
      } else if (p1 && !p2) {
	ret = data_cmp(p1, d2);
      } else if (!p1 && p2) {
	ret = data_cmp(d1, p2);
      } else if (p1 && p2) {
	ret = data_cmp(p1, p2);
      } else {
	ret = d1 -> type - d2 -> type;
      }
      data_free(p2);
    }
    data_free(p1);
    return ret;
  } else {
    type = data_typedescr(d1);
    if (type -> cmp) {
      return type -> cmp(d1, d2);
    } else {
      return (d1 -> ptrval == d2 -> ptrval) ? 0 : 1;
    }
  }
}

array_t * data_add_strings_reducer(data_t *data, array_t *target) {
  array_push(target, strdup(data_tostring(data)));
  return target;
}

array_t * data_add_all_reducer(data_t *data, array_t *target) {
  array_push(target, data_copy(data));
  return target;
}

dict_t * data_put_all_reducer(entry_t *e, dict_t *target) {
  dict_put(target, strdup(e -> key), data_copy(e -> value));
  return target;
}

