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
#include <error.h>
#include <file.h>
#include <grammarparser.h>
#include <namespace.h>
#include <script.h>

#include <str.h>

int script_debug = 0;

typedef data_t * (*mathop_t)(data_t *, data_t *);

typedef struct _mathop_def {
  char     *token;
  mathop_t  fnc;
} mathop_def_t;

static typedescr_t      typedescr_script;
static typedescr_t      typedescr_closure;
static function_t       _builtins[];
static mathop_def_t     mathops[];
static scriptloader_t * _loader;


/*
 * S T A T I C  F U N C T I O N S
 */

static data_t *         _data_new_script(data_t *, va_list);
static data_t *         _data_copy_script(data_t *, data_t *);
static int              _data_cmp_script(data_t *, data_t *);
static char *           _data_tostring_script(data_t *);
static data_t *         _data_execute_script(data_t *, char *, array_t *, dict_t *);

static data_t *         _data_new_closure(data_t *, va_list);
static data_t *         _data_copy_closure(data_t *, data_t *);
static int              _data_cmp_closure(data_t *, data_t *);
static char *           _data_tostring_closure(data_t *);
static data_t *         _data_execute_closure(data_t *, char *, array_t *, dict_t *);

static data_t *         _plus(data_t *, data_t *);
static data_t *         _minus(data_t *, data_t *);
static data_t *         _times(data_t *, data_t *);
static data_t *         _div(data_t *, data_t *);
static data_t *         _modulo(data_t *, data_t *);
static data_t *         _pow(data_t *, data_t *);
static data_t *         _gt(data_t *, data_t *);
static data_t *         _geq(data_t *, data_t *);
static data_t *         _lt(data_t *, data_t *);
static data_t *         _leq(data_t *, data_t *);
static data_t *         _eq(data_t *, data_t *);
static data_t *         _neq(data_t *, data_t *);

static data_t *         _script_function_print(data_t *, char *, array_t *, dict_t *);
static data_t *         _script_function_mathop(data_t *, char *, array_t *, dict_t *);

static void             _script_list_visitor(instruction_t *);
static closure_t *      _script_variable_copier(entry_t *, closure_t *);

static object_t *       _object_create_root(void);

static listnode_t *     _closure_execute_instruction(instruction_t *, closure_t *);
static data_t *         _closure_set_local(closure_t *, char *, data_t *);

static file_t *         _scriptloader_open_systemfile(scriptloader_t *, char *);
static file_t *         _scriptloader_open_userfile(scriptloader_t *, char *);
static file_t *         _scriptloader_open_from_basedir(scriptloader_t *, char *, char *);
static data_t *         _scriptloader_parse_reader(scriptloader_t *, reader_t *, script_t *, char *);
static data_t *         _scriptloader_load(scriptloader_t *, char *, int);


#define FUNCTION_MATHOP  ((voidptr_t) _script_function_mathop)
static function_t _builtins[] = {
    { name: "print",      fnc: ((voidptr_t) _script_function_print),  min_params: 1, max_params: -1 },
    { name: "+",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "-",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "*",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "/",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "%",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "^",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: ">=",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: ">",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "<=",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "<",          fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "==",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: "!=",         fnc: FUNCTION_MATHOP,         min_params: 2, max_params:  2 },
    { name: NULL,         fnc: NULL,                    min_params: 0, max_params:  0 }
};

static mathop_def_t mathops[] = {
  { token: "+",   fnc: _plus },
  { token: "-",   fnc: _minus },
  { token: "*",   fnc: _times },
  { token: "/",   fnc: _div },
  { token: "%",   fnc: _modulo },
  { token: "^",   fnc: _pow },
  { token: ">",   fnc: _gt },
  { token: ">=",  fnc: _geq },
  { token: "<",   fnc: _lt },
  { token: "<=",  fnc: _leq },
  { token: "==",  fnc: _eq },
  { token: "!=",  fnc: _leq },
  { token: NULL,  fnc: NULL }
};

static scriptloader_t * _loader = NULL;

/*
 * data_t Script and Closure types
 */

static typedescr_t typedescr_script = {
  type:                  Script,
  typecode:              "C",
  new:      (new_t)      _data_new_script,
  copy:     (copydata_t) _data_copy_script,
  cmp:      (cmp_t)      _data_cmp_script,
  free:     (free_t)     script_free,
  tostring: (tostring_t) _data_tostring_script,
  hash:                  NULL,
  parse:                 NULL,
  fallback: (method_t)   _data_execute_script
};

