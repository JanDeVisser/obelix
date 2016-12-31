/*
 * /obelix/src/parser/loader.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
#include <time.h>

#include "obelix.h"
#include <data.h>
#include <exception.h>
#include <file.h>
#include <fsentry.h>
#include <grammarparser.h>
#include <thread.h>

extern grammar_t *      grammar_build(void);

static inline void      _scriptloader_init(void);
static file_t *         _scriptloader_open_file(scriptloader_t *, char *, module_t *);
static data_t *         _scriptloader_open_reader(scriptloader_t *, module_t *);
static data_t *         _scriptloader_get_object(scriptloader_t *, int, ...);
static data_t *         _scriptloader_set_value(scriptloader_t *, data_t *, char *, data_t *);
static data_t *         _scriptloader_import_sys(scriptloader_t *, array_t *);

static scriptloader_t * _scriptloader_new(scriptloader_t *, va_list);
static void             _scriptloader_free(scriptloader_t *);
static char *           _scriptloader_allocstring(scriptloader_t *);
static data_t *         _scriptloader_call(scriptloader_t *, array_t *, dict_t *);
static data_t *         _scriptloader_resolve(scriptloader_t *, char *);

static _unused_ code_label_t obelix_option_labels[] = {
  { .code = ObelixOptionList,  .label = "ObelixOptionList" },
  { .code = ObelixOptionTrace, .label = "ObelixOptionTrace" },
  { .code = ObelixOptionLAST,  .label = NULL }
};

static vtable_t _vtable_ScriptLoader[] = {
  { .id = FunctionNew,         .fnc = (void_t) _scriptloader_new },
  { .id = FunctionFree,        .fnc = (void_t) _scriptloader_free },
  { .id = FunctionAllocString, .fnc = (void_t) _scriptloader_allocstring },
  { .id = FunctionCall,        .fnc = (void_t) _scriptloader_call },
  { .id = FunctionResolve,     .fnc = (void_t) _scriptloader_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

int ScriptLoader = -1;

/* ------------------------------------------------------------------------ */

void _scriptloader_init(void) {
  if (ScriptLoader < 0) {
    typedescr_register(ScriptLoader, scriptloader_t);
  }
}

/* -- S C R I P T L O A D E R   D A T A   F U N C T I O N S --------------- */

scriptloader_t * _scriptloader_new(scriptloader_t *loader, va_list args) {
  char             *sys_dir = va_arg(args, char *);
  array_t          *user_path = va_arg(args, array_t *);
  char             *grammarpath = va_arg(args, char *);
  grammar_parser_t *gp;
  data_t           *file;
  data_t           *root;
  data_t           *sys;
  int               ix;
  int               len;
  log_timestamp_t  *ts;

  ts = log_timestamp_start(obelix);
  resolve_library("liboblparser");
  resolve_library("libscriptparse");
  if (!sys_dir) {
    sys_dir = getenv("OBELIX_SYS_PATH");
  }
  if (!sys_dir) {
    sys_dir = OBELIX_DATADIR;
  }
  len = strlen(sys_dir);
  loader -> system_dir = (char *) new (len + ((*(sys_dir + (len - 1)) != '/') ? 2 : 1));
  strcpy(loader -> system_dir, sys_dir);
  if (*(loader -> system_dir + (strlen(loader -> system_dir) - 1)) != '/') {
    strcat(loader -> system_dir, "/");
  }

  if (!user_path || !array_size(user_path)) {
    if (user_path) {
      array_free(user_path);
    }
    user_path = array_split(getenv("OBELIX_USER_PATH"), ":");
  }
  if (!user_path || !array_size(user_path)) {
    if (!user_path) {
      str_array_create(1);
    }
    array_push(user_path, strdup("./"));
  }

  debug(obelix, "system dir: %s", loader -> system_dir);
  debug(obelix, "user path: %s", array_tostring(user_path));

  if (!grammarpath || !*grammarpath) {
    debug(obelix, "Using stock, compiled-in grammar");
    loader -> grammar = grammar_build();
  } else {
    debug(obelix, "grammar file: %s", grammarpath);
    file = (data_t *) file_open(grammarpath);
    assert(file_isopen(data_as_file(file)));
    gp = grammar_parser_create(file);
    loader -> grammar = grammar_parser_parse(gp);
    assert(loader -> grammar);
    grammar_parser_free(gp);
    data_free(file);
  }

  debug(obelix, "  Loaded grammar");
  loader -> parser = NULL;
  loader -> options = data_array_create((int) ObelixOptionLAST);
  for (ix = 0; ix < (int) ObelixOptionLAST; ix++) {
    scriptloader_set_option(loader, ix, 0L);
  }

  loader -> load_path = data_create(List, 1, str_to_data(loader -> system_dir));
  loader -> ns = ns_create("loader", loader, (import_t) scriptloader_load);
  root = ns_import(loader -> ns, NULL);
  if (!data_is_module(root)) {
    error("Error initializing loader scope: %s", data_tostring(root));
    ns_free(loader -> ns);
    loader -> ns = NULL;
  } else {
    debug(obelix, "  Created loader namespace");
    sys = _scriptloader_import_sys(loader, user_path);
    if (!data_is_module(sys)) {
      error("Error initializing loader scope: %s", data_tostring(sys));
      ns_free(loader -> ns);
      loader -> ns = NULL;
    }
    data_free(sys);
  }
  data_free(root);
  if (!loader -> ns) {
    error("Could not initialize loader root namespace");
    loader = NULL;
  } else {
    data_thread_set_kernel((data_t *) loader);
    loader -> cookie = strrand(NULL, COOKIE_SZ - 1);
    loader -> lastused = time(NULL);
  }
  log_timestamp_end(obelix, ts, "scriptloader created in ");
  return loader;
}

