/*
 * /obelix/src/script.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <data.h>
#include <exception.h>
#include <logging.h>
#include <namespace.h>
#include <script.h>
#include <str.h>


int script_debug = 0;

static void             _script_init(void) __attribute__((constructor(102)));

static data_t *         _data_new_script(data_t *, va_list);
static data_t *         _data_copy_script(data_t *, data_t *);
static int              _data_cmp_script(data_t *, data_t *);
static char *           _data_tostring_script(data_t *);

static data_t *         _data_new_closure(data_t *, va_list);
static data_t *         _data_copy_closure(data_t *, data_t *);
static int              _data_cmp_closure(data_t *, data_t *);
static char *           _data_tostring_closure(data_t *);
static data_t *         _data_call_closure(data_t *, array_t *, dict_t *);
static data_t *         _data_resolve_closure(data_t *, char *);
static data_t *         _data_set_closure(data_t *, char *, data_t *);

extern data_t *         _script_function_print(char *, array_t *, dict_t *);

static listnode_t *     _closure_execute_instruction(instruction_t *, closure_t *);

/*
 * data_t Script and Closure types
 */

static vtable_t _vtable_script[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_script },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_script },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_script },
  { .id = FunctionFree,     .fnc = (void_t) script_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_script },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_script = {
  .type =      Script,
  .type_name = "script",
  .vtable =    _vtable_script
};

static vtable_t _vtable_closure[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_closure },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_closure },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_closure },
  { .id = FunctionFree,     .fnc = (void_t) closure_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_closure },
  { .id = FunctionResolve,  .fnc = (void_t) _data_resolve_closure },
  { .id = FunctionCall,     .fnc = (void_t) _data_call_closure },
  { .id = FunctionSet,      .fnc = (void_t) _data_set_closure },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_closure = {
  .type =      Closure,
  .type_name = "closure",
  .vtable =    _vtable_closure
};

void _script_init(void) {
  logging_register_category("script", &script_debug);
  typedescr_register(&_typedescr_script);
  typedescr_register(&_typedescr_closure);
}

/*
 * Script data functions
 */

data_t * _data_new_script(data_t *ret, va_list arg) {
  script_t *script;

  script = va_arg(arg, script_t *);
  ret -> ptrval = script;
  ((script_t *) ret -> ptrval) -> refs++;
  return ret;
}

data_t * _data_copy_script(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((script_t *) target -> ptrval) -> refs++;
  return target;
}

int _data_cmp_script(data_t *d1, data_t *d2) {
  script_t *s1;
  script_t *s2;

  s1 = d1 -> ptrval;
  s2 = d2 -> ptrval;
  return name_cmp(s1 -> name, s2 -> name);
}

char * _data_tostring_script(data_t *d) {
  static char buf[32];

  snprintf(buf, 32, "<<script %s>>", script_tostring((script_t *) d -> ptrval));
  return buf;
}

data_t * _data_call_script(data_t *self, array_t *params, dict_t *kwargs) {
  script_t *script = (script_t *) self -> ptrval;
  return script_execute(script, NULL, params, kwargs);
}

data_t * data_create_script(script_t *script) {
  return data_create(Script, script);
}

/*
 * Closure data functions
 */

data_t * _data_new_closure(data_t *ret, va_list arg) {
  closure_t *closure;

  closure = va_arg(arg, closure_t *);
  ret -> ptrval = closure;
  ((closure_t *) ret -> ptrval) -> refs++;
  return ret;
}

data_t * _data_copy_closure(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((closure_t *) target -> ptrval) -> refs++;
  return target;
}

int _data_cmp_closure(data_t *d1, data_t *d2) {
  closure_t *c1;
  closure_t *c2;

  c1 = d1 -> ptrval;
  c2 = d2 -> ptrval;
  return name_cmp(c1 -> script -> name, c2 -> script -> name);
}

char * _data_tostring_closure(data_t *d) {
  return closure_tostring((closure_t *) d -> ptrval);
}

data_t * _data_call_closure(data_t *self, array_t *params, dict_t *kwargs) {
  closure_t *closure = (closure_t *) self -> ptrval;
  return closure_execute(closure, params, kwargs);
}

data_t * _data_resolve_closure(data_t *data, char *name) {
  return closure_resolve((closure_t *) data -> ptrval, name);
}

data_t * _data_set_closure(data_t *data, char *name, data_t *value) {
  return closure_set((closure_t *) data -> ptrval, name, value);
}

data_t * data_create_closure(closure_t *closure) {
  return data_create(Closure, closure);
}

/*
 * script_t static functions
 */

data_t * _script_function_print(char *name, array_t *params, dict_t *kwargs) {
  char          *varname;
  data_t        *value;
  char          *fmt;
  char          *ptr;
  char           buf[1000], tmp[1000];

  assert(array_size(params));
  value = (data_t *) array_get(params, 0);
  assert(value);

  /*
   * Ideally we want to do a printf-style function. The idea is to have the
   * first argument as the format string, and following parameters as values.
   * Since there is no API for sprintf that takes a user-accessible data
   * structure, just the va_list one, we will have to go through the format
   * string and do it one by one.
   *
   * But that's future. Now we just print the first parameter :-)
   */
  printf("%s\n", data_tostring(value));
  return data_create(Int, 1);
}

