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

static data_t *      _data_new_module(data_t *, va_list);
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
  { .id = FunctionNew,      .fnc = (void_t) _data_new_module },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_module },
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

void _namespace_init(void) {
  logging_register_category("namespace", &ns_debug);
  typedescr_register(&typedescr_module);
}

/*
 * Module data functions
 */

data_t * _data_new_module(data_t *ret, va_list arg) {
  module_t *module;

  module = va_arg(arg, module_t *);
  ret -> ptrval = module;
  ((module_t *) ret -> ptrval) -> refs++;
  return ret;
}

data_t * _data_copy_module(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((module_t *) target -> ptrval) -> refs++;
  return target;
}

int _data_cmp_module(data_t *d1, data_t *d2) {
  module_t *s1;
  module_t *s2;

  s1 = (module_t *) d1 -> ptrval;
  s2 = (module_t *) d2 -> ptrval;
  return strcmp(s1 -> name, s2 -> name);
}

char * _data_tostring_module(data_t *d) {
  return mod_tostring((module_t *) d -> ptrval);
}

unsigned int _data_hash_module(data_t *data) {
  return object_hash(data_moduleval(data) -> obj);
}

data_t * _data_resolve_module(data_t *mod, char *name) {
  return mod_resolve(data_moduleval(mod), name);
}

data_t * _data_call_module(data_t *mod, array_t *args, dict_t *kwargs) {
  module_t *m = data_moduleval(mod);
  
  return object_call(m -> obj, args, kwargs);
}

data_t * data_create_module(module_t *module) {
  return data_create(Module, module);
}

/* ------------------------------------------------------------------------ */

module_t * _mod_copy_object(module_t *mod, object_t *obj) {
  object_free(mod -> obj);
  mod -> obj = object_copy(obj);
  mod -> state = ModStateActive;
  if (ns_debug) {
    debug("  module '%s' initialized", mod -> name);
  }
  return mod;
}

/* ------------------------------------------------------------------------ */

module_t * mod_create(name_t *name) {
  module_t   *ret;
  
  ret = NEW(module_t);
  if (ns_debug) {
    debug("  Creating module '%s'", name_tostring(name));
  }
  ret -> state = ModStateUninitialized;
  ret -> name = strdup(name_tostring(name));
  ret -> contents = strdata_dict_create();
  ret -> obj = object_create(NULL);
  ret -> str = NULL;
  ret -> refs = 1;
  return ret;
}

void mod_free(module_t *mod) {
  if (mod) {
    mod -> refs--;
    if (mod -> refs <= 0) {
      dict_free(mod -> contents);
      object_free(mod -> obj);
      free(mod -> name);
      free(mod);
    }
  }
}

module_t * mod_copy(module_t *module) {
  module -> refs++;
  return module;
}

char * mod_tostring(module_t *module) {
  if (!module -> str) {
    asprintf(&module -> str, "<<module %s>>", module -> name);
  }
  return module -> str;
}

int mod_has_module(module_t *mod, char *name) {
  return dict_has_key(mod -> contents, name);
}

data_t * mod_get_module(module_t *mod, char *name) {
  return (data_t *) dict_get(mod -> contents, name);
}

module_t * mod_add_module(module_t *mod, name_t *name, module_t *sub) {
  dict_put(mod -> contents, strdup(name_last(name)), data_create(Module, sub));
  return sub;
}

data_t * mod_set(module_t *mod, script_t *script, array_t *args, dict_t *kwargs) {
  data_t *data;

  if (ns_debug) {
    debug("mod_set(%s, %s)", mod_tostring(mod), script_tostring(script));
  }
  if (!mod -> obj) {
    // Prevent endless loops -
    mod -> obj = object_create(NULL);
  }
  data = script_create_object(script, args, kwargs);
  if (data_is_object(data)) {
    _mod_copy_object(mod, data_objectval(data));
  } else {
    error("ERROR initializing module '%s': %s", mod -> name, data_tostring(data));
  }
  return data;
}

object_t * mod_get(module_t *mod) {
  return (mod -> obj) ? object_copy(mod -> obj) : NULL;
}

data_t * mod_resolve(module_t *mod, char *name) {
  data_t *ret;
  
  /* 
   * You can shadow subpackages by explicitely having an attribute in the 
   * object.
   */
  ret = object_resolve(mod -> obj, name);
  if (!ret) {
    ret = mod_get_module(mod, name);
  }
}

/* ------------------------------------------------------------------------ */

namespace_t * _ns_create(void) {
  namespace_t *ret;

  ret = NEW(namespace_t);
  ret -> root = NULL;
  ret -> import_ctx = NULL;
  ret -> up = NULL;
  return ret;
}

  
module_t * _ns_add_module(namespace_t *ns, name_t *name, module_t *mod) {
  name_t   *current = name_create(0);
  int       ix;
  char     *n;
  module_t *node;
  module_t *sub;

  if (ns_debug) {
    debug("_ns_add_module(%s, %s)", ns_tostring(ns), name_tostring(name));
  }
  if (!name_size(name)) {
    assert(!ns -> root);
    ns -> root = data_create(Module, mod);
  } else {
    assert(ns -> root);  
    node = data_moduleval(ns -> root);
    for (ix = 0; ix < name_size(name) - 1; ix++) {
      n = name_get(name, ix);
      name_extend(current, n);
      sub = data_moduleval(mod_get_module(node, n));
      if (!sub) {
        sub = mod_create(name);
        sub = mod_add_module(node, name_copy(current), sub);
      }
      node = sub;
    }
    mod_add_module(node, name, mod);
  }
  name_free(current);
  return mod;
}

