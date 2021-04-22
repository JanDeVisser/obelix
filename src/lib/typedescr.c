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

#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <exception.h>
#include <str.h>

static size_t        _numtypes = Dynamic;
static size_t        _capacity = 0;
static typedescr_t **_descriptors = NULL;
static interface_t **_interfaces = NULL;
static int           _next_interface = NextInterface;
static size_t        _num_interfaces = 0;

extern int           _data_count;
       int           type_debug = 1;

extern void          any_init(void);
static datalist_t *  _add_method_reducer(methoddescr_t *, datalist_t *);

static char *          _methoddescr_tostring(methoddescr_t *);
static int             _methoddescr_cmp(methoddescr_t *, methoddescr_t *);
static data_t *        _methoddescr_resolve(methoddescr_t *, char *);
static unsigned int    _methoddescr_hash(methoddescr_t *);

static kind_t *        _kind_init(kind_t *, int, int, char *);
static int             _kind_cmp(kind_t *, kind_t *);
static char *          _kind_tostring(kind_t *);
static unsigned int    _kind_hash(kind_t *);
static data_t *        _kind_resolve(kind_t *, char *);
static ctx_wrapper_t * _kind_reduce_methods(methoddescr_t *, ctx_wrapper_t *);
static void *          _kind_reduce_children(kind_t *, reduce_t, void *);
static kind_t *        _kind_inherit_methods(kind_t *, kind_t *);
static kind_t *        _inherit_method_reducer(methoddescr_t *, kind_t *);

static data_t *        _interface_resolve(interface_t *, char *);
static data_t *        _interface_isimplementedby(interface_t *, char *, arguments_t *);
static data_t *        _interface_implements(data_t *, char *, arguments_t *);

static int *           _typedescr_get_all_interfaces(typedescr_t *);
static void_t *        _typedescr_get_constructors(typedescr_t *);
static typedescr_t *   _typedescr_initialize_vtable(typedescr_t *, vtable_t[]);

static data_t *        _typedescr_resolve(typedescr_t *, char *);
static data_t *        _typedescr_gettype(data_t *, char *, arguments_t *);
static data_t *        _typedescr_hastype(data_t *, char *, arguments_t *);

