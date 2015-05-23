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

#include <string.h>

#include <exception.h>
#include <namespace.h>
#include <nvp.h>
#include <script.h>
#include <stacktrace.h>
#include <thread.h>
#include <wrapper.h>

static void             _closure_init(void) __attribute__((constructor(102)));

static closure_t *      _closure_create_closure_reducer(entry_t *, closure_t *);
static listnode_t *     _closure_execute_instruction(instruction_t *, closure_t *);
static data_t *         _closure_get(closure_t *, char *);
static data_t *         _closure_start(closure_t *);

static vtable_t _wrapper_vtable_closure[] = {
  { .id = FunctionCopy,     .fnc = (void_t) closure_copy },
  { .id = FunctionCmp,      .fnc = (void_t) closure_cmp },
  { .id = FunctionHash,     .fnc = (void_t) closure_hash },
  { .id = FunctionFree,     .fnc = (void_t) closure_free },
  { .id = FunctionToString, .fnc = (void_t) closure_tostring },
  { .id = FunctionResolve,  .fnc = (void_t) closure_resolve },
  { .id = FunctionCall,     .fnc = (void_t) closure_execute },
  { .id = FunctionSet,      .fnc = (void_t) closure_set },
  { .id = FunctionNone,     .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _closure_init(void) {
  wrapper_register(Closure, "closure", _wrapper_vtable_closure);
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
    self = data_objectval(closure -> self);
    bm = bound_method_create(data_scriptval(func), self);
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

listnode_t * _closure_execute_instruction(instruction_t *instr, closure_t *closure) {
  data_t       *ret;
  char         *label = NULL;
  listnode_t   *node = NULL;
  data_t       *catchpoint = NULL;
  int           datatype;
  exception_t  *ex;
  data_t       *ex_data;

  ret = ns_exit_code(closure -> script -> mod -> ns);
  
  /*
   * If we're exiting, we still need to unwind the context stack, but no other
   * instructions are executed.
   * 
   * FIXME: What we're effectively doing here is disable __exit__ handlers in
   * obelix objects, since they will pick up the exit code.
   */
  if (!ret || (instr -> type == ITLeaveContext)) {
    if (instr -> line > 0) {
      closure -> line = instr -> line;
    }
    ret = instruction_execute(instr, closure);
  }
  if (ret) {
    datatype = data_type(ret);
    if (datatype == String) {
      label = strdup(data_tostring(ret));
    } else {
      if (datatype == Exception) {
        ex = data_exceptionval(ret);
      } else {
        ex_data = data_exception(ErrorInternalError,
                                 "Instruction '%s' returned %s '%s'",
                                 instruction_tostring(instr),
                                 data_typedescr(ret) -> type_name,
                                 data_tostring(ret));
        ex = data_exceptionval(ex_data);
        ret = ex_data;
      }
      ex -> trace = data_create(Stacktrace, stacktrace_create());
      if (datastack_depth(closure -> catchpoints)) {
        catchpoint = datastack_peek(closure -> catchpoints);
        label = strdup(data_charval(data_nvpval(catchpoint) -> name));
      } else {
        /*
         * This is ugly. There should be a way to interrupt list_process.
         * What we do here is get the last node (which exists, because 
         * otherwise we wouldn't be here, processing a node), fondle that 
         * node to get it's next pointer (which points to the tail marker
         * node), and return that. 
         */
        node = list_tail_pointer(closure -> script -> instructions) -> next;
      }
      closure_push(closure, data_copy(ret));
    }
    data_free(ret);
  }
  if (label) {
    if (script_debug) {
      debug("  Jumping to '%s'", label);
    }
    node = (listnode_t *) dict_get(closure -> script -> labels, label);
    assert(node);
    free(label);
  }
  return node;
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

  ret = data_thread_push_stackframe(data_create_closure(closure));
  if (!data_is_exception(ret)) {
    list_process(closure -> script -> instructions,
                (reduce_t) _closure_execute_instruction,
                closure);
    ret = (datastack_notempty(closure -> stack)) ? closure_pop(closure) : data_null();
    if (script_debug) {
      debug("    Execution of %s done: %s", closure_tostring(closure), data_tostring(ret));
    }
    if (data_is_exception(ret)) {
      e = data_exceptionval(ret);
      if ((e -> code == ErrorExit) && (data_exceptionval(ret) -> throwable)) {
        ns_exit(closure -> script -> mod -> ns, ret);
      }
    }
  }
  data_thread_pop_stackframe();
  
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

/* -- C L O S U R E  P U B L I C  F U N C T I O N S ------------------------*/

closure_t * closure_create(script_t *script, closure_t *up, data_t *self) {
  closure_t *ret;
  int        depth;

  if (script_debug) {
    debug("Creating closure for script '%s'", script_tostring(script));
  }
  
  ret = NEW(closure_t);
  ret -> script = script_copy(script);

  ret -> variables = NULL;
  ret -> params = NULL;
  ret -> stack = NULL;
  ret -> catchpoints = NULL;
  ret -> up = up;
  ret -> depth = depth;
  ret -> self = data_copy(self);

  dict_reduce(script -> functions, 
              (reduce_t) _closure_create_closure_reducer, ret);
  ret -> refs++;
  
  if (!up) {
    /* Import standard lib: */
    closure_import(ret, NULL);
  }
  return ret;
}

void closure_free(closure_t *closure) {
  if (closure) {
    closure -> refs--;
    if (closure <= 0) {
      script_free(closure -> script);
      datastack_free(closure -> stack);
      datastack_free(closure -> catchpoints);
      dict_free(closure -> variables);
      dict_free(closure -> params);
      data_free(closure -> self);
      data_free(closure -> thread);
      free(closure -> str);
      free(closure);
    }
  }
}

closure_t * closure_copy(closure_t *closure) {
  if (closure) {
    closure -> refs++;
  }
  return closure;
}

char * closure_tostring(closure_t *closure) {
  char *params;
  
  if (!closure -> str) {
    params = (closure -> params && dict_size(closure -> params)) 
               ? dict_tostring_custom(closure -> params, "", "%s=%s", ",", "")
               : strdup("");
    asprintf(&closure -> str, "%s(%s)",
             script_tostring(closure -> script),
             params);
    free(params);
  }
  return closure -> str;
           
}

int closure_cmp(closure_t *c1, closure_t *c2) {
  return name_cmp(c1 -> script -> name, c2 -> script -> name);
}

unsigned int closure_hash(closure_t *closure) {
  return hashptr(closure);
}

data_t * closure_pop(closure_t *closure) {
  return datastack_pop(closure -> stack);
}

data_t * closure_peek(closure_t *closure) {
  return datastack_peek(closure -> stack);
}

/**
 * @brief Pushes a data value onto the closure run-time stack.
 * 
 * @param closure The closure on whose stack the value should be
 * pushed.
 * @param value The data value to push onto the closure's stack.
 * @return closure_t* The same closure as the one passed in.
 */
closure_t * closure_push(closure_t *closure, data_t *value) {
  datastack_push(closure -> stack, data_copy(value));
  return closure;
}

data_t * closure_import(closure_t *closure, name_t *module) {
  data_t *ret;
  
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
  data_t  *ret = _closure_get(closure, name);
  
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
    if (script -> async || !kwargs) {
      closure -> params = strdata_dict_create();
      closure -> free_params = TRUE;
    } else {
      closure -> params = kwargs;
    }
    if (script -> async && kwargs) {
      dict_reduce(kwargs, (reduce_t) data_put_all_reducer, closure -> params);
    }
    for (ix = 0; ix < array_size(args); ix++) {
      param = data_copy(data_array_get(args, ix));
      dict_put(closure -> params, 
               (char *) array_get(script -> params, ix),
               param);
    }
  }
  
  if (!closure -> stack) {
    closure -> stack = datastack_create(script_tostring(script));
    datastack_set_debug(closure -> stack, script_debug);
  } else {
    datastack_clear(closure -> stack);
  }
  
  if (!closure -> catchpoints) {
    asprintf(&str, "%s catchpoints", script_tostring(script));
    closure -> catchpoints = datastack_create(str);
    free(str);
    datastack_set_debug(closure -> catchpoints, script_debug);
  } else {
    datastack_clear(closure -> catchpoints);
  }
  
  if (script -> async) {
    closure -> thread = data_create(Thread, 
                                    closure_tostring(closure),
                                    (threadproc_t) _closure_start,
                                    closure_copy(closure));
    return closure -> thread;
  } else {
    closure -> thread = data_current_thread();
    return _closure_start(closure);
  }
}

closure_t * closure_set_location(closure_t *closure, data_t *location) {
  if (data_type(location) == Int) {
    closure -> line = data_intval(location);
  }
  return closure;
}

data_t * closure_stash(closure_t *closure, unsigned int stash, data_t *data) {
  if (stash < NUM_STASHES) {
    closure -> stashes[stash] = data;
    return data;
  } else {
    return NULL;
  }
}

data_t * closure_unstash(closure_t *closure, unsigned int stash) {
  if (stash < NUM_STASHES) {
    return closure -> stashes[stash];
  } else {
    return NULL;
  }
}
