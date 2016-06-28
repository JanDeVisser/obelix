/*
 * /obelix/src/lib/typedescr.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <config.h>
#include <stdio.h>
#include <string.h>

#include <core.h>
#include <exception.h>
#include <logging.h>
#include <str.h>
#include <typedescr.h>

static int          _numtypes = 0;
static typedescr_t *descriptors = NULL;
static interface_t *_interfaces = NULL;
static int          _next_interface = NextInterface;
static int          _num_interfaces = 0;

extern int          _data_count;
       int          type_debug = 0;

extern void           any_init(void);
static inline void    _typedescr_init(void);
static data_t *       _add_method_reducer(methoddescr_t *, data_t *);

static char *         _methoddescr_tostring(methoddescr_t *);
static int            _methoddescr_cmp(methoddescr_t *, methoddescr_t *);
static data_t *       _methoddescr_resolve(methoddescr_t *, char *);
static unsigned int   _methoddescr_hash(methoddescr_t *);

static int            _typedescr_cmp(typedescr_t *, typedescr_t *);
static char *         _typedescr_tostring(typedescr_t *);
static data_t *       _typedescr_resolve(typedescr_t *, char *);

static data_t *       _typedescr_gettype(data_t *, char *, array_t *, dict_t *);
static data_t *       _typedescr_hastype(data_t *, char *, array_t *, dict_t *);

static char *         _interface_tostring(interface_t *);
static int            _interface_cmp(interface_t *, interface_t *);
static data_t *       _interface_resolve(interface_t *, char *);
static unsigned int   _interface_hash(interface_t *);

static data_t *       _interface_isimplementedby(interface_t *, char *, array_t *, dict_t *);
static data_t *       _interface_implements(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_methoddescr[] = {
  { .id = FunctionCmp,          .fnc = (void_t) _methoddescr_cmp },
  { .id = FunctionStaticString, .fnc = (void_t) _methoddescr_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _methoddescr_resolve },
  { .id = FunctionHash,         .fnc = (void_t) _methoddescr_hash },
  { .id = FunctionNone,         .fnc = NULL }
};

static vtable_t _vtable_typedescr[] = {
  { .id = FunctionCmp,          .fnc = (void_t) _typedescr_cmp },
  { .id = FunctionStaticString, .fnc = (void_t) _typedescr_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _typedescr_resolve },
  { .id = FunctionHash,         .fnc = (void_t) typedescr_hash },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methoddescr_typedescr[] = {
  { .type = Any,    .name = "gettype",     .method = _typedescr_gettype,    .argtypes = { Any,    NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "hastype",     .method = _typedescr_hastype,    .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,          .method = NULL,                  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static vtable_t _vtable_interface[] = {
  { .id = FunctionCmp,          .fnc = (void_t) _interface_cmp },
  { .id = FunctionStaticString, .fnc = (void_t) _interface_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _interface_resolve },
  { .id = FunctionHash,         .fnc = (void_t) _interface_hash },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methoddescr_interface[] = {
  { .type = Interface,    .name = "isimplementedby", .method = (method_t) _interface_isimplementedby, .argtypes = { Any, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Any,          .name = "implements",      .method = (method_t) _interface_implements, .argtypes = { Interface, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL, .method = NULL, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static code_label_t _function_id_labels[] = {
  { .code = FunctionNone,           .label = "None" },
  { .code = FunctionFactory,        .label = "Factory" },
  { .code = FunctionNew,            .label = "New" },
  { .code = FunctionCopy,           .label = "Copy" },
  { .code = FunctionCmp,            .label = "Cmp" },
  { .code = FunctionFreeData,       .label = "FreeData" },
  { .code = FunctionFree,           .label = "Free" },
  { .code = FunctionToString,       .label = "ToString" },
  { .code = FunctionFltValue,       .label = "FltValue" },
  { .code = FunctionIntValue,       .label = "IntValue" },
  { .code = FunctionParse,          .label = "Parse" },
  { .code = FunctionCast,           .label = "Cast" },
  { .code = FunctionHash,           .label = "Hash" },
  { .code = FunctionLen,            .label = "Len" },
  { .code = FunctionResolve,        .label = "Resolve" },
  { .code = FunctionCall,           .label = "Call" },
  { .code = FunctionSet,            .label = "Set" },
  { .code = FunctionRead,           .label = "Read" },
  { .code = FunctionWrite,          .label = "Write" },
  { .code = FunctionOpen,           .label = "Open" },
  { .code = FunctionIter,           .label = "Iter" },
  { .code = FunctionNext,           .label = "Next" },
  { .code = FunctionHasNext,        .label = "HasNext" },
  { .code = FunctionDecr,           .label = "Decr" },
  { .code = FunctionIncr,           .label = "Incr" },
  { .code = FunctionVisit,          .label = "Visit" },
  { .code = FunctionReduce,         .label = "Reduce" },
  { .code = FunctionIs,             .label = "Is" },
  { .code = FunctionConstructor,    .label = "Constructor" },
  { .code = FunctionInterpolate,    .label = "Interpolate" },
  { .code = FunctionEndOfListDummy, .label = "End" },
  { .code = -1,                     .label = NULL }
};

/* ------------------------------------------------------------------------ */