static typedescr_t typedescr_closure = {
  type:                  Closure,
  typecode:              "R",
  new:      (new_t)      _data_new_closure,
  copy:     (copydata_t) _data_copy_closure,
  cmp:      (cmp_t)      _data_cmp_closure,
  free:     (free_t)     closure_free,
  tostring: (tostring_t) _data_tostring_closure,
  hash:                  NULL,
  parse:                 NULL,
  fallback: (method_t)   _data_execute_closure
};

/*
 * Closure data functions
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
  return strcmp(s1 -> name, s2 -> name);
}

char * _data_tostring_script(data_t *d) {
  static char buf[32];

  snprintf(buf, 32, "<<script %s>>", ((script_t *) d -> ptrval) -> name);
  return buf;
}

data_t * _data_execute_script(data_t *self, char *name, array_t *params, dict_t *kwargs) {
  script_t *script = (script_t *) self -> ptrval;
  return script_execute(script, NULL, params, kwargs);
}

data_t * data_create_script(script_t *script) {
  data_t *ret;

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
  return strcmp(c1 -> script -> name, c2 -> script -> name);
}

char * _data_tostring_closure(data_t *d) {
  static char buf[32];

  snprintf(buf, 32, "<<closure %s>>",
           ((closure_t *) d -> ptrval) -> script -> name);
  return buf;
}

data_t * _data_execute_closure(data_t *self, char *name, array_t *params, dict_t *kwargs) {
  closure_t *closure = (closure_t *) self -> ptrval;
  return closure_execute_function(closure, name, params, kwargs);
}

data_t * data_create_closure(closure_t *closure) {
  return data_create(Closure, closure);
}

/*
 * script_t static functions
 */

data_t * _plus(data_t *d1, data_t *d2) {
  str_t  *s;
  data_t *ret;
  int     type;

  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    if (type == Int) {
      ret = data_create(Int, d1 -> intval + d2 -> intval);
    } else {
      ret = data_create(Float, 
        (double) ((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) +
        (double) ((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
    }
  } else {
    s = str_copy_chars(data_tostring(d1));
    str_append_chars(s, data_tostring(d2));
    ret = data_create(String, str_chars(s));
    str_free(s);
  }
  return ret;
}

data_t * _minus(data_t *d1, data_t *d2) {
  data_t *ret;
  int     type;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) + 
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  }
  return ret;
}

data_t * _times(data_t *d1, data_t *d2) {
  data_t *ret;
  int     type;
  str_t  *s;
  int     ix;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) + 
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  } else if ((data_type(d1) == String) && (data_type(d2) == Int)) {
    s = str_create(0);
    for (ix = 0; ix < d2 -> intval; ix++) {
      str_append_chars(s, data_tostring(d1));
    }
    ret = data_create(String, str_chars(s));
    str_free(s);    
  }
  return ret;
}

data_t * _div(data_t *d1, data_t *d2) {
  data_t *ret;
  int     type;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    type = ((data_type(d1) == data_type(d2)) && (data_type(d1) == Int))
      ? Int : Float;
    ret = data_create(type, 
		((data_type(d1) == Int) ? d1 -> intval : d1 -> dblval) /
		((data_type(d2) == Int) ? d2 -> intval : d2 -> dblval));
  }
  return ret;
}

data_t * _modulo(data_t *d1, data_t *d2) {
  data_t *ret;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    ret = data_create(Int, 
      (long) ((data_type(d1) == Int) ? d1 -> intval : round(d1 -> dblval)) %
      (long) ((data_type(d2) == Int) ? d2 -> intval : round(d2 -> dblval)));
  }
  return ret;
}

data_t * _pow(data_t *d1, data_t *d2) {
  data_t *ret;

  ret = NULL;
  if (data_is_numeric(d1) && data_is_numeric(d2)) {
    ret = data_create(Float, pow(
      ((data_type(d1) == Int) ? (double) d1 -> intval : d1 -> dblval),
      ((data_type(d2) == Int) ? (double) d2 -> intval : d2 -> dblval)));
  }
  return ret;
}

data_t * _gt(data_t *d1, data_t *d2) {
  return data_create(Bool, data_cmp(d1, d2) > 0);
}