void _scriptloader_free(scriptloader_t *loader) {
  if (loader) {
    free(loader -> system_dir);
    data_free(loader -> load_path);
    array_free(loader -> options);
    free(loader -> cookie);
    parser_free(loader -> parser);
    grammar_free(loader -> grammar);
    ns_free(loader -> ns);
  }
}

char *_scriptloader_allocstring(scriptloader_t *loader) {
  char *buf;

  asprintf(&buf, "Loader(%s)", loader -> system_dir);
  return buf;
}

data_t *_scriptloader_call(scriptloader_t *loader, array_t *args, dict_t *kwargs) {
  name_t  *name = data_as_name(data_array_get(args, 0));
  array_t *args_shifted = array_slice(args, 1, -1);
  data_t  *ret;

  ret = scriptloader_run(loader, name, args_shifted, kwargs);
  free(args_shifted);
  return ret;
}

data_t * _scriptloader_resolve(scriptloader_t *loader, char *name) {
  if (!strcmp(name, "list")) {
    return int_as_bool(scriptloader_get_option(loader, ObelixOptionList));
  } else if (!strcmp(name, "trace")) {
    return int_as_bool(scriptloader_get_option(loader, ObelixOptionTrace));
  } else if (!strcmp(name, "loadpath")) {
    return data_copy((data_t *) loader -> load_path);
  } else if (!strcmp(name, "systempath")) {
    return str_to_data(loader -> system_dir);
  } else if (!strcmp(name, "grammar")) {
    return data_copy((data_t *) loader -> grammar);
  } else if (!strcmp(name, "namespace")) {
    return data_copy((data_t *) loader -> ns);
  }
  return NULL;
}

data_t * _scriptloader_set(scriptloader_t *loader, char *name, data_t *value) {
  if (!strcmp(name, "list")) {
    scriptloader_set_option(loader, ObelixOptionList, data_intval(value));
    return value;
  } else if (!strcmp(name, "trace")) {
    scriptloader_set_option(loader, ObelixOptionTrace, data_intval(value));
    return value;
  }
  return data_exception(ErrorType,
                        "Cannot set '%s' on objects of type '%s'",
                        name, data_typename(loader));
}

/* ------------------------------------------------------------------------ */

static file_t * _scriptloader_open_file(scriptloader_t *loader,
                                        char *basedir,
                                        module_t *mod) {
  char      *fname;
  char      *ptr;
  fsentry_t *e;
  fsentry_t *init;
  file_t    *ret;
  char      *name;
  char      *buf;
  name_t    *n = mod -> name;

  assert(*(basedir + (strlen(basedir) - 1)) == '/');
  buf = strdup(name_tostring_sep(n, "/"));
  name = buf;
  debug(obelix, "_scriptloader_open_file('%s', '%s')", basedir, name);
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
    fsentry_free(e);
    strcat(fname, ".obl");
    e = fsentry_create(fname);
  }
  if ((e != NULL) && fsentry_isfile(e) && fsentry_canread(e)) {
    debug(obelix, "_scriptloader_open_file('%s', '%s') -> '%s'", basedir, name, e -> name);
    ret = fsentry_open(e);
    mod -> source = str_to_data(e -> name);
    assert(ret -> fh > 0);
  } else {
    ret = NULL;
  }
  free(buf);
  fsentry_free(e);
  free(fname);
  return ret;
}

