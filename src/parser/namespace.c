/*
 * /obelix/src/parser/namespace.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <array.h>
#include <exception.h>
#include <logging.h>
#include <namespace.h>

static void          _namespace_init(void) __attribute__((constructor));

static data_t *      _data_new_module(int, va_list);
static data_t *      _data_copy_module(data_t *, data_t *);
static int           _data_cmp_module(data_t *, data_t *);
static char *        _data_tostring_module(data_t *);
static unsigned int  _data_hash_module(data_t *);
static data_t *      _data_resolve_module(data_t *, char *);
static data_t *      _data_call_module(data_t *, array_t *, dict_t *);

static namespace_t * _ns_create(void);
static module_t *    _ns_add(namespace_t *, name_t *);
static data_t *      _ns_delegate_up(namespace_t *, module_t *, name_t *, array_t *, dict_t *);
static data_t *      _ns_delegate_load(namespace_t *, module_t *, name_t *, array_t *, dict_t *);
static data_t *      _ns_import(namespace_t *, name_t *, array_t *, dict_t *);

int ns_debug = 0;

vtable_t _vtable_module[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _data_new_module },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_module },
  { .id = FunctionFree,     .fnc = (void_t) mod_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_module },
  { .id = FunctionHash,     .fnc = (void_t) _data_hash_module },
  { .id = FunctionResolve,  .fnc = (void_t) _data_resolve_module },
  { .id = FunctionCall,     .fnc = (void_t) _data_call_module },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t typedescr_module = {
  .type       = Module,
  .type_name  = "module",
  .vtable     = _vtable_module
};

/* ------------------------------------------------------------------------ */

typedef struct _pnm {
  name_t   *name;
  char     *next;
  set_t    *matches;
  set_t    *match_lost;
  void     *match;
  int       refs;
} pnm_t;

static pnm_t *     _pnm_create(char *);
static void        _pnm_free(pnm_t *);
static pnm_t *     _pnm_add(pnm_t *, module_t *);
static pnm_t *     _pnm_find_mod_reducer(module_t *, pnm_t *);
static module_t *  _pnm_find_mod(pnm_t *);
static pnm_t *     _pnm_matches_reducer(module_t * , pnm_t *);
static pnm_t *     _pnm_find_in_mod_reducer(module_t *, pnm_t *);

static data_t *    _data_new_pnm(data_t *, va_list);
static data_t *    _data_copy_pnm(data_t *, data_t *);
static int         _data_cmp_pnm(data_t *, data_t *);
static char *      _data_tostring_pnm(data_t *);
static data_t *    _data_resolve_pnm(data_t *, char *);
static data_t *    _data_call_pnm(data_t *, array_t *, dict_t *);
static data_t *    _data_resolve_pnm(data_t *, char *);
static data_t *    _data_set_pnm(data_t *, char *, data_t *);

int PartialNameMatch;

static vtable_t _vtable_pnm[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_pnm },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_pnm },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_pnm },
  { .id = FunctionFree,     .fnc = (void_t) _pnm_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_pnm },
  { .id = FunctionResolve,  .fnc = (void_t) _data_resolve_pnm },
  { .id = FunctionCall,     .fnc = (void_t) _data_call_pnm },
  { .id = FunctionSet,      .fnc = (void_t) _data_set_pnm },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_pnm = {
  .type =      -1,
  .type_name = "pnm",
  .vtable =    _vtable_pnm
};

/* ------------------------------------------------------------------------ */

void _namespace_init(void) {
  logging_register_category("namespace", &ns_debug);
  typedescr_register(&typedescr_module);
  PartialNameMatch = typedescr_register(&_typedescr_pnm);
}

/* -- Partial Name Match data functions ----------------------------------- */

pnm_t * _pnm_create(char *name) {
  pnm_t *ret;
  
  ret = NEW(pnm_t);
  ret -> refs = 1;
  ret -> name = name_create(1, name);
  ret -> matches = set_create((cmp_t) mod_cmp);
  set_set_hash(ret -> matches, (hash_t) mod_hash);
  set_set_free(ret -> matches, (free_t) mod_free);
  set_set_tostring(ret -> matches, (tostring_t) mod_tostring);
  return ret;
}

