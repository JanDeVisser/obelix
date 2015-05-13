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

#include <core.h>
#include <logging.h>
#include <typedescr.h>
#include <stdio.h>

static int          _numtypes = 0;
static typedescr_t *descriptors = NULL;
static interface_t *_interfaces = NULL;
static int          _next_interface = NextInterface;
static int          _num_interfaces = 0;

extern int          _data_count;
       int          type_debug = 0;

static void         _typedescr_init(void) __attribute__((constructor(101)));
static char *       _methoddescr_tostring(methoddescr_t *);

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
  { .code = FunctionEndOfListDummy, .label = "End" },
  { .code = -1,                     .label = NULL }
};

/* ------------------------------------------------------------------------ */

void _typedescr_init(void) {
  logging_register_category("type", &type_debug);
}

char * _methoddescr_tostring(methoddescr_t *mth) {
  return mth -> name;
}

/* -- I N T E R F A C E   F U N C T I O N S-------------------------------- */

int interface_register(int type, char *name, int numfncs, ...) {
  int          ix;
  int          iix;
  int          cursz;
  int          newsz;
  interface_t *new_interfaces;
  interface_t *iface;
  va_list      fncs;
  int          fnc_id;
  
  if (type < Interface) {
    type = _next_interface++;
  }
  ix = type - Interface - 1;
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
  int          ifix = type - Interface - 1;
  interface_t *ret = NULL;
  
  if ((type > Interface) && (type < _next_interface)) {
    ret = &_interfaces[ifix];
    if (!ret) {
      error("Undefined interface type %d referenced. Expect crashes", type);
      return NULL;
    } else {
      //if (type_debug) {
      //  debug("interface_get(%d:%d) = %s", type, ifix, ret -> name);
      //}
      assert(ret -> type == type);
    }
  } else {
    error("Type %d referenced as interface, but it cannot be. Expect crashes", type);
  }
  return ret;
}

void interface_register_method(interface_t *iface, methoddescr_t *method) {
  if (type_debug) {
    info("interface_register_method(%s, %s)", iface -> name, method -> name);
  }
  if (!iface -> methods) {
    iface -> methods = strvoid_dict_create();
    dict_set_tostring_data(iface -> methods, (tostring_t) _methoddescr_tostring);
  }
  dict_put(iface -> methods, method -> name, method);
  if (type_debug) {
    debug(" Out: #methods %d   methods: %s", dict_size(iface -> methods), dict_tostring(iface -> methods));
  }
}

methoddescr_t * interface_get_method(interface_t *iface, char *name) {
  if (type_debug) {
    debug("interface_get_method(%s, %s)", iface -> name, name);
    if (iface -> methods) {
      debug(" #methods %d   methods: %s", dict_size(iface -> methods), dict_tostring(iface -> methods));
    } else {
      debug(" '%s' -> methods unset", iface -> name);
    }
  }
  return (iface -> methods) 
    ? (methoddescr_t *) dict_get(iface -> methods, name) 
    : NULL;
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
  interface_t *interface;
  int          ix;
  int          fnc_id;
  
  interface = interface_get(type);
  if (!interface) {
    return FALSE;
  }
  ret = TRUE;
  for (ix = 0; ret && interface -> fncs[ix]; ix++) {
    fnc_id = interface -> fncs[ix];
    ret &= (vtable[fnc_id].fnc != NULL);
  }
  return ret;
}

/* -- T Y P E D E S C R  P U B L I C  F U N C T I O N S ------------------- */