data_t * _geq(data_t *d1, data_t *d2) {
  return data_create(Bool, data_cmp(d1, d2) >= 0);
}

data_t * _lt(data_t *d1, data_t *d2) {
  return data_create(Bool, data_cmp(d1, d2) < 0);
}

data_t * _leq(data_t *d1, data_t *d2) {
  return data_create(Bool, data_cmp(d1, d2) <= 0);
}

data_t * _eq(data_t *d1, data_t *d2) {
  return data_create(Bool, !data_cmp(d1, d2));
}

data_t * _neq(data_t *d1, data_t *d2) {
  return data_create(Bool, data_cmp(d1, d2));
}

data_t * _script_function_mathop(data_t *ignored, char *op, array_t *params, dict_t *kwargs) {
  data_t         *d1;
  data_t         *d2;
  data_t         *ret;
  int             ix;
  mathop_t        fnc;

  assert(array_size(params) == 2);
  d1 = (data_t *) array_get(params, 0);
  assert(d1);
  d2 = (data_t *) array_get(params, 1);
  assert(d2);
  fnc = NULL;
  for (ix = 0; mathops[ix].token; ix++) {
    if (!strcmp(mathops[ix].token, op)) {
      fnc = mathops[ix].fnc;
      break;
    }
  }
  assert(fnc);
  ret = fnc(d1, d2);
  if (script_debug) {
    debug("   %s %s %s = %s", data_debugstr(d1), op, data_debugstr(d2), data_debugstr(ret));
  }
  return ret;
}

data_t * _script_function_print(data_t *ignored, char *name, array_t *params, dict_t *kwargs) {
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
  info(data_tostring(value));
  return data_create(Int, 1);
}

/*
 * script_t static functions
 */

void _script_list_visitor(instruction_t *instruction) {
  debug(instruction_tostring(instruction));
}

/*
 * script_t public functions
 */

static int _initialized = 0;

script_t * script_create(ns_t *ns, script_t *up, char *name) {
  script_t   *ret;
  char       *shortname_buf;
  char       *fullname_buf;
  data_t     *data;
  function_t *builtin;
  script_t   *bi;
  int         len;

  if (!_initialized) {
    typedescr_register(typedescr_script);
    typedescr_register(typedescr_closure);
    _initialized = 1;
  }
  ret = NEW(script_t);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, (free_t) instruction_free);
  list_set_tostring(ret -> instructions, (tostring_t) instruction_tostring);

  ret -> functions = strdata_dict_create();
  ret -> labels = strvoid_dict_create();
  ret -> label = NULL;
  ret -> params = NULL;

  if (!ns && !up) {
    name = "__root__";
    for (builtin = _builtins; builtin -> name; builtin++) {
      bi = script_create_native(ret, builtin);
    }
    ret -> up = NULL;
  }
  ret -> ns = ns;
  
  shortname_buf = fullname_buf = NULL;
  if (!name) {
    len = snprintf(NULL, 0, "__anon__%d__", hashptr(ret));
    shortname_buf = (char *) new(len);
    sprintf(shortname_buf, "__anon__%d__", hashptr(ret));
    name = shortname_buf;
  }
  if (up) {
    dict_put(up -> functions, strdup(name), data_create_script(ret));
    len = snprintf(NULL, 0, "%s/%s", up -> name, name);
    fullname_buf = (char *) new(len);
    sprintf(fullname_buf, "%s/%s", up -> name, name);
    name = fullname_buf;
    ret -> up = up;
    if (!ret -> ns) {
      ret -> ns = up -> ns;
    }
  }
  ret -> name = strdup(name);
  free(shortname_buf);
  free(fullname_buf);
  return ret;
}

char * script_get_name(script_t *script) {
  return script -> name;
}

char * script_tostring(script_t *script) {
  return script_get_name(script);
}

void script_free(script_t *script) {
  if (script) {
    script -> refs--;
    if (script -> refs <= 0) {
      list_free(script -> instructions);
      dict_free(script -> labels);
      array_free(script -> params);
      dict_free(script -> functions);
      free(script -> name);
      free(script);
    }
  }
}

script_t * script_create_native(script_t *script, function_t *function) {
  script_t *ret;
  
  ret = script_create(script -> ns, script, function -> name);
  ret -> native = 1;
  ret -> native_method = (method_t) function -> fnc;
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

  if (script -> native) {
    ret = script -> native_method(self, script -> name, args, kwargs);
  } else {
    closure = script_create_closure(script, self, args, kwargs);
    ret = closure_execute(closure);
    closure_free(closure);
  }
  return ret;
}