void _typedescr_init(void) {
  if (!descriptors && !_interfaces) {
    logging_register_category("type", &type_debug);
    interface_register(Any,           "any",           0);
    interface_register(Callable,      "callable",      1, FunctionCall);
    interface_register(InputStream,   "inputstream",   1, FunctionRead);
    interface_register(OutputStream,  "outputstream",  1, FunctionWrite);
    interface_register(Iterable,      "iterable",      1, FunctionIter);
    interface_register(Iterator,      "iterator",      2, FunctionNext, FunctionHasNext);
    interface_register(Connector,     "connector",     1, FunctionQuery);
    interface_register(CtxHandler,    "ctxhandler",    2, FunctionEnter, FunctionLeave);
    interface_register(Incrementable, "incrementable", 2, FunctionIncr, FunctionDecr);
    
    typedescr_create_and_register(Type, "type",
				  _vtable_typedescr, _methoddescr_typedescr);
    typedescr_create_and_register(Interface, "interface",
				  _vtable_interface, /* _methoddescr_interface */ NULL);
    typedescr_create_and_register(Method, "method",
				  _vtable_methoddescr, /* _methoddescr_method */ NULL);
    any_init();
  }
}

data_t * type_get(int ix) {
  return (ix < FirstInterface)
    ? (data_t *) typedescr_get(ix)
    : (data_t *) interface_get(ix);
}

data_t * type_get_byname(char *name) {
  data_t *ret;

  ret = (data_t *) typedescr_get_byname(name);
  if (!ret) {
    ret = (data_t *) interface_get_byname(name);
  }
  return ret;
}

/* -- G E N E R A L  S T A T I C  F U N C T I O N S ----------------------- */

data_t * _add_method_reducer(methoddescr_t *mth, data_t *methods) {
  data_list_push(methods, (data_t *) mth);
  return methods;
}


/* -- M E T H O D D E S C R   F U N C T I O N S --------------------------- */

char * _methoddescr_tostring(methoddescr_t *mth) {
  return mth -> name;
}

int _methoddescr_cmp(methoddescr_t *mth1, methoddescr_t *mth2) {
  return strcmp(mth1 -> name, mth2 -> name);
}

data_t * _methoddescr_resolve(methoddescr_t *mth, char *name) {
  data_t *ret;
  int     ix;
  
  if (!strcmp(name, "name")) {
    return str_to_data(mth -> name);
  } else if (!strcmp(name, "args")) {
    ret = data_create_list(NULL);
    for (ix = 0; ix < MAX_METHOD_PARAMS; ix++) {
      if (!mth -> argtypes[ix]) {
	break;
      }
      data_list_push(ret, type_get(mth -> argtypes[ix]));
    }
    return ret;
  } else if (!strcmp(name, "minargs")) {
    return int_to_data(mth -> minargs);
  } else if (!strcmp(name, "varargs")) {
    return int_as_bool(mth -> varargs);
  } else {
    return NULL;
  }
}

