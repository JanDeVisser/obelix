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

static void          _data_initialize_types(void);

// -----------------------
// Data conversion support

static int          data_numtypes = 0;
static typedescr_t *descriptors = NULL;

#ifdef NDEBUG
static
#endif
int data_count = 0;


/*
 * typedescr_t public functions
 */

int typedescr_register(typedescr_t descr) {
  typedescr_t *new_descriptors;
  int          newsz;
  int          cursz;
  typedescr_t *d;

  if (descr.type <= 0) {
    descr.type = data_numtypes;
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
  
  if ((datatype >= 0) && (datatype < data_numtypes)) {
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
    typedescr_register(typedescr_error);
    
    typedescr_register_methods(methoddescr_int);
    typedescr_register_methods(methoddescr_bool);
    typedescr_register_methods(methoddescr_float);
    typedescr_register_methods(methoddescr_str);
    typedescr_register_methods(methoddescr_ptr);
    typedescr_register_methods(methoddescr_fnc);

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
  debug("data_create(%d): '%s' [%d][%s]", type, descr -> typename, descr -> type, descr -> typecode);
  
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

data_t * data_create_list(list_t *list) {
  data_t *ret;
  list_t *l;

  ret = data_create(List);
  l = (list_t *) ret -> ptrval;
  list_add_all(l, list);
  return ret;
}

data_t * data_create_list_fromarray(array_t *array) {
  data_t *ret;
  list_t *l;
  int     ix;

  ret = data_create(List);
  l = (list_t *) ret -> ptrval;
  for (ix = 0; ix < array_size(array); ix++) {
    list_append(l, data_copy((data_t *) array_get(array, ix)));
  }
  return ret;
}

array_t * data_list_toarray(data_t *data) {
  listiterator_t *iter;
  array_t        *ret;

  ret = data_array_create(list_size((list_t *) data -> ptrval));
  for (iter = li_create((list_t *) data -> ptrval); li_has_next(iter); ) {
    array_push(ret, data_copy((data_t *) li_next(iter)));
  }
  return ret;
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

int data_is_error(data_t *data) {
  return (!data || (data -> type == Error));
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

  assert(method);
  assert(self);
  assert(args);
  type = data_typedescr(self);

  /* -1 because self is an argument as well */
  if ((method -> min_args == method -> max_args) &&
      (array_size(args) != (method -> max_args - 1))) {
    return data_error(ErrorArgCount, "%s.%s requires exactly %d arguments",
                      type -> typename, method -> name, method -> min_args);
  } else if (array_size(args) < (method -> min_args - 1)) {
    return data_error(ErrorArgCount, "%s.%s requires at least %d arguments",
                      type -> typename, method -> name, method -> min_args);
  } else if ((method -> max_args > 0) && (array_size(args) > (method -> max_args - 1))) {
    return data_error(ErrorArgCount, "%s.%s accepts at most %d arguments",
                      type -> typename, method -> name, method -> max_args);
  } else {
    return method -> method(self, method -> name, args, kwargs);
  }
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
    ret = data_error(ErrorName, "data object '%s' has no method '%s",
                     data_tostring(self), name);
  }
  array_free(args_shifted);
  return ret;
}

unsigned int data_hash(data_t *data) {
  typedescr_t *type = data_typedescr(data);

  return (type -> hash) ? type -> hash(data) : hashptr(data);
}

char * data_tostring(data_t *data) {
  char         buf[20];
  char        *ret;
  typedescr_t *type;

  if (!data) {
    ret = "data:NULL";
  }
  free(data -> str);
  type = data_typedescr(data);
  if (type -> tostring) {
    ret =  type -> tostring(data);
  } else {
    snprintf(buf, sizeof(buf), "data:%s:%p", descriptors[data -> type].typecode, data);
    ret = buf;
  }
  data -> str = strdup(ret);
  return data -> str;
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
  if (!d1 && !d2) {
    return 0;
  } else if (!d1 || !d2) {
    return 1;
  } else if (d1 -> type != d2 -> type) {
    return 1;
  } else {
    type = data_typedescr(d1);
    if (type -> cmp) {
      return type -> cmp(d1, d2);
    } else {
      return (d1 -> ptrval == d2 -> ptrval) ? 0 : 1;
    }
  }
}

dict_t * data_add_all_reducer(entry_t *e, dict_t *target) {
  dict_put(target, strdup(e -> key), data_copy(e -> value));
  return target;
}