void script_list(script_t *script) {
  debug("==================================================================");
  debug("Script Listing - %s", (script -> name ? script -> name : "<<anon>>"));
  debug("------------------------------------------------------------------");
  debug("%-11.11s%-15.15s", "label","Instruction");
  debug("------------------------------------------------------------------");
  list_visit(script -> instructions, (visit_t) _script_list_visitor);
  debug("==================================================================");
}

data_t * script_create_object(script_t *script, array_t *args, dict_t *kwargs) {
  object_t  *ret;
  closure_t *closure;
  data_t    *retval;
  data_t    *self;
  
  ret = object_create(script);
  self = data_create_object(ret);
  retval = script_execute(script, self, args, kwargs);
  return (retval && (retval -> type == Error)) ? retval : self;
}

closure_t * script_create_closure(script_t *script, data_t *self, array_t *args, dict_t *kwargs) {
  closure_t *ret;
  int        ix;

  if (args || (script -> params && array_size(script -> params))) {
    assert(args && (array_size(script -> params) == array_size(args)));
  }
  ret = NEW(closure_t);
  ret -> script = script;
  script -> refs++;

  ret -> variables = strdata_dict_create();
  dict_reduce(ret -> variables, (reduce_t) data_put_all_reducer, script -> functions);
  ret -> stack = datastack_create(script_tostring(script));
  datastack_set_debug(ret -> stack, script_debug);

  if (args) {
    for (ix = 0; ix < array_size(args); ix++) {
      closure_set(ret, array_get(script -> params, ix),
                       array_get(args, ix));
    }
  }
  if (kwargs) {
    dict_reduce(kwargs, (reduce_t) data_put_all_reducer, ret -> variables);
  }
  if (self) {
    if (data_type(self) == Closure) {
      closure_t *closure = (closure_t *) self -> ptrval;
      ret -> up = closure;
      if (closure -> self) {
        self = data_create_object(closure -> self);
      } else {
        self = NULL;
      }
    } else {
      assert(data_type(self) == Object);
    }
    if (self) {
      ret -> self = (object_t *) self -> ptrval;
      _closure_set_local(ret, "self", self);
    }
  }
  ret -> refs++;
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
  switch (data_type(ret)) {
    case String:
      label = data_tostring(ret);
      data_free(ret);
      break;
    case Error:
      label = "ERROR";
      _closure_set_local(closure, "$$ERROR", data_copy(ret));
      break;
    default:
      assert(0);
      break;
  }
  if (label) {
    if (script_debug) {
      debug("  Jumping to '%s'", label);
    }
    node = (listnode_t *) dict_get(closure -> script -> labels, label);
    assert(node);
  }
  return node;
}

data_t * _closure_set_local(closure_t *closure, char *name, data_t *value) {
  data_t *ret;
  
  if (script_debug) {
    debug("  Setting local '%s' = %s in closure for %s",
          name, data_debugstr(value), closure_tostring(closure));
  }
  ret = data_copy(value);
  dict_put(closure -> variables, strdup(name), ret);
  return ret;
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
      free(closure);
    }
  }
}

char * closure_tostring(closure_t *closure) {
  return script_get_name(closure -> script);
}

data_t * closure_pop(closure_t *closure) {
  return datastack_pop(closure -> stack);
}

closure_t * closure_push(closure_t *closure, data_t *entry) {
  datastack_push(closure -> stack, entry);
  return closure;
}

/**
 * Returns the object in which the identifier <name> belongs. Names resolution
 * starts at the current closure, then up the chain of nested closures. If the
 * first element of the name cannot be found in the chain of nested closures,
 * the first part is resolved at the namespace root level.
 *
 * @param closure Closure to resolve the name in
 * @param name List of name components
 *
 * @return If the name can be resolved, a data_t wrapping that object. If
 * the name cannot be resolved but only consists of one element, NULL,
 * indicating the name is a local variable of the closure. If the name
 * consists of more than one element but the path is broken, a
 * data_error(ErrorName) is returned.
 */