unsigned int _methoddescr_hash(methoddescr_t *mth) {
  return strhash(mth -> name);
}


/* -- I N T E R F A C E   F U N C T I O N S ------------------------------- */

int interface_register(int type, char *name, int numfncs, ...) {
  int          ix;
  int          iix;
  int          cursz;
  int          newsz;
  interface_t *new_interfaces;
  interface_t *iface;
  va_list      fncs;
  int          fnc_id;

  if (type != Any) {
    _typedescr_init();
  }
  if (type < FirstInterface) {
    type = _next_interface++;
  }
  ix = type - FirstInterface - 1;
  if (type_debug) {
    info("Registering interface '%s' [%d:%d]", name, type, ix);
  }
  if (ix >= _num_interfaces) {
    newsz = (ix + 1) * sizeof(interface_t);
    cursz = _num_interfaces * sizeof(interface_t);
    new_interfaces = (interface_t *) resize_block(_interfaces, newsz, cursz);
    _interfaces = new_interfaces;
    _num_interfaces = ix + 1;
  }
  iface = &_interfaces[ix];
  iface -> _d.type = Interface;
  iface -> _d.free_me = Constant;
  iface -> _d.refs = 1;
  iface -> _d.str = NULL;
  iface -> type = type;
  iface -> name = strdup(name);
  iface -> methods = NULL;
  iface -> fncs = (int *) new_array(numfncs + 1, sizeof(int));
  va_start(fncs, numfncs);
  for (iix = 0; iix < numfncs; iix++) {
    fnc_id = va_arg(fncs, int);
    iface -> fncs[iix] = fnc_id;
  }
  va_end(fncs);
  return type;
}

interface_t * interface_get(int type) {
  int          ifix = type - FirstInterface - 1;
  interface_t *ret = NULL;

  _typedescr_init();
  if ((type > FirstInterface) && (type < _next_interface)) {
    ret = &_interfaces[ifix];
    if (!ret) {
      if (type_debug) {
	error("Undefined interface type %d referenced. Expect crashes", type);
      }
      return NULL;
    } else {
      if (ret -> type != type) {
        debug("looking for type %d, found %d (%s)", type, ret -> type, ret -> name);
      }
      assert(ret -> type == type);
    }
  } else {
    if (type_debug) {
      error("Type %d referenced as interface, but it cannot be. Expect crashes", type);
    }
  }
  return ret;
}

interface_t * interface_get_byname(char *name) {
  int          ifix;

  _typedescr_init();
  for (ifix = 0; ifix < _next_interface - FirstInterface - 1; ifix++) {
    if (!strcmp(_interfaces[ifix].name, name)) {
      return &_interfaces[ifix];
    }
  }
  return NULL;
}

void interface_register_method(interface_t *iface, methoddescr_t *method) {
  if (type_debug) {
    info("interface_register_method(%s, %s)", iface -> name, method -> name);
  }
  if (!iface -> methods) {
    iface -> methods = strdata_dict_create();
  }
  method -> _d.type = Method;
  method -> _d.free_me = Constant;
  method -> _d.refs = 1;
  method -> _d.str = NULL;
  dict_put(iface -> methods, method -> name, method);
}

methoddescr_t * interface_get_method(interface_t *iface, char *name) {
  return (iface -> methods)
    ? (methoddescr_t *) dict_get(iface -> methods, name)
    : NULL;
}

char * _interface_tostring(interface_t *iface) {
  return iface -> name;
}

int _interface_cmp(interface_t *iface1, interface_t *iface2) {
  return iface1 -> type - iface2 -> type;
}

