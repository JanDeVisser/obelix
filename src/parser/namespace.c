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
#include <error.h>
#include <namespace.h>

static data_t *         _data_new_module(data_t *, va_list);
static data_t *         _data_copy_module(data_t *, data_t *);
static int              _data_cmp_module(data_t *, data_t *);
static char *           _data_tostring_module(data_t *);
static unsigned int     _data_hash_module(data_t *);
/* static data_t *         _data_parse_module(char *); */
static data_t *         _data_execute_module(data_t *, char *, array_t *, dict_t *);

static module_t *       _mod_create(array_t *);
static module_t *       _mod_create_dummy(array_t *);
static char *           _ns_get_name_str(array_t *);
static namespace_t *    _ns_create(void);

int ns_debug = 0;

/*
  new:      (new_t)      _data_new_module,
  copy:     (copydata_t) _data_copy_module,
  cmp:      (cmp_t)      _data_cmp_module,
  free:     (free_t)     mod_free,
  tostring: (tostring_t) _data_tostring_module,
  hash:     (hash_t)     _data_hash_module,
  parse:                  NULL, // _data_parse_module,
*/

vtable_t _vtable_module[] = {
  { .id = MethodNew,      .fnc = (void_t) _data_new_module },
  { .id = MethodCopy,     .fnc = (void_t) _data_copy_module },
  { .id = MethodCmp,      .fnc = (void_t) _data_cmp_module },
  { .id = MethodFree,     .fnc = (void_t) mod_free },
  { .id = MethodToString, .fnc = (void_t) _data_tostring_module },
  { .id = MethodParse,    .fnc = NULL }, /* FIXME */
  { .id = MethodHash,     .fnc = (void_t) _data_hash_module },
  { .id = MethodNone,     .fnc = NULL }
};

static typedescr_t typedescr_module = {
  .type       = Module,
  .typecode   = "D",
  .type_name  = "mod",
  .vtable     = _vtable_module
};



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
  static char buf[128];

  snprintf(buf, 128, "<<module %s>>", ((module_t *) d -> ptrval) -> name);
  return buf;
}

unsigned int _data_hash_module(data_t *data) {
  return object_hash(data_moduleval(data) -> obj);
}

data_t * _data_execute_module(data_t *self, char *name, array_t *params, dict_t *kwargs) {
  return object_execute(data_moduleval(self) -> obj, name, params, kwargs);
}

data_t * data_create_module(module_t *module) {
  return data_create(Module, module);
}

/* ------------------------------------------------------------------------ */

module_t * _mod_create(array_t *name) {
  static int  type_module = -1;
  module_t   *ret;
  char       *n = _ns_get_name_str(name);
  
  if (type_module < 0) {
    type_module = typedescr_register(typedescr_module);
  }
  
  ret = NEW(module_t);
  ret -> name = _ns_get_name_str(name);
  ret -> refs = 0;
  free(n);
  return ret;
}

module_t * _mod_create_dummy(array_t *name) {
  module_t   *ret;

  ret = _mod_create(name);
  if (ns_debug) {
    debug("  Creating dummy module '%s'", ret -> name);
  }
  ret -> obj = object_create(NULL);
  return ret;
}

/* ------------------------------------------------------------------------ */

module_t * mod_create(script_t *script, array_t *name) {
  module_t   *ret;
  data_t     *data;

  ret = _mod_create(name);
  if (ns_debug) {
    debug("  Creating module '%s'", ret -> name);
  }
  data = script_create_object(script, NULL, NULL);
  if (data_is_object(data)) {
    ret -> obj = object_copy(data_objectval(data));
    if (ns_debug) {
      debug("  module '%s' created", ret -> name);
    }
  } else {
    error("ERROR creating module '%s': %s", ret -> name, data_tostring(data));
    mod_free(ret);
    ret = NULL;
  }
  return ret;
}