void _pnm_free(pnm_t *pnm) {
  if (pnm && --pnm -> refs <= 0) {
    name_free(pnm -> name);
    set_free(pnm -> matches);
  }
}

static pnm_t * _pnm_add(pnm_t *pnm, module_t *mod) {
  set_add(pnm -> matches, mod_copy(mod));
  return pnm;
}

pnm_t * _pnm_find_mod_reducer(module_t *mod, pnm_t *pnm) {
  data_t   *match;
  
  if (!pnm -> match && !name_cmp(mod -> name, pnm -> name)) {
    pnm -> match = mod;
  }
  return pnm;
}

module_t * _pnm_find_mod(pnm_t *pnm) {
  pnm -> match = NULL;
  set_reduce(pnm -> matches, (reduce_t) _pnm_find_mod_reducer, pnm);
  return (module_t *) pnm -> match;
}

pnm_t * _pnm_find_in_mod_reducer(module_t *mod, pnm_t *pnm) {
  data_t   *match;
  
  if (!name_cmp(mod -> name, pnm -> name)) {
    pnm -> match = mod_resolve(mod, pnm -> next);
  }
  return pnm;
}

pnm_t * _pnm_matches_reducer(module_t * mod, pnm_t *pnm) {
  if (!name_startswith(mod -> name, pnm -> name)) {
    if (!pnm -> match_lost) {
      pnm -> match_lost = set_create((cmp_t) mod_cmp);
      set_set_hash(pnm -> match_lost, (hash_t) mod_hash);
      set_set_free(pnm -> match_lost, (free_t) mod_free);
      set_set_tostring(pnm -> match_lost, (tostring_t) mod_tostring);
    }
    set_add(pnm -> match_lost, mod);
  }
  return pnm;
}

/* ------------------------------------------------------------------------ */

data_t * _data_new_pnm(data_t *ret, va_list arg) {
  pnm_t *pnm;
  char  *name;
  
  name = va_arg(arg, char *);
  pnm = _pnm_create(name);
  
  ret -> ptrval = pnm;
  return ret;
}

data_t * _data_copy_pnm(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((pnm_t *) target -> ptrval) -> refs++;
  return target;
}

int _data_cmp_pnm(data_t *d1, data_t *d2) {
  pnm_t *pnm1;
  pnm_t *pnm2;

  pnm1 = d1 -> ptrval;
  pnm2 = d2 -> ptrval;
  return name_cmp(pnm1 -> name, pnm2 -> name);
}

char * _data_tostring_pnm(data_t *d) {
  return name_tostring(((pnm_t *) d -> ptrval) -> name);
}

data_t * _data_resolve_pnm(data_t *data, char *name) {
  pnm_t *pnm = (pnm_t *) data -> ptrval;
  
  pnm -> next = name;
  pnm -> match = NULL;
  if (pnm -> match_lost) {
    set_clear(pnm -> match_lost);
  }
  
  set_reduce(pnm -> matches, (reduce_t) _pnm_find_in_mod_reducer, pnm);
  if (pnm -> match) {
    return (data_t *) pnm -> match;
  } else {
    name_extend(pnm -> name, name);
    set_reduce(pnm -> matches, (reduce_t) _pnm_matches_reducer, pnm);
    set_minus(pnm -> matches, pnm -> match_lost);
    return data;
  }
}

data_t * _data_call_pnm(data_t *self, array_t *params, dict_t *kwargs) {
  pnm_t    *pnm = (pnm_t *) self -> ptrval;
  module_t *mod = _pnm_find_mod(pnm);
  
  return (mod)
    ? object_call(mod -> obj, params, kwargs) 
    : data_exception(ErrorName, "Trouble locating '%s'", name_tostring(pnm -> name));
}