data_t * _scriptloader_open_reader(scriptloader_t *loader, module_t *mod) {
  file_t *text = NULL;
  name_t *name = mod -> name;
  data_t *path_entry;
  int     ix;

  assert(loader);
  assert(name);
  debug(obelix, "_scriptloader_open_reader('%s')", name_tostring(name));
  for (ix = 0; !text && (ix < data_list_size(loader -> load_path)); ix++) {
    path_entry = data_list_get(loader -> load_path, ix);
    text = _scriptloader_open_file(loader, data_tostring(path_entry), mod);
  }
  return (data_t *) text;
}

static data_t * _scriptloader_get_object(scriptloader_t *loader, int count, ...) {
  va_list   args;
  int       ix;
  module_t *mod;
  data_t   *data = NULL;
  name_t   *name;

  va_start(args, count);
  name = name_create(0);
  for (ix = 0; ix < count; ix++) {
    name_extend(name, va_arg(args, char *));
  }
  va_end(args);
  data = ns_get(loader -> ns, name);
  name_free(name);
  mod = data_as_module(data);
  if (mod) {
    data = (data_t *) object_copy(mod -> obj);
  }
  return data;
}

static data_t * _scriptloader_set_value(scriptloader_t *loader, data_t *obj,
                                        char *name, data_t *value) {
  name_t *n;
  data_t *ret;

  n = name_create(1, name);
  ret = data_set(obj, n, value);
  if (!data_is_exception(ret)) {
    ret = obj;
  }
  data_free(value);
  name_free(n);
  return ret;
}

static data_t * _scriptloader_import_sys(scriptloader_t *loader,
                                         array_t *user_path) {
  name_t *name;
  data_t *sys;
  data_t *ret;

  name = name_create(1, "sys");
  sys = scriptloader_import(loader, name);
  name_free(name);
  if (!data_is_exception(sys)) {
    scriptloader_extend_loadpath(loader, user_path);
    ret = _scriptloader_set_value(loader, sys, "path",
                                  data_copy(loader -> load_path));
    if (!data_is_exception(ret)) {
      ret = sys;
    } else {
      data_free(sys);
    }
  } else {
    ret = sys;
  }
  return ret;
}

/* -- S C R I P T L O A D E R _ T   P U B L I C   F U N C T I O N S ------- */

scriptloader_t * scriptloader_create(char *sys_dir, array_t *user_path,
                                     char *grammarpath) {
  _scriptloader_init();
  debug(obelix, "Creating script loader");
  return (scriptloader_t *) data_create(ScriptLoader, sys_dir, user_path, grammarpath);
}

scriptloader_t * scriptloader_get(void) {
  return (scriptloader_t *) data_thread_kernel();
}

scriptloader_t * scriptloader_set_options(scriptloader_t *loader, array_t *options) {
  int     ix;
  data_t *opt;

  for (ix = 0; ix < (int) ObelixOptionLAST; ix++) {
    if ((opt = data_array_get(options, ix))) {
      scriptloader_set_option(loader, ix, data_intval(opt));
    }
  }
  return loader;
}

scriptloader_t * scriptloader_set_option(scriptloader_t *loader, obelix_option_t option, long value) {
  data_t *opt = int_to_data(value);

  array_set(loader -> options, (int) option, opt);
  return loader;
}

long scriptloader_get_option(scriptloader_t *loader, obelix_option_t option) {
  data_t *opt;

  opt = data_array_get(loader -> options, (int) option);
  return data_intval(opt);
}

scriptloader_t * scriptloader_add_loadpath(scriptloader_t *loader, char *pathentry) {
  char *sanitized_entry;
  int   len;

  len = strlen(pathentry);
  sanitized_entry = (char *) new (len + ((*(pathentry + (len - 1)) != '/') ? 2 : 1));
  strcpy(sanitized_entry, pathentry);
  if (*(sanitized_entry + (strlen(sanitized_entry) - 1)) != '/') {
    strcat(sanitized_entry, "/");
  }
  data_list_push(loader -> load_path, str_to_data(sanitized_entry));
  free(sanitized_entry);
  return loader;
}

scriptloader_t * scriptloader_extend_loadpath(scriptloader_t *loader, array_t *path) {
  int ix;

  for (ix = 0; ix < array_size(path); ix++) {
    scriptloader_add_loadpath(loader, str_array_get(path, ix));
  }
  debug(obelix, "loadpath extended to %s", data_tostring((data_t *) loader -> load_path));
  return loader;
}