void mod_free(module_t *mod) {
  if (mod) {
    mod -> refs--;
    if (mod -> refs <= 0) {
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

/* ------------------------------------------------------------------------ */

char * _ns_get_name_str(array_t *name) {
  str_t *name_str;
  char  *n;

  if (!name || !array_size(name)) {
    n = strdup("");
  } else {
    name_str = array_join(name, ".");
    n = strdup(str_chars(name_str));
    str_free(name_str);
  }
  return n;
}

namespace_t * _ns_create(void) {
  namespace_t *ret;

  ret = NEW(namespace_t);
  ret -> contents = strdata_dict_create();
  ret -> import_ctx = NULL;
  ret -> up = NULL;
  return ret;
}

/* ------------------------------------------------------------------------ */

namespace_t * ns_create(namespace_t *up) {
  namespace_t *ret;

  assert(up);
  if (ns_debug) {
    debug("  Creating subordinate namespace");
  }
  ret = _ns_create();
  ret -> import_ctx = NULL;
  ret -> up = up;
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
  return ret;
}

void ns_free(namespace_t *ns) {
  if (ns) {
    dict_free(ns -> contents);
    free(ns);
  }
}

data_t * ns_import(namespace_t *ns, array_t *name) {
  char     *n;
  data_t   *data;
  module_t *mod;
  data_t   *script;

  n = _ns_get_name_str(name);
  if (ns_debug) {
    debug("  Importing module '%s'", n);
  }
  if (ns_has(ns, n)) {
    if (ns_debug) {
      debug("  Module '%s' already imported", n);
    }
    data = data_copy(data_dict_get(ns -> contents, n));
  } else {
    if (!ns -> import_ctx) {
      if (ns_debug) {
        debug("  Module '%s' not found - delegating to higher level namespace", n);
      }
      data = ns_import(ns -> up, name);
    } else {
      if (ns_debug) {
        debug("  Module '%s' not found - delegating to loader", n);
      }
      script = ns -> import_fnc(ns -> import_ctx, name);
      if (data_is_script(script)) {
        /* 
         * Create a dummy module and store that in the dictionary while the
         * script is executed. This prevents endless loops.
         */
        dict_put(ns -> contents, strdup(n), 
                 data_create_module(_mod_create_dummy(name)));
        mod = mod_create(data_scriptval(script), name);
        dict_remove(ns -> contents, n);

        if (mod) {
          data = data_create_module(mod);
        } else {
          data = data_error(ErrorType,
                            "Could not load module '%s'", n);
        }
      } else {
        data = script;
      }
    }
    if (data_is_module(data)) {
      if (ns_debug) {
        debug("  Adding module '%s' to inventory", n);
      }
      dict_put(ns -> contents, strdup(n), data_copy(data));
    } else {
      error("ERROR importing module '%s': %s", n, data_tostring(data));
    }
  }
  free(n);
  return data;
}

data_t * ns_get(namespace_t *ns, array_t *name) {
  char   *n;
  str_t  *name_str = NULL;
  data_t *ret;

  if (!name || !array_size(name)) {
    n = strdup("");
  } else {
    name_str = array_join(name, ".");
    n = strdup(str_chars(name_str));
  }
  ret = ns_gets(ns, n);
  free(n);
  str_free(name_str);
  return ret;
}

data_t * ns_gets(namespace_t *ns, char *name) {
  data_t *data;

  data = data_dict_get(ns -> contents, name);
  if (!data) {
    data = data_error(ErrorName,
                      "Import '%s' not found in namespace", name);
  } else {
    data = data_copy(data);
  }
  return data;
}

int ns_has(namespace_t *ns, char *name) {
  int ret;

  ret = dict_has_key(ns -> contents, name);
  if (ns_debug) {
    debug("  ns_has('%s') = %d", name, ret);
  }
  return ret;
}

data_t * ns_resolve(namespace_t *ns, array_t *name) {
  array_t  *scope;
  char     *n;
  data_t   *mod;
  data_t   *ret = NULL;
  object_t *module;
  char     *scope_str;

  assert(name && array_size(name));
  scope = array_slice(name, 0, -2);
  n = (char *) array_get(name, -1);

  scope_str = _ns_get_name_str(scope);
  if (ns_has(ns, scope_str)) {
    mod = data_dict_get(ns -> contents, scope_str);
    module = data_moduleval(mod) -> obj;
    if (object_has(module, n)) {
      ret = data_create_object(module);
    }
  }
  if (!ret) {
    ret = data_error(ErrorName,
                     "Could not resolve '%s.%s' in namespace", scope_str, n);
  }
  free(scope_str);
  array_free(scope);
  return ret;
}