data_t * _data_set_pnm(data_t *data, char *name, data_t *value) {
  pnm_t    *pnm = (pnm_t *) data -> ptrval;
  module_t *mod = _pnm_find_mod(pnm);
  
  return (mod)
    ? object_set(mod -> obj, name, value) 
    : data_exception(ErrorName, "Trouble locating '%s'", name_tostring(pnm -> name));
}

/* -- M O D U L E  D A T A  F U N C T I O N S ----------------------------- */

data_t * _data_new_module(int type, va_list arg) {
  module_t *module;
  data_t   *ret;

  module = va_arg(arg, module_t *);
  ret = &module -> data;
  if (!ret -> ptrval) {
    ret -> ptrval = mod_copy(module);
  }
  return ret;
}

int _data_cmp_module(data_t *d1, data_t *d2) {
  module_t *m1;
  module_t *m2;

  m1 = (module_t *) d1 -> ptrval;
  m2 = (module_t *) d2 -> ptrval;
  return mod_cmp(m1, m2);
}

char * _data_tostring_module(data_t *d) {
  return mod_tostring((module_t *) d -> ptrval);
}

unsigned int _data_hash_module(data_t *data) {
  return mod_hash(data_moduleval(data));
}

data_t * _data_resolve_module(data_t *mod, char *name) {
  return mod_resolve(data_moduleval(mod), name);
}

data_t * _data_call_module(data_t *mod, array_t *args, dict_t *kwargs) {
  return object_call(data_moduleval(mod) -> obj, args, kwargs);
}

data_t * data_create_module(module_t *module) {
  return data_create(Module, module);
}

/* ------------------------------------------------------------------------ */

module_t * mod_create(namespace_t *ns, name_t *name) {
  module_t   *ret;
  
  ret = NEW(module_t);
  if (ns_debug) {
    debug("  Creating module '%s'", name_tostring(name));
  }
  ret -> state = ModStateUninitialized;
  ret -> name = name_copy(name);
  ret -> ns = ns_copy(ns);
  ret -> obj = object_create(NULL);
  ret -> imports = set_create((cmp_t) mod_cmp);
  ret -> data.refs = 0;
  ret -> data.ptrval = ret;
  set_set_hash(ret -> imports, (hash_t) mod_hash);
  set_set_free(ret -> imports, (free_t) mod_free);
  set_set_tostring(ret -> imports, (tostring_t) mod_tostring);
  ret -> refs = 1;
  return ret;
}

void mod_free(module_t *mod) {
  if (mod && (--mod -> refs) <= 0) {
    object_free(mod -> obj);
    ns_free(mod -> ns);
    free(mod -> name);
    free(mod);
  }
}

module_t * mod_copy(module_t *module) {
  if (module) {
    module -> refs++;
  }
  return module;
}

char * mod_tostring(module_t *module) {
  if (module) {
    return name_tostring(module -> name);
  } else {
    return "mod:NULL";
  }
}

unsigned int mod_hash(module_t *mod) {
  return name_hash(mod -> name);
}

int mod_cmp(module_t *m1, module_t *m2) {
  return name_cmp(m1 -> name, m2 -> name);
}

int mod_cmp_name(module_t *mod, name_t *name) {
  return name_cmp(mod -> name, name);
}

data_t * mod_set(module_t *mod, script_t *script, array_t *args, dict_t *kwargs) {
  data_t *data;

  if (ns_debug) {
    debug("mod_set(%s, %s)", mod_tostring(mod), script_tostring(script));
  }
  data = script_create_object(script, args, kwargs);
  if (data_is_object(data)) {
    mod -> state = ModStateActive;
    if (ns_debug) {
      debug("  %s initialized: %s \n%s", mod_tostring(mod), 
            object_tostring(mod -> obj),
            dict_tostring(mod -> obj -> variables));
    }
  } else {
    assert(data_is_exception(data));
    object_free(mod -> obj);
  }
  return data;
}

object_t * mod_get(module_t *mod) {
  return (mod -> obj) ? object_copy(mod -> obj) : NULL;
}