data_t * scriptloader_load_fromreader(scriptloader_t *loader, module_t *mod, data_t *reader) {
  char   *name;
  data_t *ret;

  debug(obelix, "scriptloader_load_fromreader('%s')", name_tostring(mod -> name));
  if (!loader -> parser) {
    loader -> parser = parser_create(loader -> grammar);
    debug(obelix, "Created parser");
    parser_set(loader -> parser, "module", (data_t *) mod_copy(mod));
    name = (name_size(mod -> name)) ? name_tostring(mod -> name) : "__root__";
    parser_set(loader -> parser, "name", str_to_data(name));
    parser_set(loader -> parser, "options", data_create_list(loader -> options));
    parser_start(loader -> parser);
  }
  ret = parser_parse_reader(loader -> parser, reader);
  if (!ret) {
    ret = parser_get(loader -> parser, "in_statement");
    if (!ret) {
      ret = data_false();
    }
  }
  return ret;
}

data_t * scriptloader_import(scriptloader_t *loader, name_t *name) {
  return ns_import(loader -> ns, name);
}

data_t * scriptloader_load(scriptloader_t *loader, module_t *mod) {
  data_t      *rdr;
  data_t      *ret;
  char        *script_name;
  name_t      *name = mod -> name;

  assert(loader);
  assert(name);
  script_name = strdup((name && name_size(mod -> name)) ? name_tostring(mod -> name) : "__root__");
  debug(obelix, "scriptloader_load('%s')", script_name);
  if (mod -> state == ModStateLoading) {
    if ((rdr = _scriptloader_open_reader(loader, mod))) {
      ret = scriptloader_load_fromreader(loader, mod, rdr);
      if (!data_is_exception(ret)) {
        ret = parser_end(loader -> parser);
      }
      if (!data_is_exception(ret)) {
        ret = data_copy(parser_get(loader -> parser, "script"));
      }
      parser_free(loader -> parser);
      loader -> parser = NULL;
      data_free(rdr);
    } else {
      ret = data_exception(ErrorName, "Could not load '%s'", script_name);
    }
  } else {
    debug(obelix, "Module '%s' is already active. Skipped.", name_tostring(mod -> name));
  }
  free(script_name);
  return ret;
}

data_t * scriptloader_run(scriptloader_t *loader, name_t *name, array_t *args, dict_t *kwargs) {
  data_t          *data;
  object_t        *obj;
  data_t          *sys;
  log_timestamp_t *ts;

  ts = log_timestamp_start(obelix);
  debug(obelix, "scriptloader_run(%s)", name_tostring(name));
  sys = _scriptloader_get_object(loader, 1, "sys");
  if (sys && !data_is_exception(sys)) {
    _scriptloader_set_value(loader, sys, "argv", data_create_list(args));
    if (scriptloader_get_option(loader, ObelixOptionTrace)) {
      logging_enable("trace");
    }
    data_free(sys);
    data = ns_execute(loader -> ns, name, args, kwargs);
    if ((obj = data_as_object(data))) {
      data = data_copy(obj -> retval);
      object_free(obj);
    } else if (!data_is_exception(data)) {
      data = data_exception(ErrorInternalError,
                            "ns_execute '%s' returned '%s', a %s",
                            name_tostring(name),
                            data_tostring(data),
                            data_typename(data));
    }
    logging_disable("trace");
  } else {
    data = (sys) ? sys : data_exception(ErrorName, "Could not resolve module 'sys'");
  }
  data_thread_clear_exit_code();
  log_timestamp_end(obelix, ts, "scriptloader_run(%s) = %s in ", name_tostring(name), data_tostring(data));
  return data;
}

data_t * scriptloader_eval(scriptloader_t *loader, data_t *src) {
  module_t *root;
  data_t   *data;
  script_t *script;

  data = ns_get(loader -> ns, NULL);
  if (data_is_module(data)) {
    root = data_as_module(data);
    data = scriptloader_load_fromreader(loader, root, src);
    if (!data_is_exception(data)) {
      if (!data_intval(data)) {
        parser_end(loader -> parser);
        data = data_copy(parser_get(loader -> parser, "script"));
        if (data_is_script(data)) {
          script = data_as_script(data);
          data = closure_eval(root -> closure, script);
          debug(obelix, "closure_eval: %s", data_tostring(data));
          if (!data) {
            data = data_null();
          }
          script_free(script);
        }
        parser_free(loader -> parser);
        loader -> parser = NULL;
      } else {
        data = NULL;
      }
    }
  }
  return data;
}