data_t * closure_get_container_for(closure_t *closure, array_t *name) {
  closure_t *c;
  data_t    *data;
  data_t    *ret;
  array_t   *tail;
  char      *n;
  char      *first;
  char      *ptr;

  assert(array_size(name));
  if (script_debug) {
    array_debug(name, "  Getting object for %s");
  }
  tail = array_slice(name, 1, -1);
  
  first = (char *) (((data_t *) array_get(name, 0)) -> ptrval);
  data = NULL;
  c = closure;
  while(c) {
    data = closure_get(c, first);
    if (!data_is_error(data)) {
      break;
    } else {
      data_free(data);
      data = NULL;
      c = c -> up;
    }
  }
  if (c) {
    /* Found a match in the closure chain. Now walk the name: */
    if (tail) {
      /* There are more components in the name. data must be object: */
      if (data -> type == Object) {
        ret = object_resolve((object_t *) data -> ptrval, tail);
      } else {
        ret = data_error(ErrorName,
                         "Variable '%s' of function '%s' is not an object",
                         name,
                         closure_tostring(c));
      }
    } else {
      /* Name is local to the closure we found it in: */
      ret = data_create_closure(c);
    }
  } else {
    /* No match. Delegate to the namespace. If the namespace returns
     * NULL, the name must be local.
     */
    ret = ns_resolve(closure -> script -> ns, name);
    if (data_is_error(ret)) {
      data_free(ret);
      ret = data_create_closure(closure);
    }
  }
  data_free(data);
  array_free(tail);
  return ret;
}