data_t * _interface_resolve(interface_t *iface, char *name) {
  data_t      *ret;
  int          ix;
  typedescr_t *type;
  
  if (!strcmp(name, "name")) {
    return str_to_data(iface -> name);
  } else if (!strcmp(name, "id")) {
    return int_to_data(iface -> type);
  } else if (!strcmp(name, "methods")) {
    ret = data_create_list(NULL);
    dict_reduce_values(iface -> methods,
		       (reduce_t) _add_method_reducer,
		       ret);
    return ret;
  } else if (!strcmp(name, "implementations")) {
    ret = data_create_list(NULL);
    for (ix = 0; ix < _numtypes; ix++) {
      type = &descriptors[ix];
      if (type && typedescr_is(type, iface -> type)) {
	data_list_push(ret, (data_t *) type);
      }
    }
    return ret;
  } else {
    return NULL;
  }
}

unsigned int _interface_hash(interface_t *iface) {
  return strhash(iface -> name);
}

data_t * _interface_isimplementedby(interface_t *iface, char *name, array_t *args, dict_t *kwargs) {
  data_t      *data = data_array_get(args, 0);
  typedescr_t *type;
  
  (void) name;
  (void) kwargs;
  if (data_hastype(data, Interface)) {
    return data_false();
  } else {
    type = (data_hastype(data, Type))
      ? (typedescr_t *) data
      : data_typedescr(data);
    return int_as_bool(typedescr_is(type, iface -> type));   
  }
}

data_t * _interface_implements(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  array_t     *a = data_array_create(1);
  interface_t *iface = (interface_t *) array_get(args, 0);
  data_t      *ret;

  array_push(a, data_copy(self));
  ret = _interface_isimplementedby(iface, NULL, a, NULL);
  array_free(a);
  return ret;
}

/* -- V T A B L E  F U N C T I O N S -------------------------------------- */

void vtable_dump(vtable_t *vtable) {
  int ix;

  for (ix = 0; ix < FunctionEndOfListDummy; ix++) {
    assert(ix == vtable[ix].id);
    if (vtable[ix].fnc) {
      debug("%-20.20s %d %p",
            label_for_code(_function_id_labels, ix),
            ix, vtable[ix].fnc);
    }
  }
}

void_t vtable_get(vtable_t *vtable, int fnc_id) {
  assert(fnc_id > FunctionNone);
  assert(fnc_id < FunctionEndOfListDummy);
  return vtable[fnc_id].fnc;
}

vtable_t * vtable_build(vtable_t vtable[]) {
  int       ix;
  int       fnc_id;
  vtable_t *ret;

  ret = (vtable_t *) new((FunctionEndOfListDummy + 1) * sizeof(vtable_t));
  for (ix = 0; ix <= FunctionEndOfListDummy; ix++) {
    ret[ix].id = ix;
    ret[ix].fnc = NULL;
  }
  if (vtable) {
    for (ix = 0; vtable[ix].fnc; ix++) {
      fnc_id = vtable[ix].id;
      ret[fnc_id].id = fnc_id;
      ret[fnc_id].fnc = vtable[ix].fnc;
    }
  }
  return ret;
}

int vtable_implements(vtable_t *vtable, int type) {
  int          ret;
  interface_t *iface;
  int          ix;
  int          fnc_id;

  iface = interface_get(type);
  if (!iface) {
    return FALSE;
  }
  ret = TRUE;
  for (ix = 0; ret && iface -> fncs[ix]; ix++) {
    fnc_id = iface -> fncs[ix];
    ret &= (vtable[fnc_id].fnc != NULL);
  }
  return ret;
}

/* -- T Y P E D E S C R  S T A T I C  F U N C T I O N S ------------------- */

int _typedescr_cmp(typedescr_t *type1, typedescr_t *type2) {
  return type1 -> type - type2 -> type;
}


char * _typedescr_tostring(typedescr_t *descr) {
  char *str;
  
  if (asprintf(&str, "'%s' [%d]",
	       descr -> type_name, descr -> type) < 0) {
    str = "Out of Memory?";
  }
  return str;
}

