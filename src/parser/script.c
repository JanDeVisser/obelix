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
#include <file.h>
#include <grammarparser.h>
#include <namespace.h>
#include <script.h>

#include <str.h>

int script_debug = 0;

static void             _data_script_init(void) __attribute__((constructor));
static data_t *         _data_new_script(data_t *, va_list);
static data_t *         _data_copy_script(data_t *, data_t *);
static int              _data_cmp_script(data_t *, data_t *);
static char *           _data_tostring_script(data_t *);
static data_t *         _data_execute_script(data_t *, char *, array_t *, dict_t *);

static data_t *         _data_new_closure(data_t *, va_list);
static data_t *         _data_copy_closure(data_t *, data_t *);
static int              _data_cmp_closure(data_t *, data_t *);
static char *           _data_tostring_closure(data_t *);
static data_t *         _data_call_closure(data_t *, array_t *, dict_t *);
static data_t *         _data_resolve_closure(data_t *, char *);

extern data_t *         _script_function_print(data_t *, char *, array_t *, dict_t *);

static void             _script_list_visitor(instruction_t *);
// static closure_t *      _script_variable_copier(entry_t *, closure_t *);

// static object_t *       _object_create_root(void);

static listnode_t *     _closure_execute_instruction(instruction_t *, closure_t *);
static data_t *         _closure_set_local(closure_t *, char *, data_t *);

static file_t *         _scriptloader_open_file(scriptloader_t *, char *, char *);
static reader_t *       _scriptloader_open_reader(scriptloader_t *, char *);

static scriptloader_t * _loader = NULL;

/*
 * data_t Script and Closure types
 */

static vtable_t _vtable_script[] = {
  { .id = MethodNew,      .fnc = (void_t) _data_new_script },
  { .id = MethodCopy,     .fnc = (void_t) _data_copy_script },
  { .id = MethodCmp,      .fnc = (void_t) _data_cmp_script },
  { .id = MethodFree,     .fnc = (void_t) script_free },
  { .id = MethodToString, .fnc = (void_t) _data_tostring_script },
  { .id = MethodNone,     .fnc = NULL }
};

static typedescr_t _typedescr_script = {
  .type =      Script,
  .type_name = "script",
  .vtable =    _vtable_script
};

static vtable_t _vtable_closure[] = {
  { .id = MethodNew,      .fnc = (void_t) _data_new_closure },
  { .id = MethodCopy,     .fnc = (void_t) _data_copy_closure },
  { .id = MethodCmp,      .fnc = (void_t) _data_cmp_closure },
  { .id = MethodFree,     .fnc = (void_t) closure_free },
  { .id = MethodToString, .fnc = (void_t) _data_tostring_closure },
  { .id = MethodResolve,  .fnc = (void_t) _data_resolve_closure },
  { .id = MethodCall,     .fnc = (void_t) _data_call_closure },
  { .id = MethodNone,     .fnc = NULL }
};

static typedescr_t _typedescr_closure = {
  .type =      Closure,
  .type_name = "closure",
  .vtable =    _vtable_closure
};

/*
 * Script data functions
 */

void _data_script_init(void) {
  typedescr_register(&_typedescr_script);
  typedescr_register(&_typedescr_closure);
}

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
  return strcmp(c1 -> script -> name, c2 -> script -> name);
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

data_t * data_create_closure(closure_t *closure) {
  return data_create(Closure, closure);
}

/*
 * script_t static functions
 */

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
  printf("%s\n", data_tostring(value));
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

script_t * script_create(namespace_t *ns, script_t *up, char *name) {
  script_t   *ret;
  char       *shortname_buf;
  char       *fullname_buf;
  data_t     *data;
  function_t *builtin;
  script_t   *bi;
  int         len;

  ret = NEW(script_t);
  ret -> instructions = list_create();
  list_set_free(ret -> instructions, (free_t) instruction_free);
  list_set_tostring(ret -> instructions, (tostring_t) instruction_tostring);

  ret -> functions = strdata_dict_create();
  ret -> labels = strvoid_dict_create();
  ret -> label = NULL;
  ret -> params = NULL;

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
    fullname_buf = (char *) new(len + 1);
    sprintf(fullname_buf, "%s/%s", up -> name, name);
    name = fullname_buf;
    ret -> up = script_copy(up);
    ret -> ns = ns_create(up -> ns);
  } else {
    assert(ns);
    ret -> ns = ns_create(ns);
    ret -> up = NULL;
  }
  ret -> name = strdup(name);
  free(shortname_buf);
  free(fullname_buf);
  return ret;
}

