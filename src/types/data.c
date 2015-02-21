/*
 * /obelix/src/types/data.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <exception.h>
#include <list.h>
#include <object.h>

static void     _any_init(void) __attribute__((constructor));
static data_t * _any_cmp(data_t *, char *, array_t *, dict_t *);
static data_t * _any_hash(data_t *, char *, array_t *, dict_t *);

static void     _data_call_free(typedescr_t *, void *);

// -----------------------
// Data conversion support

static int          data_numtypes = 0;
static typedescr_t *descriptors = NULL;
static int          _data_count = 0;

static typedescr_t _typedescr_any = {
  .type =      Any,
  .type_name = "any"
};

static methoddescr_t _methoddescr_any[] = {
  { .type = Any,    .name = ">" ,   .method = _any_cmp,  .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "<" ,   .method = _any_cmp,  .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = ">=" ,  .method = _any_cmp,  .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "<=" ,  .method = _any_cmp,  .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "==" ,  .method = _any_cmp,  .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "!=" ,  .method = _any_cmp,  .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "hash", .method = _any_hash, .argtypes = { Any, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,   .method = NULL,      .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static code_label_t _function_id_labels[] = {
  { .code = FunctionNone,     .label = "None" },
  { .code = FunctionNew,      .label = "New" },
  { .code = FunctionCopy,     .label = "Copy" },
  { .code = FunctionCmp,      .label = "Cmp" },
  { .code = FunctionFree,     .label = "Free" },
  { .code = FunctionToString, .label = "ToString" },
  { .code = FunctionFltValue, .label = "FltValue" },
  { .code = FunctionIntValue, .label = "IntValue" },
  { .code = FunctionParse,    .label = "Parse" },
  { .code = FunctionCast,     .label = "Cast" },
  { .code = FunctionHash,     .label = "Hash" },
  { .code = FunctionLen,      .label = "Len" },
  { .code = FunctionResolve,  .label = "Resolve" },
  { .code = FunctionCall,     .label = "Call" },
  { .code = FunctionSet,      .label = "Set" },
  { .code = FunctionRead,     .label = "Read" },
  { .code = FunctionWrite,    .label = "Write" },
  { .code = FunctionOpen,     .label = "Open" },
  { .code = FunctionIter,     .label = "Iter" },
  { .code = FunctionNext,     .label = "Next" },
  { .code = FunctionEndOfListDummy, .label = "End" },
  { .code = -1,             .label = NULL }
};

/*
 * 'any' global methods
 */

void _any_init(void) {
  typedescr_register(&_typedescr_any);
  typedescr_register_methods(_methoddescr_any);
}

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
  return ret;
}

data_t * _any_hash(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, data_hash(self));
}

/*
 * typedescr_t public functions
 */

int typedescr_register(typedescr_t *descr) {
  typedescr_t    *new_descriptors;
  vtable_t       *vtable;
  int             newsz;
  int             cursz;
  typedescr_t    *d;

  if (descr -> type < 0) {
    descr -> type = data_numtypes;
  }
  //debug("Registering type '%s'", descr -> type_name);
  assert((descr -> type >= data_numtypes) || descriptors[descr -> type].type == 0);
  if (descr -> type >= data_numtypes) {
    newsz = (descr -> type + 1) * sizeof(typedescr_t);
    cursz = data_numtypes * sizeof(typedescr_t);
    new_descriptors = (typedescr_t *) resize_block(descriptors, newsz, cursz);
    descriptors = new_descriptors;
    data_numtypes = descr -> type + 1;
  }
  d = &descriptors[descr -> type];
  memcpy(d, descr, sizeof(typedescr_t));
  vtable = d -> vtable;
  d -> vtable = NULL;
  typedescr_register_functions(d, vtable);
  d -> str = NULL;
  return d -> type;
}

typedescr_t * typedescr_register_functions(typedescr_t *type, vtable_t vtable[]) {
  int ix;
  int fnc_id;

  if (!type -> vtable) {
    type -> vtable = (vtable_t *) new((FunctionEndOfListDummy + 1) * sizeof(vtable_t));
    for (ix = 0; ix <= FunctionEndOfListDummy; ix++) {
      type -> vtable[ix].id = ix;
      type -> vtable[ix].fnc = NULL;
    }
  }
  if (vtable) {
    for (ix = 0; vtable[ix].fnc; ix++) {
      fnc_id = vtable[ix].id;
      type -> vtable[fnc_id].id = fnc_id;
      type -> vtable[fnc_id].fnc = vtable[ix].fnc;
    }
  }
  return type;
}