/*
 * script_t static functions
 */

/*
 * script_t public functions
 */

script_t * script_create(namespace_t *ns, script_t *up, char *name) {
  script_t   *ret;
  char       *anon = NULL;

  if (!name) {
    asprintf(&anon, "__anon__%d__", hashptr(ret));
    name = anon;
  }
  if (script_debug) {
    debug("Creating script '%s'", name);
  }
  ret = NEW(script_t);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, (free_t) instruction_free);
  list_set_tostring(ret -> instructions, (tostring_t) instruction_tostring);

  ret -> functions = strdata_dict_create();
  ret -> labels = strvoid_dict_create();
  ret -> label = NULL;
  ret -> params = NULL;

  ret -> name = name_create(0);
  if (up) {
    dict_put(up -> functions, strdup(name), data_create_script(ret));
    name_append(ret -> name, up -> name);
    ret -> up = script_copy(up);
    ret -> ns = ns_create(up -> ns);
  } else {
    assert(ns);
    ret -> ns = ns_create(ns);
    ret -> up = NULL;
  }
  name_extend(ret -> name, name);
  free(anon);
  return ret;
}

script_t * script_copy(script_t *script) {
  script -> refs++;
  return script;
}

char * script_tostring(script_t *script) {
  return name_tostring(script -> name);
}

void script_free(script_t *script) {
  if (script) {
    script -> refs--;
    if (script -> refs <= 0) {
      list_free(script -> instructions);
      dict_free(script -> labels);
      array_free(script -> params);
      dict_free(script -> functions);
      script_free(script -> up);
      ns_free(script -> ns);
      name_free(script -> name);
      free(script);
    }
  }
}

script_t * script_push_instruction(script_t *script, instruction_t *instruction) {
  listnode_t *node;

  list_push(script -> instructions, instruction);
  if (script -> label) {
    instruction_set_label(instruction, script -> label);
    free(script -> label);
    script -> label = NULL;
  }
  if (instruction -> label) {
    node = list_tail_pointer(script -> instructions);
    dict_put(script -> labels, strdup(instruction ->label), node);
  }
  return script;
}

data_t * script_execute(script_t *script, data_t *self, array_t *args, dict_t *kwargs) {
  data_t    *ret;
  closure_t *closure;

  if (script_debug) {
    debug("Executing script '%s'", script_tostring(script));
  }
  closure = script_create_closure(script, self, NULL);
  ret = closure_execute(closure, args, kwargs);
  closure_free(closure);
  return ret;
}

void script_list(script_t *script) {
  instruction_t *instr;
  
  debug("==================================================================");
  debug("Script Listing - %s", script_tostring(script));
  debug("------------------------------------------------------------------");
  debug("%-11.11s%-15.15s", "label","Instruction");
  debug("------------------------------------------------------------------");
  for (list_start(script -> instructions); list_has_next(script -> instructions); ) {
    instr = (instruction_t *) list_next(script -> instructions);
    debug(instruction_tostring(instr));
  }
  debug("==================================================================");
}

script_t * script_get_toplevel(script_t *script) {
  script_t *ret;

  for (ret = script; ret -> up; ret = ret -> up);
  return ret;
}

data_t * script_create_object(script_t *script, array_t *args, dict_t *kwargs) {
  object_t  *ret;
  closure_t *closure;
  data_t    *retval;
  data_t    *self;
  
  if (script_debug) {
    debug("script_create_object(%s)", script_tostring(script));
  }
  ret = object_create(script);
  self = data_create_object(ret);
  retval = script_execute(script, self, args, kwargs);
  if (!data_is_error(retval)) {
    ret -> retval = retval;
    retval = self;
  }
  if (script_debug) {
    debug("  script_create_object returns %s", data_tostring(retval));
  }
  return retval;
}

closure_t * _script_create_closure_reducer(entry_t *entry, closure_t *closure) {
  char      *name = (char *) entry -> key;
  data_t    *func = (data_t *) entry -> value;
  data_t    *value;
  closure_t *c;

  if (data_is_script(func)) {
    c = script_create_closure(data_scriptval(func), closure -> self, closure);
    c -> up = closure;
    value = data_create(Closure, c);
  } else {
    /* Native function */
    /* TODO: Do we have a closure-like structure to bind the function to self? */
    value = data_copy(func);
  }
  closure_set(closure, name, value);
  return closure;
}

object_t * _object_set_closures(entry_t *entry, object_t *object) {
  data_t *closure;
  char   *name;
  
  closure = data_copy((data_t *) entry -> value);
  name = (char *) entry -> key;
  object_set(object, name, closure);
  return object;
}