data_t * _typedescr_resolve(typedescr_t *descr, char *name) {
  data_t      *ret;
  int          ix;
  interface_t *iface;

  if (!strcmp(name, "name")) {
    return str_to_data(descr -> type_name);
  } else if (!strcmp(name, "id")) {
    return int_to_data(descr -> type);
  } else if (!strcmp(name, "inherits")) {
    ret = data_create_list(NULL);
    for (ix = 0; ix < MAX_INHERITS; ix++) {
      if (descr -> inherits[ix] < 0) {
	      break;
      }
      data_list_push(ret, (data_t *) typedescr_get(descr -> inherits[ix]));
    }
    return ret;
  } else if (!strcmp(name, "methods")) {
    ret = data_create_list(NULL);
    dict_reduce_values(descr -> methods,
		       (reduce_t) _add_method_reducer,
		       ret);
    return ret;
  } else if (!strcmp(name, "implements")) {
    ret = data_create_list(NULL);
    for (ix = FirstInterface + 1; ix < _next_interface; ix++) {
      iface = &_interfaces[ix - FirstInterface - 1];
      if (iface && typedescr_is(descr, iface -> type)) {
      	data_list_push(ret, (data_t *) iface);
      }
    }
    return ret;
  } else {
    return NULL;
  }
}

data_t * _typedescr_gettype(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t      *t = data_array_get(args, 0);
  typedescr_t *type;
  
  (void) self;
  (void) name;
  (void) kwargs;

  type = (data_is_int(t))
    ? typedescr_get(data_intval(t))
    : typedescr_get_byname(data_tostring(t));
  return (type)
    ? (data_t *) type
    : (data_t *) data_exception(ErrorParameterValue,
		     "Type '%s' not found", data_tostring(t));
}

data_t * _typedescr_hastype(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  typedescr_t *type = (typedescr_t *) data_array_get(args, 0);
  
  (void) name;
  (void) kwargs;
  return data_hastype(self, type -> type) ? data_true() : data_false();
}

/* -- T Y P E D E S C R  P U B L I C  F U N C T I O N S ------------------- */

int typedescr_register(typedescr_t *descr) {
  typedescr_t    *new_descriptors;
  vtable_t       *vtable;
  int             newsz;
  int             cursz;
  typedescr_t    *d;

  _typedescr_init();
  if (descr -> type < 0) {
    descr -> type = (_numtypes > Dynamic) ? _numtypes : Dynamic;
  }
  if (type_debug) {
    info("Registering type '%s' [%d]", descr -> type_name, descr -> type);
  }
  if (!descriptors) {
    descriptors = (typedescr_t *) new_array(Dynamic + 1, sizeof(typedescr_t));
    _numtypes = Dynamic;
  } else {
    if (descr -> type >= _numtypes) {
      newsz = (descr -> type + 1) * sizeof(typedescr_t);
      cursz = _numtypes * sizeof(typedescr_t);
      new_descriptors = (typedescr_t *) resize_block(descriptors, newsz, cursz);
      descriptors = new_descriptors;
      _numtypes = descr -> type + 1;
    }
  }
  d = &descriptors[descr -> type];
  memcpy(d, descr, sizeof(typedescr_t));
  d -> _d.type = Type;
  d -> _d.free_me = Constant;
  d -> _d.refs = 1;
  d -> _d.str = NULL;
  vtable = d -> vtable;
  d -> vtable = NULL;
  typedescr_register_functions(d, vtable);
  d -> methods = NULL;
  d -> hash = 0;
  d -> constructors = NULL;
  return d -> type;
}

int typedescr_register_type(typedescr_t *td, methoddescr_t *md) {
  int ret = typedescr_register(td);
  int i, j;

  if (md) {
    for (i = 0; md[i].type != NoType; i++) {
      if (md[i].type < 0) {
        md[i].type = ret;
      }
      for (j = 0; j < MAX_METHOD_PARAMS; j++) {
        if (md[i].argtypes[j] < 0) {
          md[j].argtypes[j] = ret;
        }
      }
    }
    typedescr_register_methods(md);
  }
  return ret;
}


void typedescr_register_types(typedescr_t *types) {
  typedescr_t *type;

  for (type = types; type -> type_name; type++) {
    typedescr_register(type);
  }
}