typedescr_t * typedescr_get(int datatype) {
  typedescr_t *ret = NULL;
  
  if (datatype == Any) {
    return &_typedescr_any;
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

unsigned int typedescr_hash(typedescr_t *type) {
  if (!type -> hash) {
    type -> hash = strhash(type -> type_name);
  }
  return type -> hash;
}

void typedescr_dump_vtable(typedescr_t *type) {
  int ix;
  
  debug("vtable for %s", typedescr_tostring(type));
  for (ix = 0; ix < FunctionEndOfListDummy; ix++) {
    assert(ix == type -> vtable[ix].id);
    debug("%-20.20s %d %p", label_for_code(_function_id_labels, ix), ix, type -> vtable[ix].fnc);
  }
}

void typedescr_count(void) {
  int ix;
  
  debug("Atom count");
  debug("-------------------------------------------------------");
  for (ix = 0; ix < data_numtypes; ix++) {
    if (descriptors[ix].type > NoType) {
      debug("%3d. %-20.20s %6d", descriptors[ix].type, descriptors[ix].type_name, descriptors[ix].count);
    }
  }
  debug("-------------------------------------------------------");
  debug("     %-20.20s %6d", "T O T A L", _data_count);
}

void_t typedescr_get_function(typedescr_t *type, int fnc_id) {
  void_t       ret;
  int          ix;
  typedescr_t *inherits;
  
  assert(type);
  assert(fnc_id > FunctionNone);
  assert(fnc_id < FunctionEndOfListDummy);
  ret = type -> vtable[fnc_id].fnc;
  for (ix = 0; !ret && ix < type -> inherits_size; ix++) {
    inherits = typedescr_get(type -> inherits[ix]);
    ret = typedescr_get_function(inherits, fnc_id);
  }
  return ret;
}

void typedescr_register_methods(methoddescr_t methods[]) {
  methoddescr_t *method;
  typedescr_t   *type;

  for (method = methods; method -> type > NoType; method++) {
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
  methoddescr_t *ret;
  int            ix;
  
  assert(descr);
  if (!descr -> methods) {
    descr -> methods = strvoid_dict_create();
  }
  ret =  (methoddescr_t *) dict_get(descr -> methods, name);
  if (!ret) {
    if (descr -> inherits_size) {
      for (ix = 0; !ret && ix < descr -> inherits_size; ix++) {
        ret = typedescr_get_method(typedescr_get(descr -> inherits[ix]), name);
      }
    }
    if (!ret && (descr -> type != Any)) {
      ret = typedescr_get_method(typedescr_get(Any), name);
    }
    if (ret) {
      dict_put(descr -> methods, name, ret);
    }
  }
  return ret;
}

char * typedescr_tostring(typedescr_t *descr) {
  if (!descr -> str) {
    if (asprintf(&(descr -> str), "'%s' [%d]", 
                 descr -> type_name, descr -> type) < 0) {
      descr -> str = "Out of Memory?";
    }
  }
  return descr -> str;
}

int typedescr_is(typedescr_t *descr, int type) {
  int ix;
  int ret;
  
  if (type == descr -> type) {
    return 1;
  } else if (descr -> inherits_size) {
    ret = 0;
    for (ix = 0; !ret && ix < descr -> inherits_size; ix++) {
      ret = typedescr_is(typedescr_get(descr -> inherits[ix]), type);
    }
    return ret;
  } else {
    return 0;
  }
}

/*
 * data_t public functions
 */

data_t * data_create(int type, ...) {
  va_list      arg;
  data_t      *ret;
  typedescr_t *descr = typedescr_get(type);
  new_t        n;
  
  ret = NEW(data_t);
  ret -> type = type;
  ret -> refs = 1;
  ret -> str = NULL;
  descr -> count++;
  _data_count++;
#ifndef NDEBUG
  ret -> debugstr = NULL;
#endif
  ret -> methods = NULL;
  
  n = (new_t) typedescr_get_function(descr, FunctionNew);
  if (n) {
    va_start(arg, type);
    ret = n(ret, arg);
    va_end(arg);
  }
  return ret;
}

data_t * data_parse(int type, char *str) {
  typedescr_t *descr;
  parse_t      parse;

  descr = typedescr_get(type);
  parse = (parse_t) typedescr_get_function(descr, FunctionParse);
  return (parse) ? parse(str) : NULL;
}

data_t * data_cast(data_t *data, int totype) {
  typedescr_t *descr;
  typedescr_t *totype_descr;
  cast_t       cast;
  tostring_t   tostring;
  parse_t      parse;

  assert(data);
  if (data_type(data) == totype) {
    return data_copy(data);
  } else {
    descr = typedescr_get(data_type(data));
    assert(descr);
    totype_descr = typedescr_get(totype);
    assert(totype_descr);
    if (totype == String) {
      tostring = (tostring_t) typedescr_get_function(descr, FunctionToString);
      if (tostring) {
        return data_create(String, tostring(data));
      }
    }
    if (data_type(data) == String) {
      parse = (parse_t) typedescr_get_function(totype_descr, FunctionParse);
      if (parse) {
        return parse(data -> ptrval);
      }
    }
    cast = (cast_t) typedescr_get_function(descr, FunctionCast);
    if (cast) {
      return cast(data, totype);
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

void _data_call_free(typedescr_t *type, void *ptr) {
  free_t f;
  int    ix;
  
  f = (free_t) typedescr_get_function(type, FunctionFree);
  if (f) {
    f(ptr);
  }
  for (ix = 0; ix < type -> inherits_size; ix++) {
    _data_call_free(typedescr_get(type -> inherits[ix]), ptr);
  }
}

void data_free(data_t *data) {
  typedescr_t *type;
  
  if (data) {
    data -> refs--;
    if (data -> refs <= 0) {
      type = data_typedescr(data);
      _data_call_free(type, data -> ptrval);
      dict_free(data -> methods);
      free(data -> str);
#ifndef NDEBUG
      free(data -> debugstr);
#endif
      type -> count--;
      _data_count--;
      free(data);
    }
  }
}

int data_type(data_t *data) {
  return data -> type;
}

typedescr_t * data_typedescr(data_t *data) {
  return (data) ? typedescr_get(data_type(data)) : NULL;
}

void_t data_get_function(data_t *data, int fnc_id) {
  return typedescr_get_function(data_typedescr(data), fnc_id);
}

int data_is_numeric(data_t *data) {
  return data && typedescr_is(data_typedescr(data), Number);
}

int data_is_callable(data_t *data) {
  typedescr_t *td;
  
  if (data) {
    td = data_typedescr(data);
    assert(td);
    return (typedescr_get_function(td, FunctionCall) != NULL);
  } else {
    return 0;
  }
}

int data_hastype(data_t *data, int type) {
  if (type == Any) {
    return TRUE;
  } else {
    return data && typedescr_is(data_typedescr(data), type);
  }
}

data_t * data_copy(data_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
}

data_t * data_call(data_t *self, array_t *args, dict_t *kwargs) {
  call_t call = (call_t) data_get_function(self, FunctionCall);
  
  assert(data_is_callable(self));
  call(self, args, kwargs);
}

data_t * data_method(data_t *data, char *name) {
  typedescr_t   *type = data_typedescr(data);
  methoddescr_t *md;
  data_t        *ret = NULL;
  
  md = typedescr_get_method(type, name);
  if (md) {
    if (!data -> methods) {
      data -> methods = strdata_dict_create();
    } else {
      ret = (data_t *) dict_get(data -> methods, name);
    }
    if (!ret) {
      ret = data_create(Method, md, data);
      dict_put(data -> methods, strdup(name), ret);
    }
    ret = data_copy(ret);
  }
  return ret;
}

data_t * data_resolve(data_t *data, name_t *name) {
  typedescr_t    *type = data_typedescr(data);
  data_t         *ret = NULL;
  data_t         *tail_resolve;
  resolve_name_t  resolve;
  name_t         *tail;
  
  assert(type);
  assert(name);
  if (name_size(name) == 0) {
    ret = data_copy(data);
  } else if (name_size(name) == 1) {
    ret = data_method(data, name_first(name));
  }
  if (!ret) {
    resolve = (resolve_name_t) typedescr_get_function(type, FunctionResolve);
    if (!resolve) {
      ret = data_error(ErrorType,
                       "Cannot resolve name '%s' in %s '%s'", 
                       name_tostring(name),
                       type -> type_name, 
                       data_tostring(data));
    } else {
      ret = resolve(data, name_first(name));
    }
  }
  if (ret && !data_is_error(ret) && (name_size(name) > 1)) {
    tail = name_tail(name);
    tail_resolve = data_resolve(ret, tail);
    data_free(ret);
    ret = tail_resolve;
    name_free(tail);
  }
  return ret;
}

data_t * data_invoke(data_t *self, name_t *name, array_t *args, dict_t *kwargs) {
  data_t  *callable;
  data_t  *ret = NULL;
  array_t *args_shifted = NULL;
  
  if (!self && array_size(args)) {
    self = (data_t *) array_get(args, 0);
    args_shifted = array_slice(args, 1, -1);
    if (!args_shifted) {
      args_shifted = data_array_create(1);
    }
    args = args_shifted;
  }
  if (!self) {
    ret = data_error(ErrorArgCount, "No 'self' object specified for method '%s'", name);
  }

  if (!ret) {
    callable = data_resolve(self, name);
    if (data_is_error(callable)) {
      ret = callable;
    } else if (callable) {
      if (data_is_callable(callable)) {
        ret = data_call(callable, args, kwargs);
      } else {
        ret = data_error(ErrorNotCallable, 
                         "Atom '%s' is not callable",
                         data_tostring(callable));
      }
      data_free(callable);
    } else {
      ret = data_error(ErrorName, "Could not resolve '%s' in '%s'", 
                       name_tostring(name), data_tostring(self));
    }
  }
  if (args_shifted) {
    array_free(args_shifted);
  }
  return ret;
}

data_t * data_execute(data_t *data, char *name, array_t *args, dict_t *kwargs) {
  name_t *n = name_create(1, name);
  data_t *ret;
  
  ret = data_invoke(data, n, args, kwargs);
  name_free(n);
  return ret;
}

data_t * data_get(data_t *data, name_t *name) {
  data_t *ret;
  
  ret = data_resolve(data, name);
  if (!ret) {
    ret = data_error(ErrorName, "Could not resolve '%s' in '%s'", 
                     name_tostring(name), data_tostring(data));
  }
  return ret;
}

data_t * data_set(data_t *data, name_t *name, data_t *value) {
  typedescr_t *type;
  data_t      *container;
  name_t      *head;
  data_t      *ret;
  setvalue_t   setter;

  if (name_size(name) == 1) {
    container = data;
  } else {
    assert(name_size(name) > 1);
    head = name_head(name);
    container = data_resolve(data, head);
    name_free(head);
  }
  if (data_is_error(container)) {
    ret = container;
  } else if (container) {
    type = data_typedescr(container);
    setter = (setvalue_t) typedescr_get_function(type, FunctionSet);
    if (!setter) {
      ret = data_error(ErrorType,
                       "Cannot set values on objects of type '%s'", 
                       typedescr_tostring(type));
    } else {
      ret = setter(container, name_last(name), value);
    }
  } else {
    ret = data_error(ErrorName, "Could not resolve '%s' in '%s'", 
                      name_tostring(name), data_tostring(data));
  }
  return ret;
}

unsigned int data_hash(data_t *data) {
  typedescr_t *type = data_typedescr(data);
  hash_t       hash = (hash_t) typedescr_get_function(type, FunctionHash);

  return (hash) ? hash(data) : hashptr(data);
}

char * data_tostring(data_t *data) {
  int          len;
  char        *ret;
  typedescr_t *type;
  tostring_t   tostring;

  if (!data) {
    return "data:NULL";
  } else {
    free(data -> str);
    data -> str = NULL;
    type = data_typedescr(data);
    tostring = (tostring_t) typedescr_get_function(type, FunctionToString);
    if (tostring) {
      ret =  tostring(data);
      if (ret && !data -> str) {
	data -> str = strdup(ret);
      }
    } else {
      len = asprintf(&(data -> str), "data:%s:%p", descriptors[data -> type].type_name, data);
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
  asprintf(&(data -> debugstr), "%3.3s %s", 
	  type -> type_name, 
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
  cmp_t        cmp;
  
  if (!d1 && !d2) {
    return 0;
  } else if (!d1 || !d2) {
    return (!d1) ? -1 : 1;
  } else if (d1 -> type != d2 -> type) {
    p1 = data_promote(d1);
    if (p1 && (p1 -> type == d2 -> type)) {
      ret = data_cmp(p1, d2);
    } else {
      p2 = data_promote(d2);
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
    cmp = (cmp_t) typedescr_get_function(type, FunctionCmp);
    return (cmp) ? cmp(d1, d2) : ((d1 -> ptrval == d2 -> ptrval) ? 0 : 1);
  }
}

int data_count(void) {
  return _data_count;
}

/*
 * Standard reducer functions
 */

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