data_t * closure_set(closure_t *closure, array_t *varname, data_t *value) {
  data_t    *d;
  data_t    *ret;
  closure_t *c;
  object_t  *o;
  char      *n;

  assert(closure);
  assert(value);
  assert(!varname || !array_size(varname));

  d = closure_get_container_for(closure, varname);
  switch (data_type(d)) {
    case Closure:
      c = (closure_t *) d -> ptrval;
      n = (char *) array_get(varname, 0);
      ret = _closure_set_local(c, n, value);
      data_free(d);
      break;
    case Script:
      o = (object_t *) d -> ptrval;
      n = (char *) array_get(varname, -1);
      ret = data_copy(value);
      object_set(o, n, ret);
      data_free(d);
      break;
    case Error:
      ret = d;
      break;
    default:
      assert(0);
      break;
  }
  return ret;
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

data_t * closure_resolve(closure_t *closure, array_t *varname) {
  data_t    *d;
  data_t    *ret;
  closure_t *c;
  object_t  *o;
  char      *n;

  assert(closure);
  assert(!varname || !array_size(varname));

  d = closure_get_container_for(closure, varname);
  switch (data_type(d)) {
    case Closure:
      c = (closure_t *) d -> ptrval;
      n = (char *) array_get(varname, 0);
      if (script_debug) {
        debug("  Getting local '%s' in closure for %s",
              n, data_tostring(d));
      }
      ret = closure_get(c, n);
      data_free(d);
      break;
    case Object:
      o = (object_t *) d -> ptrval;
      n = (char *) array_get(varname, -1);
      if (script_debug) {
        debug("  Getting object property '%s' in object %s",
              n, data_tostring(d));
      }
      ret = object_get(o, n);
      data_free(d);
      break;
    case Error:
      ret = d;
      break;
    default:
      assert(0);
      break;
  }
  return ret;
}

data_t * closure_execute(closure_t *closure) {
  data_t *ret;

  datastack_clear(closure -> stack);
  list_process(closure -> script -> instructions, (reduce_t) _closure_execute_instruction, closure);
  ret = (datastack_notempty(closure -> stack)) ? closure_pop(closure) : data_null();
  if (script_debug) {
    debug("    Execution of %s done: %s", closure -> script -> name, data_tostring(ret));
  }
  return ret;
}

data_t * closure_execute_function(closure_t *closure, char *name, array_t *args, dict_t *kwargs) {
  script_t *script;
  data_t   *func;
  data_t   *self;
  data_t   *ret;
  
  func = closure_get(closure, name);
  if (!data_is_error(func)) {
    if (data_type(func) != Script)  {
      ret = data_error(ErrorName,
                       "Variable '%s' of function '%s' is not callable",
                       name,
                       closure_tostring(closure));
    } else {
      script = (script_t *) func -> ptrval;
      self = data_create_closure(closure);
      ret = script_execute(script, self, args, kwargs);
      data_free(self);
    }
    data_free(func);
  } else {
    ret = func;
  }
  return ret;
}

/*
 * scriptloader_t - static functions
 */

static file_t * _scriptloader_open_systemfile(scriptloader_t *loader, char *path) {
  if (script_debug) {
    debug("scriptloader_open_systemfile(%s)", path);
  }
  if (!path) {
    return NULL;
  } else {
    return _scriptloader_open_from_basedir(loader, loader -> system_dir, path);
  }
}

static file_t * _scriptloader_open_userfile(scriptloader_t *loader, char *path) {
  if (script_debug) {
    debug("scriptloader_open_userfile(%s)", path);
  }
  if (!path) {
    return NULL;
  } else {
    return _scriptloader_open_from_basedir(loader, loader -> userpath, path);
  }
}

static file_t * _scriptloader_open_from_basedir(scriptloader_t *loader, char *basedir, char *name) {
  char      *fname;
  char      *ptr;
  fsentry_t *e;
  fsentry_t *init;
  file_t  *ret;

  if (script_debug) {
    debug("_scriptloader_open_from_basedir(%s, %s)", basedir, name);
  }
  assert(*(basedir + (strlen(basedir) - 1)) == '/');
  if ((*name == '/') || (*name == '.')) {
    name++;
  }
  fname = new(strlen(loader -> system_dir) + strlen(name) + 5);
  sprintf(fname, "%s%s", loader -> system_dir, name);
  for (ptr = strchr(fname, '.'); ptr; ptr = strchr(ptr, '.')) {
    *ptr = '/';
  }
  e = fsentry_create(fname);
  init = NULL;
  if (fsentry_isdir(e)) {
    init = fsentry_getentry(e, "__init__.obl");
    fsentry_free(e);
    if (fsentry_exists(init)) {
      e = init;
    } else {
      e = NULL;
      fsentry_free(init);
    }
  } else {
    strcat(fname, ".obl");
    e = fsentry_create(fname);
  }
  if ((e != NULL) && fsentry_isfile(e) && fsentry_canread(e)) {
    ret = fsentry_open(e);
    assert(ret -> fh > 0);
  } else {
    ret = NULL;
  }
  fsentry_free(e);
  free(fname);
  return ret;
}

data_t * _scriptloader_parse_reader(scriptloader_t *loader, reader_t *rdr, script_t *up, char *name) {
  script_t *script;
  object_t *object;
  
  parser_clear(loader -> parser);
  parser_set(loader -> parser, "name", data_create(String, name));
  parser_set(loader -> parser, "up", data_create_script(up));
  parser_set(loader -> parser, "ns", data_create(Object, loader -> ns -> root));
  parser_parse(loader -> parser, rdr);
  script = (script_t *) loader -> parser -> data;
  return script_create_object(script, NULL, NULL);
}

data_t * _scriptloader_load(scriptloader_t *loader, char *name, int empty_allowed) {
  file_t    *text;
  str_t     *dummy;
  reader_t  *rdr;
  data_t    *ret;

  if (script_debug) {
    debug("scriptloader_load(%s)", name);
  }
  assert(name);
  text = _scriptloader_open_userfile(loader, name);
  if (!text) {
    text = _scriptloader_open_systemfile(loader, name);
  }
  if (!text) {
    if (empty_allowed) {
      dummy = str_wrap("");
      rdr = (reader_t *) dummy;
    } else {
      rdr = NULL;
    }
  } else {
    rdr = (reader_t *) rdr;
  }
  ret = (rdr) 
          ? scriptloader_load_fromreader(loader, name, rdr) 
          : data_error(ErrorName, "Could not locate '%s'", name);
  str_free(dummy);
  file_free(text);
  return ret;
}

/*
 * scriptloader_t - public functions
 */

#define GRAMMAR_PATH    "etc/grammar.txt"
/* #define OBELIX_SYS_PATH "/usr/share/obelix" */
#define OBELIX_SYS_PATH "share"

scriptloader_t * scriptloader_create(char *obl_path, char *grammarpath) {
  scriptloader_t   *ret;
  file_t           *file;
  grammar_parser_t *gp;
  data_t           *root;
  char             *sysdir;
  int               len;

  assert(!_loader);
  ret = NEW(scriptloader_t);
  sysdir = getenv("OBELIX_SYS_PATH");
  if (!sysdir) {
    sysdir = OBELIX_SYS_PATH;
  }
  len = strlen(sysdir);
  ret -> system_dir = (char *) new (len + ((*(sysdir + (len - 1)) != '/') ? 2 : 1));
  strcpy(ret -> system_dir, sysdir);
  if (*(ret -> system_dir + (strlen(ret -> system_dir) - 1)) != '/') {
    strcat(ret -> system_dir, "/");
  }
  if (!obl_path) {
    obl_path = "./";
  }
  len = strlen(obl_path);
  ret -> userpath = (char *) new (len + ((*(obl_path + (len - 1)) != '/') ? 2 : 1));
  strcpy(ret -> userpath, obl_path);
  if (*(ret -> userpath + (strlen(ret -> userpath) - 1)) != '/') {
    strcat(ret -> userpath, "/");
  }

  if (!grammarpath) {
    grammarpath = (char *) new(strlen(ret -> system_dir) + strlen(GRAMMAR_PATH) + 1);
    strcpy(grammarpath, ret -> system_dir);
    strcat(grammarpath, GRAMMAR_PATH);
  }
  debug("system dir: %s", ret -> system_dir);
  debug("user path: %s", ret -> userpath);
  debug("grammar file: %s", grammarpath);

  file = file_open(grammarpath);
  assert(file);
  gp = grammar_parser_create(file);
  ret -> grammar = grammar_parser_parse(gp);
  assert(ret -> grammar);
  grammar_parser_free(gp);
  file_free(file);
  free(grammarpath);

  ret -> parser = parser_create(ret -> grammar);

  ret -> ns = NULL;
  file = _scriptloader_open_systemfile(ret, "");
  if (file) {
    root = _scriptloader_parse_reader(ret, (reader_t *) file, NULL, NULL);
    file_free(file);
    if (!data_is_error(root)) {
      ret -> ns = ns_create((object_t *) root -> ptrval);
      data_free(root);
    }
  }
  if (!ret -> ns) {
    scriptloader_free(ret);
    ret = NULL;
  } else {
    _loader = ret;
  }
  return ret;
}

scriptloader_t * scriptloader_get(void) {
  return _loader;
}

void scriptloader_free(scriptloader_t *loader) {
  if (loader) {
    free(loader -> system_dir);
    free(loader -> userpath);
    grammar_free(loader -> grammar);
    parser_free(loader -> parser);
    ns_free(loader -> ns);
    free(loader);
  }
}

data_t * scriptloader_load_fromreader(scriptloader_t *loader, char *name, reader_t *reader) {
  parser_t       *parser;
  script_t       *script;
  object_t       *object;
  array_t        *components;
  int             ix;
  char           *curname;
  data_t         *ret = NULL;

  if (script_debug) {
    debug("scriptloader_load_fromreader(%s)", name);
  }
  if (!name || !name[0]) {
    return data_error(ErrorName, "Cannot load script with no name");
  }
  components = array_split(name, ".");
  object = loader -> ns -> root;
  curname = (char *) new(strlen(name) + 1);
  curname[0] = 0;
  for (ix = 0; ix < array_size(components) - 1; ix++) {
    char *n = (char *) array_get(components, ix);
    data_t *o = object_get(object, n);
    if (curname[0]) {
      strcat(curname, ".");
    }
    strcat(curname, n);
    if (data_is_error(o)) {
      if (script_debug) {
        debug("Module %s does not have submodule %s yet", 
              script_tostring(object->script), n);
      }
      o = _scriptloader_load(loader, curname, 1);
    }
    if (data_type(o) == Object) {
      if (script_debug) {
        debug("Module %s already has submodule %s", 
              script_tostring(object->script), n);
      }
      object = (object_t *) o -> ptrval;
    } else if (data_is_error(o)) {
      ret = o;
    } else {
      if (script_debug) {
        debug("Module %s already has attribute %s, but it's not a submodule", 
              script_tostring(object->script), n);
      }
      ret = data_error(ErrorName, "%s is not an object", curname);
    }
    if (ret) {
      ix = array_size(components);
    }
  }
  free(curname);
  array_free(components);
  
  if (!ret) {
    ret = _scriptloader_parse_reader(loader, reader, object -> script -> up, name);
  }
  return ret;
}

data_t * scriptloader_load(scriptloader_t *loader, char *name) {
  return _scriptloader_load(loader, name, 0);
}