static vtable_t _vtable_Method[] = {
  { .id = FunctionCmp,          .fnc = (void_t) _methoddescr_cmp },
  { .id = FunctionStaticString, .fnc = (void_t) _methoddescr_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _methoddescr_resolve },
  { .id = FunctionHash,         .fnc = (void_t) _methoddescr_hash },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t * _methods_Method = NULL;

static vtable_t _vtable_Kind[] = {
  { .id = FunctionCmp,          .fnc = (void_t) _kind_cmp },
  { .id = FunctionStaticString, .fnc = (void_t) _kind_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _kind_resolve },
  { .id = FunctionHash,         .fnc = (void_t) _kind_hash },
  { .id = FunctionReduce,       .fnc = (void_t) _kind_reduce_children },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t * _methods_Kind = NULL;

static vtable_t _vtable_Type[] = {
  { .id = FunctionResolve,      .fnc = (void_t) _typedescr_resolve },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methods_Type[] = {
  { .type = Any,    .name = "gettype",     .method = _typedescr_gettype,    .argtypes = { Any,    NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = Any,    .name = "hastype",     .method = _typedescr_hastype,    .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,          .method = NULL,                  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static vtable_t _vtable_Interface[] = {
  { .id = FunctionResolve,      .fnc = (void_t) _interface_resolve },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methods_Interface[] = {
  { .type = Interface,    .name = "isimplementedby", .method = (method_t) _interface_isimplementedby, .argtypes = { Any, NoType, NoType },       .minargs = 1, .varargs = 0 },
  { .type = Any,          .name = "implements",      .method = (method_t) _interface_implements,      .argtypes = { Interface, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType,       .name = NULL,              .method = NULL,                                  .argtypes = { NoType, NoType, NoType },    .minargs = 0, .varargs = 0 }
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

void typedescr_init(void) {
  if (!_descriptors || !_interfaces) {
    logging_register_module(type);
  }
  if (!_interfaces) {
    builtin_interface_register(Any, 0);
    builtin_interface_register(Callable, 1, FunctionCall);
    builtin_interface_register(InputStream, 1, FunctionRead);
    builtin_interface_register(OutputStream, 1, FunctionWrite);
    builtin_interface_register(Iterable, 1, FunctionIter);
    builtin_interface_register(Iterator, 2, FunctionNext, FunctionHasNext);
    builtin_interface_register(Connector, 1, FunctionQuery);
    builtin_interface_register(CtxHandler, 2, FunctionEnter, FunctionLeave);
    builtin_interface_register(Incrementable, 2, FunctionIncr, FunctionDecr);
  }
  if (!_descriptors) {
    builtin_typedescr_register_nomethods(Kind, "Kind", kind_t);
    builtin_typedescr_register_nomethods(Type, "Type", typedescr_t);
    typedescr_assign_inheritance(Type, Kind);
    builtin_typedescr_register_nomethods(Interface, "Interface", interface_t);
    typedescr_assign_inheritance(Interface, Kind);
    builtin_typedescr_register_nomethods(Method, "Method", methoddescr_t);
    typedescr_register_methods(Kind, _methods_Kind);
    typedescr_register_methods(Type, _methods_Type);
    typedescr_register_methods(Interface, _methods_Interface);
    typedescr_register_methods(Method, _methods_Method);
    any_init();
  }
}

kind_t * kind_get(int ix) {
  return (ix < FirstInterface)
    ? (kind_t *) typedescr_get(ix)
    : (kind_t *) interface_get(ix);
}

kind_t * kind_get_byname(char *name) {
  kind_t *ret;
  ret = (kind_t *) typedescr_get_byname(name);

  if (!ret) {
    ret = (kind_t *) interface_get_byname(name);
  }
  return ret;
}

/* -- G E N E R A L  S T A T I C  F U N C T I O N S ----------------------- */

datalist_t * _add_method_reducer(methoddescr_t *mth, datalist_t *methods) {
  datalist_push(methods, (data_t *) mth);
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
  datalist_t *ret;
  int         ix;

  if (!strcmp(name, "name")) {
    return str_to_data(mth -> name);
  } else if (!strcmp(name, "args")) {
    ret = datalist_create(NULL);
    for (ix = 0; ix < MAX_METHOD_PARAMS; ix++) {
      if (!mth -> argtypes[ix]) {
        break;
      }
      datalist_push(ret, (data_t *) kind_get(mth -> argtypes[ix]));
    }
    return (data_t *) ret;
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

/* -- G E N E R I C  T Y P E  D E S C R I P T O R ------------------------- */

kind_t * _kind_init(kind_t * descr, int kind, int type, char *name) {
  descr -> _d.cookie = MAGIC_COOKIE;
  descr -> _d.type = kind;
  descr -> _d.data_semantics = DataSemanticsConstant;
  data_set_string_semantics(descr, StrSemanticsExternStatic);
  descr -> _d.refs = 1;
  descr -> _d.str = NULL;
  descr -> type = type;
  descr -> name = strdup(name);
  descr -> methods = strdata_dict_create();
  data_register(descr);
  return descr;
}

void kind_register_method(kind_t *kind, methoddescr_t *method) {
  if (type_debug) {
    info("kind_register_method(%s, %s)", kind -> name, method -> name);
  }
  if (!dict_has_key(kind -> methods, method -> name)) {
    method -> _d.type = Method;
    method -> _d.data_semantics = DataSemanticsConstant;
    method -> _d.refs = 1;
    method -> _d.str = NULL;
    data_set_string_semantics(method, StrSemanticsExternStatic);
    dict_put(kind -> methods, strdup(method -> name), method);
  }
}

methoddescr_t * kind_get_method(kind_t *kind, char *name) {
  return (methoddescr_t *) dict_get(kind -> methods, name);
}

char * _kind_tostring(kind_t *kind) {
  return kind -> name;
}

int _kind_cmp(kind_t *kind1, kind_t *kind2) {
  return kind1 -> type - kind2 -> type;
}

unsigned int _kind_hash(kind_t *kind) {
  return strhash(kind -> name);
}

data_t * _kind_resolve(kind_t *kind, char *name) {
  datalist_t *list;

  if (!strcmp(name, "name")) {
    return str_to_data(kind -> name);
  } else if (!strcmp(name, "id")) {
    return int_to_data(kind -> type);
  } else if (!strcmp(name, "methods")) {
    list = datalist_create(NULL);
    dict_reduce_values(kind -> methods, (reduce_t) _add_method_reducer, list);
    return (data_t *) list;
  } else {
    return NULL;
  }
}

kind_t * _inherit_method_reducer(methoddescr_t *mth, kind_t *kind) {
  kind_register_method(kind, mth);
  return kind;
}

kind_t * _kind_inherit_methods(kind_t *kind, kind_t *from) {
  return dict_reduce_values(from -> methods,
         (reduce_t) _inherit_method_reducer,
         kind);
}

/* -- I N T E R F A C E   F U N C T I O N S ------------------------------- */

int _interface_register(int type, char *name, int numfncs, ...) {
  size_t        ix;
  size_t        cursz;
  size_t        newsz;
  interface_t **new_interfaces;
  interface_t  *iface;
  va_list       fncs;

  if ((type != Any) && !_interfaces) {
    typedescr_init();
  }
  if (type < FirstInterface) {
    type = _next_interface++;
  }
  ix = type - FirstInterface - 1;
  if (ix >= _num_interfaces) {
    newsz = (ix + 1) * sizeof(interface_t *);
    cursz = _num_interfaces * sizeof(interface_t *);
    new_interfaces = (interface_t **) resize_block(_interfaces, newsz, cursz);
    _interfaces = new_interfaces;
    _num_interfaces = ix + 1;
  }
  if (type_debug) {
    info("Registering interface '%s' [%d:%d]. _num_interfaces: %d",
      name, type, ix, _num_interfaces);
  }
  iface = NEWDYNARR(interface_t, numfncs + 1, int);
  _interfaces[ix] = iface;
  _kind_init((kind_t *) iface, Interface, type, name);
  va_start(fncs, numfncs);
  if (numfncs < 0) {
    numfncs = 0;
  }
  for (ix = 0; ix < (size_t) numfncs; ix++) {
    iface -> fncs[ix] = va_arg(fncs, int);
  }
  va_end(fncs);
  iface -> fncs[numfncs] = 0;
  return type;
}

interface_t * interface_get(int type) {
  size_t       ifix = type - FirstInterface - 1;
  interface_t *ret = NULL;

  if (!_interfaces) {
    typedescr_init();
  }
  if ((type > FirstInterface) && (type < _next_interface)) {
    ret = _interfaces[ifix];
    if (!ret) {
      if (type_debug) {
        error("Undefined interface type %d referenced. Expect crashes", type);
      }
      return NULL;
    } else {
      if (ret -> _d.type != type) {
        _debug("looking for type %d, found %d (%s)", type, ret -> _d.type, ret -> _d.name);
      }
      assert(ret -> _d.type == type);
    }
  } else {
    if (type_debug) {
      error("Type %d referenced as interface, but it cannot be. Expect crashes", type);
    }
  }
  return ret;
}

interface_t * interface_get_byname(char *name) {
  size_t ifix;

  if (!_interfaces) {
    typedescr_init();
  }
  for (ifix = 0; ifix < _num_interfaces; ifix++) {
    if (!strcasecmp(_interfaces[ifix] -> _d.name, name)) {
      return _interfaces[ifix];
    }
  }
  return NULL;
}

data_t * _interface_resolve(interface_t *iface, char *name) {
  datalist_t  *list;
  size_t       ix;
  typedescr_t *type;

  if (!strcmp(name, "implementations")) {
    list = datalist_create(NULL);
    for (ix = 0; ix < _numtypes; ix++) {
      type = _descriptors[ix];
      if (type && typedescr_implements(type, iface -> _d.type)) {
        datalist_push(list, (data_t *) type);
      }
    }
    return (data_t *) list;
  } else {
    return NULL;
  }
}

data_t * _interface_isimplementedby(interface_t *iface, char _unused_ *name, arguments_t *args) {
  data_t      *data = data_uncopy(arguments_get_arg(args, 0));
  typedescr_t *type;

  if (data_hastype(data, Interface)) {
    return data_false();
  } else {
    type = (data_hastype(data, Type))
      ? (typedescr_t *) data
      : data_typedescr(data);
    return int_as_bool(typedescr_is(type, iface -> _d.type));
  }
}

data_t * _interface_implements(data_t *self, char *name, arguments_t *args) {
  arguments_t *a;
  interface_t *iface = data_as_interface(data_uncopy(arguments_get_arg(args, 0)));
  data_t      *ret;

  a = arguments_create_args(1, self);
  ret = _interface_isimplementedby(iface, name, a);
  arguments_free(a);
  return ret;
}

/* -- V T A B L E  F U N C T I O N S -------------------------------------- */

void vtable_dump(vtable_t *vtable) {
  int ix;

  for (ix = 0; ix < FunctionEndOfListDummy; ix++) {
    assert(ix == vtable[ix].id);
    if (vtable[ix].fnc) {
      _debug("%-20.20s %d %p",
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
  vtable_id_t  ix;
  vtable_id_t  fnc_id;
  vtable_t    *ret;

  ret = (vtable_t *) new((FunctionEndOfListDummy + 1) * sizeof(vtable_t));
  for (ix = FunctionNone; ix <= FunctionEndOfListDummy; ix++) {
    ret[ix].id = ix;
    ret[ix].fnc = NULL;
  }
  if (vtable) {
    for (ix = FunctionNone; vtable[ix].fnc; ix++) {
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

data_t * _typedescr_resolve(typedescr_t *descr, char *name) {
  datalist_t  *list;
  size_t       ix;
  interface_t *iface;

  if (!strcmp(name, "inherits")) {
    list = datalist_create(NULL);
    for (ix = 0; descr -> inherits[ix]; ix++) {
      datalist_push(list, (data_t *) typedescr_get(descr -> inherits[ix]));
    }
    return (data_t *) list;
  } else if (!strcmp(name, "ancestors")) {
    list = datalist_create(NULL);
    for (ix = 0; descr -> ancestors[ix]; ix++) {
      datalist_push(list, (data_t *) typedescr_get(descr -> ancestors[ix]));
    }
    return (data_t *) list;
  } else if (!strcmp(name, "implements")) {
    _typedescr_get_all_interfaces(descr);
    list = datalist_create(NULL);
    for (ix = 0; ix < descr -> implements_sz; ix++) {
      iface = (descr -> implements[ix])
        ? _interfaces[descr -> implements[ix]] : NULL;
      if (iface) {
        datalist_push(list, (data_t *) iface);
      }
    }
    return (data_t *) list;
  } else {
    return NULL;
  }
}

ctx_wrapper_t * _kind_reduce_methods(methoddescr_t *mth, ctx_wrapper_t *wrapper) {
  wrapper->ctx = wrapper->reducer(mth, wrapper->ctx);
  return wrapper;
}

void * _kind_reduce_children(kind_t *kind, reduce_t reducer, void *ctx) {
  ctx_wrapper_t wrapper;

  if (kind && kind->methods) {
    wrapper.reducer = reducer;
    wrapper.ctx = ctx;
    dict_reduce_values(kind->methods, (reduce_t) _kind_reduce_methods, &wrapper);
    return wrapper.ctx;
  }
}

data_t * _typedescr_gettype(data_t _unused_ *self, char _unused_ *name, arguments_t *args) {
  data_t      *t = data_uncopy(arguments_get_arg(args, 0));
  typedescr_t *type;

  type = (data_is_int(t))
    ? typedescr_get(data_intval(t))
    : typedescr_get_byname(data_tostring(t));
  return (type)
    ? (data_t *) type
    : data_exception(ErrorParameterValue, "Type '%s' not found", data_tostring(t));
}

data_t * _typedescr_hastype(data_t *self, char _unused_ *name, arguments_t *args) {
  typedescr_t *type = data_as_typedescr(data_uncopy(arguments_get_arg(args, 0)));

  return data_hastype(self, type -> _d.type) ? data_true() : data_false();
}

/* ------------------------------------------------------------------------ */

int _typedescr_check_if_implements(typedescr_t *descr, interface_t *iface) {
  int ret;
  int ix;
  int fnc_id;

  if (!iface) {
    return FALSE;
  }
  if ((iface -> _d.type == Any) && descr) {
    return TRUE;
  }
  ret = TRUE;
  for (ix = 0; ret && iface -> fncs[ix]; ix++) {
    fnc_id = iface -> fncs[ix];
    ret &= (typedescr_get_function(descr, fnc_id) != NULL);
  }
  return ret;
}

int * _typedescr_get_all_interfaces(typedescr_t *descr) {
  size_t ix;

  if (!descr -> implements || (_num_interfaces > descr -> implements_sz)) {
    free(descr -> implements);
    descr -> implements = NEWARR((size_t) _num_interfaces, int);
    descr -> implements_sz = _num_interfaces;
    for (ix = 0; ix < _num_interfaces; ix++) {
      descr -> implements[ix] = _typedescr_check_if_implements(descr, _interfaces[ix]);
      if (descr -> implements[ix]) {
        _kind_inherit_methods((kind_t *) descr, (kind_t *) _interfaces[ix]);
      }
    }
  }
  return descr -> implements;
}

void_t * _typedescr_get_constructors(typedescr_t *type) {
  void_t  local;
  void_t *inherited;
  int     ix;
  int     iix;
  size_t  count;

  free(type -> constructors);
  for (count = 0, ix = 0; type -> inherits[ix]; ix++) {
    inherited = _typedescr_get_constructors(typedescr_get(type -> inherits[ix]));
    for (iix = 0; inherited[iix]; iix++) {
      count++;
    }
  }
  local = typedescr_get_local_function(type, FunctionNew);
  if (local) {
    count++;
  }
  type -> constructors = NEWARR(count + 1, void_t);
  for (count = 0, ix = 0; type -> inherits[ix]; ix++) {
    inherited = _typedescr_get_constructors(typedescr_get(type -> inherits[ix]));
    for (iix = 0; inherited[iix]; iix++) {
      type -> constructors[count++] = inherited[iix];
    }
  }
  if (local) {
    type -> constructors[count++] = local;
  }
  type -> constructors[count] = NULL;
  return type -> constructors;
}

typedescr_t * _typedescr_build_ancestors(typedescr_t *td) {
  size_t       count;
  int          ix;
  int          iix;
  typedescr_t *base;

  free(td -> ancestors);

  /* Count the total number of ancestors: */
  for (count = 0, ix = 0; td -> inherits[ix]; ix++) {
    base = typedescr_get(td -> inherits[ix]);
    for (iix = 0; base -> ancestors[iix]; iix++) {
      count++;
    }
    count++; /* For the reference to base */
  }

  td -> ancestors = NEWARR(count + 1, int); /* +1 for the Terminating 0 */
  td -> ancestors[count] = 0;
  for (count = 0, ix = 0; td -> inherits[ix]; ix++) {
    base = typedescr_get(td -> inherits[ix]);
    for (iix = 0; base -> ancestors[iix]; iix++) {
      td -> ancestors[count++] = base -> ancestors[iix];
    }
    td -> ancestors[count++] = typetype(base);
  }
  for (ix = 0; td -> ancestors[ix]; ix++) {
    base = typedescr_get(td -> ancestors[ix]);
    _kind_inherit_methods((kind_t *) td, (kind_t *) base);
  }
  return td;
}

typedescr_t * _typedescr_build_inherited_vtable(typedescr_t *td) {
  int          ix;
  int          iix;
  typedescr_t *base;

  for (ix = 0; ix < FunctionEndOfListDummy; ix++) {
    if (!td -> inherited_vtable[ix].fnc) {
      for (iix = 0; td -> inherits[iix] && !td -> inherited_vtable[ix].fnc; iix++) {
        base = typedescr_get(td -> inherits[iix]);
        td -> inherited_vtable[ix].fnc = base -> inherited_vtable[ix].fnc;
      }
    }
  }
  return td;
}

typedescr_t * _typedescr_initialize_vtable(typedescr_t *type, vtable_t vtable[]) {
  free(type -> vtable);
  free(type -> inherited_vtable);
  type -> vtable = vtable_build(vtable);
  type -> inherited_vtable = vtable_build(vtable);
  return type;
}

/* -- T Y P E D E S C R  P U B L I C  F U N C T I O N S ------------------- */

int _typedescr_register(int type, char *type_name, vtable_t *vtable, methoddescr_t *methods) {
  typedescr_t  *d;
  size_t        cursz;
  size_t        newsz;
  size_t        newcap;

  if ((type != Kind) && !_descriptors) {
    typedescr_init();
  }
  debug(type, "Registering type '%s' [%d]", type_name, type);
  if (type <= 0) {
    type = (_numtypes > (size_t) Dynamic) ? (int) _numtypes : (int) Dynamic;
    _numtypes = (size_t) (type + 1);
    debug(type, "Giving type '%s' ID %d", type_name, type);
  }
  if ((size_t) type >= _capacity) {
    for (newcap = (_capacity) ? _capacity * 2 : (size_t) Dynamic;
         newcap < (size_t) type;
         newcap *= 2);
    cursz = _capacity * sizeof(typedescr_t *);
    newsz = newcap * sizeof(typedescr_t *);
    debug(type, "Expanding type dictionary buffer from %d to %d (%d/%d bytes)",
        _capacity, newcap, cursz, newsz);
    _descriptors = (typedescr_t **) resize_block(_descriptors, newsz, cursz);
    _capacity = newcap;
  }
  d = NEW(typedescr_t);
  _descriptors[type] = d;
  _kind_init((kind_t *) d, Type, type, type_name);
  _typedescr_initialize_vtable(d, vtable);
  if (methods) {
    typedescr_register_methods(type, methods);
  }
  d -> inherits = NEWARR(1, int);
  d -> inherits[0] = 0;
  d -> ancestors = NEWARR(1, int);
  d -> ancestors[0] = 0;
  d -> accessors = NULL;
  _typedescr_get_all_interfaces(d);
  _typedescr_get_constructors(d);
  return type;
}

typedescr_t * typedescr_assign_inheritance(int type, int inherits) {
  typedescr_t *td;
  typedescr_t *base;
  int          ix;

  td = typedescr_get(type);
  if (td) {
    for (ix = 0; td -> inherits && td -> inherits[ix]; ix++);
    td -> inherits = resize_block(td -> inherits,
      sizeof(int) * (ix + 2), (td -> inherits) ? (sizeof(int) * (ix + 1)) : 0);
    td -> inherits[ix] = inherits;
    td -> inherits[ix + 1] = 0;
    base = typedescr_get(inherits);
    if (!base) {
      fatal("Attempt to make type '%s' a subtype of non-existant type '%d'",
        td -> _d.name, inherits);
    }
    _typedescr_build_inherited_vtable(td);
    _typedescr_get_constructors(td);
    _typedescr_build_ancestors(td);
    td -> implements_sz = 0;
    _typedescr_get_all_interfaces(td);
  }
  return td;
}

typedescr_t * typedescr_register_accessors(int type, accessor_t *accessors) {
  typedescr_t *descr = typedescr_get(type);

  assert(descr);
  for (; accessors -> name; accessors++) {
    if (!descr -> accessors) {
      descr -> accessors = strvoid_dict_create();
      dict_set_free_key(descr -> accessors, NULL);
    }
    dict_put(descr -> accessors, accessors -> name, accessors);
  }
  return descr;
}

accessor_t * typedescr_get_accessor(typedescr_t *type, char *name) {
  return (type -> accessors)
         ? (accessor_t *) dict_get(type -> accessors, name)
         : NULL;
}

#define DEBUG_TYPES
typedescr_t * typedescr_get(int datatype) {
#ifdef DEBUG_TYPES
  typedescr_t *ret = NULL;

  if (!_descriptors) {
    typedescr_init();
  }
  if ((datatype >= 0) && (datatype < (int) _numtypes)) {
    ret = _descriptors[datatype];
  }
  if (!ret) {
    fatal("Undefined type %d referenced. Terminating...", datatype);
    return NULL;
  } else {
    return ret;
  }
#else
  return (_descriptors) ? _descriptors[datatype] : NULL;
#endif
}

typedescr_t * typedescr_get_byname(char *name) {
  typedescr_t *ret = NULL;
  size_t       ix;

  if (!_descriptors) {
    typedescr_init();
  }
  for (ix = 0; ix < _numtypes; ix++) {
    if (_descriptors[ix] &&
        type_name(_descriptors[ix]) &&
        !strcasecmp(name, type_name(_descriptors[ix]))) {
      return _descriptors[ix];
    }
  }
  debug(type, "typedescr_get_byname(%s) = %d", name, (ret) ? ret -> _d.type : -1);
  return NULL;
}

void typedescr_dump_vtable(typedescr_t *type) {
  _debug("vtable for %s", typedescr_tostring(type));
  vtable_dump(type -> vtable);
}

void typedescr_count(void) {
  size_t ix;

  _debug("Atom count");
  _debug("-------------------------------------------------------");
  for (ix = 0; ix < _numtypes; ix++) {
    _debug("%3d. %-20.20s %6d", _descriptors[ix] -> _d.type, _descriptors[ix] -> _d.name, _descriptors[ix] -> count);
  }
  _debug("-------------------------------------------------------");
  _debug("     %-20.20s %6d", "T O T A L", _data_count);
}

void typedescr_register_methods(int type, methoddescr_t methods[]) {
  methoddescr_t *method;
  kind_t        *kind;
  int            ix;

  typedescr_init();
  if (!methods) {
    return;
  }
  for (method = methods; method -> type != NoType; method++) {
    if (method -> type < 0) {
      method -> type = type;
    }
    data_settype((data_t *) method, Method);
    for (ix = 0; (ix < MAX_METHOD_PARAMS) && method -> argtypes[ix]; ix++) {
      if (method -> argtypes[ix] < 0) {
        method -> argtypes[ix] = type;
      }
    }
    kind = kind_get(method -> type);
    kind_register_method(kind, method);
  }
}

int typedescr_implements(typedescr_t *descr, int type) {
  _typedescr_get_all_interfaces(descr);
  return descr -> implements[type - FirstInterface - 1];
}

int typedescr_inherits(typedescr_t *descr, int type) {
  int ix;

  if (descr -> _d.type == type) {
    return TRUE;
  } else {
    for (ix = 0; descr -> ancestors && descr -> ancestors[ix]; ix++) {
      if (descr -> ancestors[ix] == type) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

int typedescr_is(typedescr_t *descr, int type) {
  int       ix;
  int       ret = FALSE;
  int     (*fnc)(typedescr_t *, int);

  if ((type == descr -> _d.type) || (type == Any)) {
    ret = TRUE;
  } else if (descr -> vtable[FunctionIs].fnc) {
    fnc = (int (*)(typedescr_t *, int)) descr -> vtable[FunctionIs].fnc;
    ret = fnc(descr, type);
  } else if (type > FirstInterface) {
    if (!descr -> implements || (_num_interfaces > descr -> implements_sz)) {
      _typedescr_get_all_interfaces(descr);
    }
    ret = descr -> implements[type - FirstInterface - 1];
  } else {
    for (ix = 0; descr -> ancestors && descr -> ancestors[ix]; ix++) {
      if (descr -> ancestors[ix] == type) {
        ret = TRUE;
        break;
      }
    }
  }
  return ret;
}

methoddescr_t * typedescr_get_method(typedescr_t *descr, char *name) {
  if (!descr -> implements || ((int) _num_interfaces > descr -> implements_sz)) {
    _typedescr_get_all_interfaces(descr);
  }
  return kind_get_method((kind_t *) descr, name);
}