int typedescr_create_and_register(int type, char *type_name, vtable_t *vtable, methoddescr_t *methods) {
  typedescr_t td;
  int         ix;

  memset(&td, 0, sizeof(typedescr_t));
  td.type = type;
  td.type_name = type_name;
  td.vtable = vtable;
  for (ix = 0; ix < MAX_INHERITS; ix++) {
    td.inherits[ix] = NoType;
  }
  return typedescr_register_type(&td, methods);
}

typedescr_t * typedescr_register_functions(typedescr_t *type, vtable_t vtable[]) {
  if (type -> vtable) {
    free(type -> vtable);
  }
  type -> vtable = vtable_build(vtable);
  return type;
}

typedescr_t * typedescr_assign_inheritance(typedescr_t *type, int inherits) {
  int ix;
  
  for (ix = 0; ix < MAX_INHERITS; ix++) {
    if (type -> inherits[ix] == NoType) {
      type -> inherits[ix] = inherits;
      break;
    } else if (type -> inherits[ix] == inherits) {
      break;
    }
  }
  return type;
}

typedescr_t * typedescr_get(int datatype) {
  typedescr_t *ret = NULL;

  _typedescr_init();
  if ((datatype >= 0) && (datatype < _numtypes)) {
    ret = &descriptors[datatype];
  }
  if (!ret) {
    fatal("Undefined type %d referenced. Terminating...", datatype);
    return NULL;
  } else {
    //if (type_debug) {
    //  debug("typedescr_get(%d) = %s", datatype, ret -> type_name);
    //}
    return ret;
  }
}

typedescr_t * typedescr_get_byname(char *name) {
  typedescr_t *ret = NULL;
  int          ix;

  _typedescr_init();
  for (ix = 0; ix < _numtypes; ix++) {
    if (descriptors[ix].type_name && !strcmp(name, descriptors[ix].type_name)) {
      return &descriptors[ix];
    }
  }
  if (type_debug) {
    debug("typedescr_get_byname(%s) = %d", name, (ret) ? ret -> type : -1);
  }
  return NULL;
}

unsigned int typedescr_hash(typedescr_t *type) {
  if (!type -> hash) {
    type -> hash = strhash(type -> type_name);
  }
  return type -> hash;
}

void typedescr_dump_vtable(typedescr_t *type) {
  debug("vtable for %s", typedescr_tostring(type));
  vtable_dump(type -> vtable);
}

void typedescr_count(void) {
  int ix;

  debug("Atom count");
  debug("-------------------------------------------------------");
  for (ix = 0; ix < _numtypes; ix++) {
    if (descriptors[ix].type > NoType) {
      debug("%3d. %-20.20s %6d", descriptors[ix].type, descriptors[ix].type_name, descriptors[ix].count);
    }
  }
  debug("-------------------------------------------------------");
  debug("     %-20.20s %6d", "T O T A L", _data_count);
}

void_t typedescr_get_local_function(typedescr_t *type, int fnc_id) {
  void_t ret = NULL;

  assert(type);
  assert(fnc_id > FunctionNone);
  assert(fnc_id < FunctionEndOfListDummy);
  if (type -> vtable) {
    ret = type -> vtable[fnc_id] . fnc;
  }
  return ret;
}

void_t typedescr_get_function(typedescr_t *type, int fnc_id) {
  void_t       ret = NULL;
  int          ix;
  typedescr_t *inherits;

  ret = typedescr_get_local_function(type, fnc_id);
  for (ix = 0; !ret && (ix < MAX_INHERITS) && type -> inherits[ix]; ix++) {
    inherits = typedescr_get(type -> inherits[ix]);
    ret = typedescr_get_function(inherits, fnc_id);
  }
  return ret;
}