script_t * script_copy(script_t *script) {
  script -> refs++;
  return script;
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
      script_free(script -> up);
      ns_free(script -> ns);
      free(script -> name);
      free(script);
    }
  }
}

script_t * script_make_native(script_t *script, function_t *function) {
  if (!script -> native) {
    assert(list_size(script -> instructions) == 0);
    list_free(script -> instructions);
    script -> native = 1;
  }
  script -> native_method = (method_t) function -> fnc;
  return script;
}

script_t * script_create_native(script_t *script, function_t *function) {
  script_t *ret;
  
  ret = script_create(script -> ns, script, function -> name);
  script_make_native(ret, function);
  return ret;
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
    closure = script_create_closure(script, self);
    ret = closure_execute(closure, args, kwargs);
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
    debug("script_create_object(%s)", script_get_name(script));
  }
  ret = object_create(script);
  self = data_create_object(ret);
  retval = script_execute(script, self, args, kwargs);
  if (!data_is_error(retval)) {
    retval = self;
  }
  if (script_debug) {
    debug("  script_create_object returns %s", data_tostring(retval));
  }
  return retval;
}

/*
 FIXME: Closures should be created when an object is created, with no arguments
 */


closure_t * _script_create_closure_reducer(entry_t *entry, closure_t *closure) {
  char      *name = (char *) entry -> key;
  script_t  *func = (script_t *) entry -> value;
  closure_t *c;

  c = script_create_closure(func, closure -> self, closure);
  c -> up = closure;
  closure_set(closure, name, data_create(Closure, c));
  return closure;
}