void ** _mod_resolve_reducer(module_t *import, void **ctx) {
  char *needle = (char *) ctx[0];
  
  if (needle[0] && (name_size(import -> name) > 0) && 
      !strcmp(needle, name_first(import -> name))) {
    if (!ctx[1]) {
      ctx[1] = data_create(PartialNameMatch, needle);
    }
    _pnm_add((pnm_t *) (((data_t *) ctx[1]) -> ptrval), import);
  }
  return ctx;
}

data_t * mod_resolve(module_t *mod, char *name) {
  data_t   *ret;
  void     *ctx[2];
  module_t *root;
  data_t   *droot;
  
  if (ns_debug) {
    debug("mod_resolve('%s', '%s')", mod_tostring(mod), name);
  }
  
  /*
   * First see if the name sought is local to this module:
   */
  ret = object_resolve(mod -> obj, name);
  if (!ret) {
    
    if (ns_debug) {
      debug("mod_resolve('%s', '%s'): Not local.", mod_tostring(mod), name);
    }
    /*
     * Not local. Check if it's the first part of the name of one of the 
     * modules imported in this module:
     */
    ctx[0] = name;
    ctx[1] = NULL;
    set_reduce(mod -> imports, (reduce_t) _mod_resolve_reducer, ctx);
    ret = (data_t *) ctx[1];
    if (!ret && ns_debug) {
      debug("mod_resolve('%s', '%s'): Not an import", mod_tostring(mod), name);
    }
  }
  
  if (!ret && (name_size(mod -> name) > 0)) {
    
    if (ns_debug) {
      debug("mod_resolve('%s', '%s'): Check root module", mod_tostring(mod), name);
    }
    /*
     * Not local and does not start one of the imports. If this is not the 
     * root module, check the root module:
     */
    droot = ns_get(mod -> ns, NULL);
    if (!data_is_module(droot)) {
      error("mod_resolve(%s): root module not found", mod_tostring(mod));
    } else {
      ret = object_resolve(data_moduleval(droot) -> obj, name);
    }
    data_free(droot);
  }
  if (ns_debug) {
    debug("mod_resolve('%s', '%s'): %s", mod_tostring(mod), name, data_tostring(ret));
  }
  return ret;
}

data_t * mod_import(module_t *mod, name_t *name) {
  data_t *imp = ns_import(mod -> ns, name);

  if (data_is_module(imp)) {
    set_add(mod -> imports, data_moduleval(imp));
  }
  return imp;
}

module_t * mod_exit(module_t *mod, data_t *code) {
  return (ns_exit(mod -> ns, code)) ? mod : NULL;
}

data_t * mod_exit_code(module_t *mod) {
  return ns_exit_code(mod -> ns);
}

/* ------------------------------------------------------------------------ */

module_t * _ns_add_module(namespace_t *ns, name_t *name, module_t *mod) {
  if (ns_debug) {
    debug("_ns_add_module(%s, %s)", ns_tostring(ns), name_tostring(name));
  }
  dict_put(ns -> modules, strdup(name_tostring(name)), mod);
  return mod;
}

module_t * _ns_add(namespace_t *ns, name_t *name) {
  module_t *mod;
  
  mod = mod_create(ns, name);
  _ns_add_module(ns, name, mod);
  return mod;
}
  
module_t * _ns_get(namespace_t *ns, name_t *name) {
  return dict_get(ns -> modules, (name) ? name_tostring(name) : "");
}

data_t * _ns_load(namespace_t *ns, module_t *module, 
			   name_t *name, array_t *args, dict_t *kwargs) {
  data_t   *ret = NULL;
  data_t   *obj;
  data_t   *script;
  
  if (ns_debug) {
    debug("  Module '%s' not found - delegating to loader", name_tostring(name));
  }
  if (!module) {
    module = _ns_add(ns, name);
  }
  module -> state = ModStateLoading;
  script = ns -> import_fnc(ns -> import_ctx, module);
  if (data_is_script(script)) {
    obj = mod_set(module, data_scriptval(script), args, kwargs);
    if (data_is_object(obj)) {
      ret = data_create(Module, module);
      data_free(obj);
    } else {
      assert(data_is_exception(obj));
      ret = obj;
    }
    data_free(script);
  } else {
    assert(data_is_exception(script));
    ret = script;
  }
  return ret;
}

