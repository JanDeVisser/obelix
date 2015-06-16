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
#include <set.h>

static void          _namespace_init(void) __attribute__((constructor));

extern void          _mod_free(module_t *);
extern char *        _mod_tostring(module_t *);
static data_t *      _mod_call(module_t *, array_t *, dict_t *);

static namespace_t * _ns_create(void);
extern void          _ns_free(namespace_t *);
extern char *        _ns_tostring(namespace_t *);
static module_t *    _ns_add(namespace_t *, name_t *);
static data_t *      _ns_delegate_up(namespace_t *, module_t *, name_t *, array_t *, dict_t *);
static data_t *      _ns_delegate_load(namespace_t *, module_t *, name_t *, array_t *, dict_t *);
static data_t *      _ns_import(namespace_t *, name_t *, array_t *, dict_t *);

int ns_debug = 0;
int Module = -1;
int Namespace = -1;

vtable_t _vtable_namespace[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _ns_free },
  { .id = FunctionToString, .fnc = (void_t) _ns_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

vtable_t _vtable_module[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) mod_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _mod_free },
  { .id = FunctionToString, .fnc = (void_t) _mod_tostring },
  { .id = FunctionHash,     .fnc = (void_t) mod_hash },
  { .id = FunctionResolve,  .fnc = (void_t) mod_resolve },
  { .id = FunctionCall,     .fnc = (void_t) _mod_call },
  { .id = FunctionNone,     .fnc = NULL }
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
static int         _pnm_cmp(pnm_t *, pnm_t *);
static char *      _pnm_tostring(pnm_t *);
static data_t *    _pnm_resolve(pnm_t *, char *);
static data_t *    _pnm_call(pnm_t *, array_t *, dict_t *);
static data_t *    _pnm_resolve(pnm_t *, char *);
static data_t *    _pnm_set(pnm_t *, char *, data_t *);

int PartialNameMatch = -1;

static vtable_t _vtable_pnm[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) _pnm_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _pnm_free },
  { .id = FunctionToString, .fnc = (void_t) _pnm_tostring },
  { .id = FunctionResolve,  .fnc = (void_t) _pnm_resolve },
  { .id = FunctionCall,     .fnc = (void_t) _pnm_call },
  { .id = FunctionSet,      .fnc = (void_t) _pnm_set },
  { .id = FunctionNone,     .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _namespace_init(void) {
  logging_register_category("namespace", &ns_debug);
  Module = typedescr_create_and_register(Module, "module", _vtable_module, NULL);
  Namespace = typedescr_create_and_register(Namespace, "namespace", _vtable_namespace, NULL);
  PartialNameMatch = typedescr_create_and_register(PartialNameMatch, "pnm", _vtable_pnm, NULL);
}

/* -- Partial Name Match data functions ----------------------------------- */

pnm_t * _pnm_create(char *name) {
  pnm_t *ret;
  
  ret = data_new(PartialNameMatch, pnm_t);
  ret -> name = name_create(1, name);
  ret -> matches = data_set_create();
  return ret;
}

void _pnm_free(pnm_t *pnm) {
  if (pnm) {
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
      pnm -> match_lost = data_set_create();
    }
    set_add(pnm -> match_lost, mod);
  }
  return pnm;
}

int _pnm_cmp(pnm_t *pnm1, pnm_t *pnm2) {
  return name_cmp(pnm1 -> name, pnm2 -> name);
}

char * _pnm_tostring(pnm_t *pnm) {
  return name_tostring(pnm -> name);
}

data_t * _pnm_resolve(pnm_t *pnm, char *name) {
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
    return (data_t *) pnm;
  }
}

data_t * _pnm_call(pnm_t *pnm, array_t *params, dict_t *kwargs) {
  module_t *mod = _pnm_find_mod(pnm);
  
  return (mod)
    ? _mod_call(mod, params, kwargs) 
    : data_exception(ErrorName, "Trouble locating '%s'", name_tostring(pnm -> name));
}

data_t * _pnm_set(pnm_t *pnm, char *name, data_t *value) {
  module_t *mod = _pnm_find_mod(pnm);
  
  return (mod)
    ? object_set(mod -> obj, name, value) 
    : data_exception(ErrorName, "Trouble locating '%s'", name_tostring(pnm -> name));
}

/* -- M O D U L E  D A T A  F U N C T I O N S ----------------------------- */

void _mod_free(module_t *mod) {
  if (mod) {
    object_free(mod -> obj);
    ns_free(mod -> ns);
    free(mod -> name);
    free(mod);
  }
}

char * _mod_tostring(module_t *module) {
  if (module) {
    return name_tostring(module -> name);
  } else {
    return "mod:NULL";
  }
}

data_t * _mod_call(module_t *mod, array_t *args, dict_t *kwargs) {
  return object_call(mod -> obj, args, kwargs);
}

/* ------------------------------------------------------------------------ */

module_t * mod_create(namespace_t *ns, name_t *name) {
  module_t   *ret;
  
  ret = data_new(Module, module_t);
  if (ns_debug) {
    debug("  Creating module '%s'", name_tostring(name));
  }
  ret -> state = ModStateUninitialized;
  ret -> name = name_copy(name);
  ret -> ns = ns_copy(ns);
  ret -> obj = object_create(NULL);
  ret -> imports = data_set_create();
  return ret;
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
      ctx[1] = (data_t *) _pnm_create(needle);
    }
    _pnm_add((pnm_t *) ctx[1], import);
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
      ret = object_resolve(data_as_module(droot) -> obj, name);
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
    set_add(mod -> imports, data_as_module(imp));
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
    obj = mod_set(module, data_as_script(script), args, kwargs);
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

void _ns_free(namespace_t *ns) {
  if (ns) {
    dict_free(ns -> modules);
    free(ns -> name);
    data_free(ns -> exit_code);
    free(ns);
  }
}

char * _ns_tostring(namespace_t *ns) {
  return ns -> name;
}

/* ------------------------------------------------------------------------ */

namespace_t * ns_create(char *name, void *importer, import_t import_fnc) {
  namespace_t *ret;

  assert(importer && import_fnc);
  if (ns_debug) {
    debug("  Creating root namespace");
  }

  ret = data_new(Namespace, namespace_t);
  ret -> name = strdup(name);
  ret -> import_ctx = importer;
  ret -> import_fnc = import_fnc;
  ret -> exit_code = NULL;
  ret -> modules = strdata_dict_create();
  return ret;
}

data_t * ns_execute(namespace_t *ns, name_t *name, array_t *args, dict_t *kwargs) {
  data_t *mod = _ns_import(ns, name, args, kwargs);
  data_t *obj;

  if (data_is_module(mod)) {
    obj = data_create(Object, data_as_module(mod) -> obj);
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