closure_t * script_create_closure(script_t *script, data_t *self, closure_t *up) {
  closure_t *ret;
  int        ix;

  if (script_debug) {
    debug("Creating closure for script '%s'", script_tostring(script));
  }
  ret = NEW(closure_t);
  ret -> script = script_copy(script);
  ret -> imports = ns_create(script -> ns);

  ret -> variables = strdata_dict_create();
  ret -> stack = datastack_create(script_tostring(script));
  datastack_set_debug(ret -> stack, script_debug);
  ret -> up = up;

  if (self) {
    ret -> self = data_copy(self);
    closure_set(ret, "self", data_copy(self));
  }
  dict_reduce(script -> functions, 
              (reduce_t) _script_create_closure_reducer, ret);
  ret -> refs++;
  
  if (!up) {
    /*
     * Bind all functions in this closure to the 'self' object:
     */
    if (self) {
      dict_reduce(ret -> variables, (reduce_t) _object_set_closures, data_objectval(self));
    }
    /*
     * Import standard lib:
     */
    closure_import(ret, NULL);
  }
  return ret;
}

/*
 * closure_t - static functions
 */

listnode_t * _closure_execute_instruction(instruction_t *instr, closure_t *closure) {
  data_t     *ret;
  char       *label = NULL;
  listnode_t *node = NULL;

  ret = instruction_execute(instr, closure);
  if (ret) {
    switch (data_type(ret)) {
      case String:
        label = strdup(data_tostring(ret));
        data_free(ret);
        break;
      case Error:
        label = strdup("ERROR");
        closure_set(closure, "$$ERROR", data_copy(ret));
        break;
      default:
        debug("Instr '%s' returned '%s'", instruction_tostring(instr), data_tostring(ret));
        assert(0);
        break;
    }
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

/*
 * closure_t - public functions
 */

void closure_free(closure_t *closure) {
  if (closure) {
    closure -> refs--;
    if (closure <= 0) {
      script_free(closure -> script);
      datastack_free(closure -> stack);
      dict_free(closure -> variables);
      data_free(closure -> self);
      ns_free(closure -> imports);
      free(closure);
    }
  }
}

char * closure_tostring(closure_t *closure) {
  return script_tostring(closure -> script);
}

data_t * closure_pop(closure_t *closure) {
  return datastack_pop(closure -> stack);
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
  datastack_push(closure -> stack, value);
  return closure;
}

data_t * closure_import(closure_t *closure, name_t *module) {
  return ns_import(closure -> imports, module);
}

data_t * closure_set(closure_t *closure, char *name, data_t *value) {
  if (script_debug) {
    if (strcmp(name, "self")) {
      debug("  Setting local '%s' = '%s' in closure for %s",
            name, data_debugstr(value), closure_tostring(closure));
    } else {
      debug("  Setting local '%s' in closure for %s",
            name, closure_tostring(closure));
    }
  }
  dict_put(closure -> variables, strdup(name), data_copy(value));
  return value;
}

data_t * closure_get(closure_t *closure, char *varname) {
  data_t *ret;

  ret = (data_t *) dict_get(closure -> variables, varname);
  if (ret) {
    ret = data_copy(ret);
  } else {
    ret = data_error(ErrorName,
                     "Closure '%s' has no attribute '%s'",
                     closure_tostring(closure),
                     varname);
  }
  return ret;
}

int closure_has(closure_t *closure, char *name) {
  int ret;

  ret = dict_has_key(closure -> variables, name);
  if (res_debug) {
    debug("   closure_has('%s', '%s'): %d", closure_tostring(closure), name, ret);
  }
  return ret;
}

data_t * closure_resolve(closure_t *closure, char *name) {
  data_t *ret;
  
  ret = (data_t *) dict_get(closure -> variables, name);
  if (!ret) {
    if (closure -> up) {
      if (!strcmp(name, "^") ||
          !strcmp(name, name_last(closure -> up -> script -> name))) {
        ret = data_create(Closure, closure -> up);
      } else {
        ret = closure_resolve(closure -> up, name);
      }
    } else {
      ret = ns_resolve(closure -> imports, name);
    }
  }
  return ret;
}

data_t * closure_execute(closure_t *closure, array_t *args, dict_t *kwargs) {
  int       ix;
  data_t   *ret;
  script_t *script;

  script = closure -> script;
  if (args || (script -> params && array_size(script -> params))) {
    // FIXME Proper error message
    assert(args && (array_size(script -> params) == array_size(args)));
  }
  if (args) {
    for (ix = 0; ix < array_size(args); ix++) {
      closure_set(closure, array_get(script -> params, ix),
                           array_get(args, ix));
    }
  }
  if (kwargs) {
    dict_reduce(kwargs, (reduce_t) data_put_all_reducer, closure -> variables);
  }
  datastack_clear(closure -> stack);
  list_process(closure -> script -> instructions, (reduce_t) _closure_execute_instruction, closure);
  ret = (datastack_notempty(closure -> stack)) ? closure_pop(closure) : data_null();
  if (script_debug) {
    debug("    Execution of %s done: %s", closure_tostring(closure), data_tostring(ret));
  }
  return ret;
}

