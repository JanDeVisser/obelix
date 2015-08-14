/*
 * /obelix/src/parser/closure.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <boundmethod.h>
#include <closure.h>
#include <exception.h>
#include <namespace.h>
#include <nvp.h>
#include <script.h>
#include <stacktrace.h>
#include <thread.h>
#include <vm.h>
#include <wrapper.h>

static void         _closure_init(void) __attribute__((constructor(102)));

static closure_t *  _closure_create_closure_reducer(entry_t *, closure_t *);
static listnode_t * _closure_execute_instruction(instruction_t *, closure_t *);
static data_t *     _closure_get(closure_t *, char *);
static data_t *     _closure_start(closure_t *);

static char *       _closure_allocstring(closure_t *closure);
static void         _closure_free(closure_t *);

static data_t *     _closure_import(data_t *, char *, array_t *, dict_t *);

int Closure = -1;

static vtable_t _vtable_closure[] = {
  { .id = FunctionCmp,         .fnc = (void_t) closure_cmp },
  { .id = FunctionHash,        .fnc = (void_t) closure_hash },
  { .id = FunctionFree,        .fnc = (void_t) _closure_free },
  { .id = FunctionAllocString, .fnc = (void_t) _closure_allocstring },
  { .id = FunctionCall,        .fnc = (void_t) closure_execute },
  { .id = FunctionSet,         .fnc = (void_t) closure_set },
  { .id = FunctionResolve,     .fnc = (void_t) closure_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_closure[] = {
  { .type = -1,      .name = "import", .method = _closure_import, .argtypes = { NoType, NoType, NoType }, .minargs = 1, .varargs = 1 },
  { .type = NoType,  .name = NULL,     .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/* ------------------------------------------------------------------------ */

void _closure_init(void) {
  _methoddescr_closure[0].argtypes[0] = Name;
  Closure = typedescr_create_and_register(Closure, "closure",
					  _vtable_closure,
					  _methoddescr_closure);

}

/* ------------------------------------------------------------------------ */

/* -- C L O S U R E  S T A T I C  F U N C T I O N S ------------------------*/

closure_t * _closure_create_closure_reducer(entry_t *entry, closure_t *closure) {
  char           *name = (char *) entry -> key;
  data_t         *func = (data_t *) entry -> value;
  data_t         *value = NULL;
  bound_method_t *bm;
  object_t       *self;

  if (data_is_script(func)) {
    self = data_as_object(closure -> self);
    bm = bound_method_create(data_as_script(func), self);
    bm -> closure = closure;
    value = data_create(BoundMethod, bm);
  } else {
    /* Native function */
    /* TODO: Do we have a closure-like structure to bind the function to self? */
    value = data_copy(func);
  }
  if (value) {
    closure_set(closure, name, value);
  }
  return closure;
}

data_t * _closure_get(closure_t *closure, char *varname) {
  data_t *ret = NULL;

  if (closure -> self && !strcmp(varname, "self")) {
    ret = closure -> self;
  } else if (closure -> variables) {
    ret = (data_t *) dict_get(closure -> variables, varname);
  }
  if (!ret && closure -> params) {
    /*
     * We store the passed-in parameter values. If the parameter variable gets
     * re-assigned, the new value shadows the old one because it will be set
     * in the variables dict, not in the params one.
     */
    ret = (data_t *) dict_get(closure -> params, varname);
  }
  return ret;
}

data_t * _closure_start(closure_t *closure) {
  data_t      *ret;
  exception_t *e;
  data_t      *d;
  vm_t        *vm = vm_create(closure -> bytecode);

  ret = vm_execute(vm, d = data_create(Closure, closure));
  if (data_is_exception(ret)) {
    e = data_as_exception(ret);
    if ((e -> code == ErrorExit) && (e -> throwable)) {
      ns_exit(closure -> script -> mod -> ns, ret);
    } else if (e -> code == ErrorReturn) {
      data_free(ret);
      ret = (e -> throwable) ? data_copy(e -> throwable) : data_null();
    //} else if (e -> code == ErrorYield) {
    //  ret = data_create(Generator, closure, vm);
    }
    exception_free(e);
  }
  data_free(d);

  if (closure -> free_params) {
    /*
     * kwargs was assigned to closure -> params. Freeing the closure should
     * not free the params in that case, since the owner is responsible for
     * kwargs,
     */
    closure -> params = NULL;
  }
  return ret;
}

char * _closure_allocstring(closure_t *closure) {
  char *params;
  char *buf;

  params = (closure -> params && dict_size(closure -> params))
    ? dict_tostring_custom(closure -> params, "", "%s=%s", ",", "")
    : "";
  asprintf(&buf, "%s(%s)",
	   script_tostring(closure -> script),
	   params);
  return buf;
}

void _closure_free(closure_t *closure) {
  if (closure) {
    script_free(closure -> script);
    dict_free(closure -> variables);
    dict_free(closure -> params);
    data_free(closure -> self);
    data_free(closure -> thread);
  }
}