data_t * _ns_import(namespace_t *ns, name_t *name, array_t *args, dict_t *kwargs) {
  data_t   *ret = NULL;
  module_t *module = NULL;
  name_t   *dummy = NULL;

  if (!name) {
    dummy = name_create(0);
    name = dummy;
  }
  if (ns_debug) {
    debug("  Importing module '%s' into %s", name_tostring(name), ns_tostring(ns));
  }
  module = _ns_get(ns, name);
  if (module) {
    if (module -> state != ModStateUninitialized) {
      if (ns_debug) {
        debug("  Module '%s' %s in %s", name_tostring(name),
              ((module -> state == ModStateLoading) 
                ? "currently loading"
                : "already imported"), ns_tostring(ns));
      }
      ret = data_create(Module, module);
    } else {
      if (ns_debug) {
        debug("  Module found but it's Uninitialized. Somebody must be busy loading it.");
      }
    }
  } else {
    if (ns_debug) {
      debug("  Module not found");
    }
  }
  if (!ret) {
    ret = _ns_load(ns, module, name, args, kwargs);
  }
  name_free(dummy);
  return ret;
}

/* ------------------------------------------------------------------------ */

namespace_t * ns_create(char *name, void *importer, import_t import_fnc) {
  namespace_t *ret;

  assert(importer && import_fnc);
  if (ns_debug) {
    debug("  Creating root namespace");
  }

  ret = NEW(namespace_t);
  ret -> name = strdup(name);
  ret -> import_ctx = importer;
  ret -> import_fnc = import_fnc;
  ret -> exit_code = NULL;
  ret -> modules = strvoid_dict_create();
  dict_set_free_data(ret -> modules, (free_t) mod_free);
  dict_set_tostring_data(ret -> modules, (tostring_t) mod_tostring);
  ret -> str = NULL;
  ret -> refs = 1;
  return ret;
}

void ns_free(namespace_t *ns) {
  if (ns && (--ns -> refs <= 0)) {
    dict_free(ns -> modules);
    free(ns -> str);
    free(ns -> name);
    data_free(ns -> exit_code);
    free(ns);
  }
}

namespace_t * ns_copy(namespace_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
}

data_t * ns_execute(namespace_t *ns, name_t *name, array_t *args, dict_t *kwargs) {
  data_t *mod = _ns_import(ns, name, args, kwargs);
  data_t *obj;

  if (data_is_module(mod)) {
    obj = data_create(Object, data_moduleval(mod) -> obj);
    data_free(mod);
    return obj;
  } else {
    assert(data_is_exception(mod));
    return mod;
  }
}

data_t * ns_import(namespace_t *ns, name_t *name) {
  return _ns_import(ns, name, NULL, NULL);
}

data_t * ns_get(namespace_t *ns, name_t *name) {
  module_t *mod;
  
  mod = dict_get(ns -> modules, (name) ? name_tostring(name) : "");
  if (!mod || !mod -> obj -> constructor) {
      return data_exception(ErrorName,
                        "Import '%s' not found in %s", 
                        name_tostring(name), ns_tostring(ns));
  } else {
    return data_create(Module, mod);
  }
}

char * ns_tostring(namespace_t *ns) {
  if (ns) {
    if (!ns -> str) {
      asprintf(&ns -> str, "Namespace '%s'", ns -> name);
    }
    return ns -> str;
  } else {
    return "ns:NULL";
  }
}

namespace_t * ns_exit(namespace_t *ns, data_t *code) {
  ns -> exit_code = data_copy(code);
  if (ns_debug) {
    debug("Setting exit code %s", data_tostring(ns -> exit_code));
  }
  return ns;
}

data_t * ns_exit_code(namespace_t *ns) {
  return ns -> exit_code;
}