list_t * typedescr_get_constructors(typedescr_t *type) {
  void_t  n;
  list_t *constructors;
  list_t *inherited;
  int     ix;

  if (!type -> constructors) {
    constructors = list_create();
    for (ix = 0; (ix < MAX_INHERITS) && type -> inherits[ix]; ix++) {
      inherited = typedescr_get_constructors(
        typedescr_get(type -> inherits[ix]));
      list_add_all(type -> constructors, inherited);
    }
    n = typedescr_get_local_function(type, FunctionNew);
    if (n) {
      list_push(type -> constructors, n);
    }
    if (list_empty(constructors)) {
      list_free(constructors);
      type -> constructors = EmptyList;
    } else {
      type -> constructors = constructors;
    }
  }
  return (type -> constructors != EmptyList) ? type -> constructors : NULL;
}

void typedescr_register_methods(methoddescr_t methods[]) {
  methoddescr_t *method;
  typedescr_t   *type;
  interface_t   *iface;

  for (method = methods; method -> type > NoType; method++) {
    if (method -> type < FirstInterface) {
      type = typedescr_get(method -> type);
      assert(type);
      typedescr_register_method(type, method);
    } else {
      iface = interface_get(method -> type);
      assert(iface);
      interface_register_method(iface, method);
    }
  }
}

void typedescr_register_method(typedescr_t *type, methoddescr_t *method) {
  if (type_debug) {
    info("typedescr_register_method(%s, %s)", type -> type_name, method -> name);
  }
  if (!type -> methods) {
    type -> methods = strdata_dict_create();
  }
  method -> _d.type = Method;
  method -> _d.free_me = Constant;
  method -> _d.refs = 1;
  method -> _d.str = NULL;
  dict_put(type -> methods, method -> name, method);
}

methoddescr_t * typedescr_get_method(typedescr_t *descr, char *name) {
  methoddescr_t *ret = NULL;
  int            ix;

  assert(descr);
  if (type_debug) {
    info("typedescr_get_method(%s, %s)", descr -> type_name, name);
  }
  if (descr -> methods) {
    ret = (methoddescr_t *) dict_get(descr -> methods, name);
  }
  if (!ret) {
    for (ix = 0; !ret && ix < _num_interfaces; ix++) {
      if (typedescr_is(descr, _interfaces[ix].type)) {
        ret = interface_get_method(&_interfaces[ix], name);
      }
    }
    for (ix = 0; !ret && (ix < MAX_INHERITS) && descr -> inherits[ix]; ix++) {
      ret = typedescr_get_method(typedescr_get(descr -> inherits[ix]), name);
    }
    if (ret) {
      typedescr_register_method(descr, ret);
    }
  }
  return ret;
}

int typedescr_is(typedescr_t *descr, int type) {
  int   ix;
  int   ret;
  int (*fnc)(typedescr_t *, int);

  //if (type_debug) {
  //  debug("'%s'.is(%d)", descr -> type_name, type);
  //}
  if ((type == descr -> type) || (type == Any)) {
    if (type_debug) {
      debug("'%s'.is(%d) = by definition", descr -> type_name, type);
    }
    ret = TRUE;
  } else {
    if (descr -> vtable[FunctionIs].fnc) {
      fnc = (int (*)(typedescr_t *, int)) descr -> vtable[FunctionIs].fnc;
      ret = fnc(descr, type);
      //if (type_debug) {
      //  debug("'%s'.is(%d) = %d by delegation", descr -> type_name, type, ret);
      //}
    } else {
      ret = (type > FirstInterface) && vtable_implements(descr -> vtable, type);
      //if (type_debug) {
      //  debug("'%s'.is(%d) = %d by implementation", descr -> type_name, type, ret);
      //}
    }
  }
  //if (type_debug) {
  //  debug("'%s'.is(%d) checking parents", descr -> type_name, type);
  //}
  for (ix = 0; !ret && (ix < MAX_INHERITS) && descr -> inherits[ix]; ix++) {
    ret = typedescr_is(typedescr_get(descr -> inherits[ix]), type);
  }
  //if (type_debug) {
  //  debug("Survey says: '%s'.is(%d) = %d", descr -> type_name, type, ret);
  //}
  return ret;
}