int typedescr_register(typedescr_t *descr) {
  typedescr_t    *new_descriptors;
  vtable_t       *vtable;
  int             newsz;
  int             cursz;
  typedescr_t    *d;
  interface_t    *interface;
  int             interfaces[sizeof(_interfaces) / sizeof(interface_t)];
  int             num_interfaces = 0;

  if (descr -> type < 0) {
    descr -> type = (_numtypes > Dynamic) ? _numtypes : Dynamic;
  }
  if (type_debug) {
    info("Registering type '%s' [%d]", descr -> type_name, descr -> type);
  }
  if (!descriptors) {
    descriptors = (typedescr_t *) new_array(Dynamic, sizeof(typedescr_t));
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
  vtable = d -> vtable;
  d -> vtable = NULL;
  typedescr_register_functions(d, vtable);
  d -> str = NULL;
  d -> methods = NULL;
  d -> hash = 0;
  return d -> type;
}

void typedescr_register_types(typedescr_t *types) {
  typedescr_t *type;

  for (type = types; type -> type_name; type++) {
    typedescr_register(type);
  }
}


typedescr_t * typedescr_register_functions(typedescr_t *type, vtable_t vtable[]) {
  int ix;
  int fnc_id;

  if (type -> vtable) {
    free(type -> vtable);
  }
  type -> vtable = vtable_build(vtable);
  return type;
}

typedescr_t * typedescr_get(int datatype) {
  typedescr_t *ret = NULL;
  
  if ((datatype >= 0) && (datatype < _numtypes)) {
    ret = &descriptors[datatype];
  }
  if (!ret) {
    error("Undefined type %d referenced. Expect crashes", datatype);
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

void_t typedescr_get_function(typedescr_t *type, int fnc_id) {
  void_t       ret = NULL;
  int          ix;
  typedescr_t *inherits;

  assert(type);
  assert(fnc_id > FunctionNone);
  assert(fnc_id < FunctionEndOfListDummy);
  if (type -> vtable) {
    ret = type -> vtable[fnc_id].fnc;
    for (ix = 0; !ret && ix < type -> inherits_size; ix++) {
      inherits = typedescr_get(type -> inherits[ix]);
      ret = typedescr_get_function(inherits, fnc_id);
    }
    //if (type_debug) {
    //  debug("typedescr_get_function(%s, %s) = %p", 
    //        type -> type_name, label_for_code(_function_id_labels, fnc_id), ret);
    //}
  } else {
    error("Type '%s' has no vtable. Quite odd.", type -> type_name);
  }
  return ret;
}

void typedescr_register_methods(methoddescr_t methods[]) {
  methoddescr_t *method;
  typedescr_t   *type;
  interface_t   *interface;

  for (method = methods; method -> type > NoType; method++) {
    if (method -> type < Interface) {
      type = typedescr_get(method -> type);
      assert(type);
      typedescr_register_method(type, method);
    } else {
      interface = interface_get(method -> type);
      assert(interface);
      interface_register_method(interface, method);
    }
  }
}

void typedescr_register_method(typedescr_t *type, methoddescr_t *method) {
  if (type_debug) {
    info("typedescr_register_method(%s, %s)", type -> type_name, method -> name);
  }
  if (!type -> methods) {
    type -> methods = strvoid_dict_create();
    dict_set_tostring_data(type -> methods, (tostring_t) _methoddescr_tostring);
  }
  dict_put(type -> methods, method -> name, method);
}

methoddescr_t * typedescr_get_method(typedescr_t *descr, char *name) {
  methoddescr_t *ret = NULL;
  interface_t   *interface;
  int            ix;
  int            iftype;
  
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
    if (!ret && descr -> inherits_size) {
      for (ix = 0; !ret && ix < descr -> inherits_size; ix++) {
        ret = typedescr_get_method(typedescr_get(descr -> inherits[ix]), name);
      }
    }
    if (ret) {
      typedescr_register_method(descr, ret);
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
      ret = (type > Interface) && vtable_implements(descr -> vtable, type);
      //if (type_debug) {
      //  debug("'%s'.is(%d) = %d by implementation", descr -> type_name, type, ret);
      //}
    }
  }
  if (!ret && descr -> inherits_size) {
    //if (type_debug) {
    //  debug("'%s'.is(%d) checking parents", descr -> type_name, type);
    //}
    ret = 0;
    for (ix = 0; !ret && ix < descr -> inherits_size; ix++) {
      ret = typedescr_is(typedescr_get(descr -> inherits[ix]), type);
    }
  }
  //if (type_debug) {
  //  debug("Survey says: '%s'.is(%d) = %d", descr -> type_name, type, ret);
  //}
  return ret;
}