closure_t * script_create_closure(script_t *script, data_t *self, closure_t *up) {
  closure_t *ret;
  int        ix;

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
        _closure_set_local(closure, "$$ERROR", data_copy(ret));
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
  return script_get_name(closure -> script);
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

data_t * closure_import(closure_t *closure, array_t *module) {
  return ns_import(closure -> imports, module);
}

data_t * closure_set(closure_t *closure, char *name, data_t *value) {
  data_t *ret;
  
  if (script_debug) {
    debug("  Setting local '%s' = %s in closure for %s",
          name, data_debugstr(value), closure_tostring(closure));
  }
  ret = data_copy(value);
  dict_put(closure -> variables, strdup(name), ret);
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
      ret = closure_resolve(closure -> up, name);
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

  if (args || (script -> params && array_size(script -> params))) {
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
    if (!data_is_script(func))  {
      ret = data_error(ErrorName,
                       "Variable '%s' of function '%s' is not callable",
                       name,
                       closure_tostring(closure));
    } else {
      script = data_scriptval(func);
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

static file_t * _scriptloader_open_file(scriptloader_t *loader, char *basedir, char *name) {
  char      *fname;
  char      *ptr;
  fsentry_t *e;
  fsentry_t *init;
  file_t    *ret;

  if (script_debug) {
    debug("_scriptloader_open_file('%s', '%s')", basedir, name);
  }
  assert(*(basedir + (strlen(basedir) - 1)) == '/');
  while (name && ((*name == '/') || (*name == '.'))) {
    name++;
  }
  fname = new(strlen(basedir) + strlen(name) + 5);
  sprintf(fname, "%s%s", basedir, name);
  for (ptr = strchr(fname + strlen(basedir), '.'); ptr; ptr = strchr(ptr, '.')) {
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

reader_t * _scriptloader_open_reader(scriptloader_t *loader, char *name) {
  file_t   *text = NULL;
  char     *path_entry;
  str_t    *dummy;
  reader_t *rdr = NULL;
  data_t   *ret;

  if (script_debug) {
    debug("_scriptloader_open_reader('%s')", name);
  }
  assert(name);
  for (list_start(loader -> load_path); !text && list_has_next(loader -> load_path); ) {
    path_entry = (char *) list_next(loader -> load_path);
    text = _scriptloader_open_file(loader, path_entry, name);
  }
  if (text) {
    rdr = (reader_t *) text;
  }
  return rdr;
}

/*
 * scriptloader_t - public functions
 */

#define GRAMMAR_FILE    "grammar.txt"
/* #define OBELIX_SYS_PATH "/usr/share/obelix" */
#define OBELIX_SYS_PATH "install/share/"

scriptloader_t * scriptloader_create(char *sys_dir, char *user_path, char *grammarpath) {
  scriptloader_t   *ret;
  grammar_parser_t *gp;
  file_t           *file;
  data_t           *root;
  reader_t         *rdr;
  char             *userdir;
  int               len;

  if (script_debug) {
    debug("Creating script loader");
  }
  assert(!_loader);
  ret = NEW(scriptloader_t);
  if (!sys_dir) {
    sys_dir = getenv("OBELIX_SYS_PATH");
  }
  if (!sys_dir) {
    sys_dir = OBELIX_SYS_PATH;
  }
  len = strlen(sys_dir);
  ret -> system_dir = (char *) new (len + ((*(sys_dir + (len - 1)) != '/') ? 2 : 1));
  strcpy(ret -> system_dir, sys_dir);
  if (*(ret -> system_dir + (strlen(ret -> system_dir) - 1)) != '/') {
    strcat(ret -> system_dir, "/");
  }
  
  if (!user_path) {
    user_path = getenv("OBELIX_USER_PATH");
  }
  if (!user_path) {
    user_path = "./";
  }
  len = strlen(user_path);

  ret -> load_path = str_list_create();
  userdir = (char *) new (len + ((*(user_path + (len - 1)) != '/') ? 2 : 1));
  strcpy(userdir, user_path);
  if (*(userdir + (strlen(userdir) - 1)) != '/') {
    strcat(userdir, "/");
  }

  if (!grammarpath) {
    grammarpath = (char *) new(strlen(ret -> system_dir) + strlen(GRAMMAR_FILE) + 1);
    strcpy(grammarpath, ret -> system_dir);
    strcat(grammarpath, GRAMMAR_FILE);
  }
  debug("system dir: %s", ret -> system_dir);
  debug("user path: %s", userdir);
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

  ret -> load_path = str_list_create();
  list_unshift(ret -> load_path, strdup(ret -> system_dir));
  ret -> ns = ns_create_root(ret, (import_t) scriptloader_import);
  root = ns_import(ret -> ns, NULL);
  if (!data_is_module(root)) {
    error("Error initializing loader scope: %s", data_tostring(root));
    ns_free(ret -> ns);
    ret -> ns = NULL;
  } else {
    if (script_debug) {
      debug("  Created loader namespace");
    }
    list_unshift(ret -> load_path, strdup(userdir));
  }
  data_free(root);
  if (!ret -> ns) {
    error("Could not initialize loader root namespace");
    scriptloader_free(ret);
    ret = NULL;
  } else {
    _loader = ret;
  }
  if (script_debug) {
    debug("scriptloader created");
  }
  return ret;
}

scriptloader_t * scriptloader_get(void) {
  return _loader;
}

void scriptloader_free(scriptloader_t *loader) {
  if (loader) {
    free(loader -> system_dir);
    list_free(loader -> load_path);
    grammar_free(loader -> grammar);
    parser_free(loader -> parser);
    ns_free(loader -> ns);
    free(loader);
  }
}

data_t * scriptloader_load_fromreader(scriptloader_t *loader, char *name, reader_t *reader) {
  data_t   *ret = NULL;
  script_t *script;
  
  if (script_debug) {
    debug("scriptloader_load_fromreader(%s)", name);
  }
  if (!name || !name[0]) {
    ret = data_error(ErrorName, "Cannot load script with no name");
  }
  if (!ret) {
    parser_clear(loader -> parser);
    parser_set(loader -> parser, "name", data_create(String, name));
    if (loader -> ns) {
      parser_set(loader -> parser, "ns", data_create(Pointer, sizeof(namespace_t), loader -> ns));
    }
    parser_parse(loader -> parser, reader);
    script = (script_t *) loader -> parser -> data;
    ret = data_create(Script, script);
  }
  return ret;
}

data_t * scriptloader_load(scriptloader_t *loader, char *name) {
  file_t   *text = NULL;
  str_t    *dummy;
  reader_t *rdr;
  data_t   *ret;
  char     *script_name;

  if (script_debug) {
    debug("scriptloader_load('%s')", name);
  }
  if (!ns_has(loader -> ns, name)) {
    rdr = _scriptloader_open_reader(loader, name);
    script_name = (name && name[0]) ? name : "__root__";
    ret = (rdr)
            ? scriptloader_load_fromreader(loader, script_name, rdr)
            : data_error(ErrorName, "Could not load '%s'", script_name);
    reader_free(rdr);
  }
  return ret;
}

data_t * scriptloader_import(scriptloader_t *loader, array_t *module) {
  data_t   *ret;
  str_t    *path;

  path = (module && array_size(module))
    ? array_join(module, ".")
    : str_wrap("");
  ret = scriptloader_load(loader, str_chars(path));
  str_free(path);
  return ret;
}