module_t * _ns_add(namespace_t *ns, name_t *name) {
  module_t *mod;
  
  mod = mod_create(name);
  mod -> state = ModStateLoading;
  _ns_add_module(ns, name, mod);
  return mod;
}
  
module_t * _ns_get(namespace_t *ns, name_t *name) {
  int       ix;
  char     *n;
  module_t *node;
  
  node = data_moduleval(ns -> root);
  for (ix = 0; node && (ix < name_size(name)); ix++) {
    n = name_get(name, ix);
    node = data_moduleval(mod_get_module(node, n));
  }
  if (!node && ns -> up) {
    node = _ns_get(ns -> up, name);
    if (node) {
      _ns_add_module(ns, name, node);
    }
  }
  return node;
}

data_t * _ns_delegate_up(namespace_t *ns, module_t *module, name_t *name,
			 array_t *args, dict_t *kwargs) {
  data_t   *updata;
  module_t *upmod;
  
  if (ns_debug) {
    debug("  Module '%s' not found - delegating to higher level %s", 
          name_tostring(name), ns_tostring(ns));
  }
  updata = _ns_import(ns -> up, name, args, kwargs);
  if (data_is_module(updata)) {
    upmod = data_moduleval(updata);
    if (!module) {
      module = _ns_add(ns, name);
    }
    _mod_copy_object(module, upmod -> obj);
    return data_create(Module, module);
  } else {
    return updata;
  }
}

data_t * _ns_delegate_load(namespace_t *ns, module_t *module, 
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
  script = ns -> import_fnc(ns -> import_ctx, name);
  if (data_is_script(script)) {
    obj = mod_set(module, data_scriptval(script), args, kwargs);
    if (data_is_object(obj)) {
      ret = data_create(Module, module);
      data_free(obj);
    } else {
      ret = obj; /* !data_is_object(obj) => Error */
    }
    data_free(script);
  } else {
    ret = script; /* !data_is_script(script) => Error */
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
        debug("  Module found but it's Uninitialized");
      }
    }
  } else {
    if (ns_debug) {
      debug("  Module not found");
    }
  }
  if (!ret) {
    if (!ns -> import_ctx) {
      return _ns_delegate_up(ns, module, name, args, kwargs);
    } else {
      return _ns_delegate_load(ns, module, name, args, kwargs);
    }
  }
  name_free(dummy);
  return ret;
}

/* ------------------------------------------------------------------------ */

namespace_t * ns_create(namespace_t *up) {
  namespace_t *ret;

  assert(up);
  if (ns_debug) {
    debug("  Creating subordinate namespace level [%d]", up -> level + 1);
  }
  ret = _ns_create();
  ret -> import_ctx = NULL;
  ret -> import_fnc = NULL;
  ret -> up = up;
  ret -> level = up -> level + 1;
  return ret;
}

namespace_t * ns_create_root(void *importer, import_t import_fnc) {
  namespace_t *ret;

  assert(importer && import_fnc);
  if (ns_debug) {
    debug("  Creating root namespace");
  }
  ret = _ns_create();
  ret -> import_ctx = importer;
  ret -> import_fnc = import_fnc;
  ret -> up = NULL;
  ret -> level = 0;
  return ret;
}

void ns_free(namespace_t *ns) {
  if (ns) {
    data_free(ns -> root);
    free(ns -> str);
    free(ns);
  }
}

data_t * ns_execute(namespace_t *ns, name_t *name, array_t *args, dict_t *kwargs) {
  data_t *mod = _ns_import(ns, name, args, kwargs);
  data_t *obj;

  if (data_is_module(mod)) {
    obj = data_create(Object, data_moduleval(mod) -> obj);
    data_free(mod);
    return obj;
  } else {
    assert(data_is_error(mod));
    return mod;
  }
}

data_t * ns_import(namespace_t *ns, name_t *name) {
  return _ns_import(ns, name, NULL, NULL);
}

data_t * ns_get(namespace_t *ns, name_t *name) {
  int       ix;
  char     *n;
  module_t *node;
  
  node = data_moduleval(ns -> root);
  for (ix = 0; ix < name_size(name); ix++) {
    n = name_get(name, ix);
    node = data_moduleval(mod_get_module(node, n));
    if (!node) break;
  }
  if (!node || !node -> obj -> constructor) {
      return data_error(ErrorName,
                        "Import '%s' not found in %s", 
                        name_tostring(name), ns_tostring(ns));
  } else {
    return data_create(Module, node);
  }
}

data_t * ns_resolve(namespace_t *ns, char *name) {
  return mod_resolve(data_moduleval(ns -> root), name);
}

char * ns_tostring(namespace_t *ns) {
  if (ns) {
    free(ns -> str);
    asprintf(&ns -> str, "namespace level [%d]", ns -> level);
    return ns -> str;
  } else {
    return "ns:NULL";
  }
}