/* -- C L O S U R E  P U B L I C  F U N C T I O N S ------------------------*/

closure_t * closure_create(script_t *script, closure_t *up, data_t *self) {
  closure_t *ret;

  if (script_debug) {
    debug("Creating closure for script '%s'", script_tostring(script));
  }

  ret = data_new(Closure, closure_t);
  ret -> script = script_copy(script);
  ret -> bytecode = bytecode_copy(script -> bytecode);

  ret -> variables = NULL;
  ret -> params = NULL;
  ret -> up = up;
  ret -> self = data_copy(self);

  dict_reduce(script -> functions,
              (reduce_t) _closure_create_closure_reducer, ret);

  if (!up) {
    /* Import standard lib: */
    closure_import(ret, NULL);
  }
  return ret;
}

int closure_cmp(closure_t *c1, closure_t *c2) {
  return name_cmp(c1 -> script -> name, c2 -> script -> name);
}

unsigned int closure_hash(closure_t *closure) {
  return hashptr(closure);
}

data_t * closure_import(closure_t *closure, name_t *module) {
  data_t *ret;

  if (script_debug) {
    debug("Importing '%s'", name_tostring(module));
  }
  ret = mod_import(closure -> script -> mod, module);
}

data_t * closure_set(closure_t *closure, char *name, data_t *value) {
  if (script_debug) {
    if (strcmp(name, "self")) {
      debug("  Setting local '%s' = '%s' in closure for %s",
            name, data_tostring(value), closure_tostring(closure));
    } else {
      debug("  Setting local '%s' in closure for %s",
            name, closure_tostring(closure));
    }
  }
  if (!closure -> variables) {
    closure -> variables = strdata_dict_create();
  }
  dict_put(closure -> variables, strdup(name), data_copy(value));
  return value;
}

data_t * closure_get(closure_t *closure, char *varname) {
  data_t *ret = _closure_get(closure, varname);

  if (ret) {
    ret = data_copy(ret);
  } else {
    ret = data_exception(ErrorName,
                     "Closure '%s' has no attribute '%s'",
                     closure_tostring(closure),
                     varname);
  }
  return ret;
}

int closure_has(closure_t *closure, char *name) {
  int ret;

  ret = (closure -> self && !strcmp(name, "self")) ||
        (closure -> variables && dict_has_key(closure -> variables, name)) ||
        (closure -> params && dict_has_key(closure -> params, name));
  if (script_debug) {
    debug("   closure_has('%s', '%s'): %d", closure_tostring(closure), name, ret);
  }
  return ret;
}

data_t * closure_resolve(closure_t *closure, char *name) {
  data_t  *ret;

  ret = _closure_get(closure, name);
  if (!ret) {
    if (closure -> up) {
      if (!strcmp(name, "^") ||
          !strcmp(name, name_last(script_fullname(closure -> up -> script)))) {
        ret = data_create(Closure, closure -> up);
      } else {
        ret = closure_resolve(closure -> up, name);
      }
    } else {
      ret = mod_resolve(closure -> script -> mod, name);
    }
  }
  if (script_debug) {
    debug("   closure_resolve('%s', '%s'): %s",
          closure_tostring(closure), name, data_tostring(ret));
  }
  return data_copy(ret);
}

data_t * closure_execute(closure_t *closure, array_t *args, dict_t *kwargs) {
  int        ix;
  data_t    *param;
  script_t  *script;
  object_t  *self;
  pthread_t  thr_id;
  char      *str;

  script = closure -> script;
  closure -> free_params = FALSE;
  if (script -> params && array_size(script -> params)) {
    if (!args || (array_size(script -> params) > array_size(args))) {
      return data_exception(ErrorArgCount,
                        "Function %s takes %d arguments, %d provided",
                        name_tostring(script -> name),
                        array_size(script -> params),
                        (args) ? array_size(args) : 0);
    }
    if ((script -> type == STASync) || !kwargs) {
      closure -> params = strdata_dict_create();
      closure -> free_params = TRUE;
    } else {
      closure -> params = kwargs;
    }
    if ((script -> type == STASync) && kwargs) {
      dict_reduce(kwargs, (reduce_t) data_put_all_reducer, closure -> params);
    }
    for (ix = 0; ix < array_size(args); ix++) {
      param = data_copy(data_array_get(args, ix));
      dict_put(closure -> params,
               (char *) array_get(script -> params, ix),
               param);
    }
  }

  if (script -> type == STASync) {
    return (data_t *) thread_new(closure_tostring(closure),
                                 (threadproc_t) _closure_start,
                                 closure_copy(closure));
  } else {
    return _closure_start(closure);
  }
}

/* -- C L O S U R E  D A T A  M E T H O D S --------------------------------*/

data_t * _closure_import(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return closure_import(data_as_closure(self),
                        data_as_name(data_array_get(args, 0)));
}
