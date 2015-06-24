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

#include <data.h>
#include <exception.h>
#include <file.h>
#include <fsentry.h>
#include <grammarparser.h>
#include <loader.h>
#include <namespace.h>

static file_t *         _scriptloader_open_file(scriptloader_t *, char *, module_t *);
static data_t *         _scriptloader_open_reader(scriptloader_t *, module_t *);
static scriptloader_t * _scriptloader_extend_loadpath(scriptloader_t *, name_t *);
static data_t *         _scriptloader_get_object(scriptloader_t *, int, ...);
static data_t *         _scriptloader_set_value(scriptloader_t *, data_t *, char *, data_t *);
static data_t *         _scriptloader_import_sys(scriptloader_t *, name_t *);

static scriptloader_t * _loader = NULL;

static code_label_t obelix_option_labels[] = {
  { .code = ObelixOptionList,  .label = "ObelixOptionList" },
  { .code = ObelixOptionTrace, .label = "ObelixOptionTrace" },
  { .code = ObelixOptionLAST,  .label = NULL }
};

/*
 * scriptloader_t - static functions
 */

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
  if (script_debug) {
    debug("_scriptloader_open_file('%s', '%s')", basedir, name);
  }
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
    mod -> source = data_create(String, e -> name);
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
  char   *path_entry;
  int     ix;

  assert(loader);
  assert(name);
  if (script_debug) {
    debug("_scriptloader_open_reader('%s')", name_tostring(name));
  }
  for (ix = 0; !text && (ix < name_size(loader -> load_path)); ix++) {
    path_entry = name_get(loader -> load_path, ix);
    text = _scriptloader_open_file(loader, path_entry, mod);
  }
  return (data_t *) text;
}

scriptloader_t * _scriptloader_extend_loadpath(scriptloader_t *loader, name_t *path) {
  char   *entry;
  char   *sanitized_entry;
  name_t *sanitized = name_create(0);
  int     ix;
  int     len;
  
  for (ix = 0; ix < name_size(path); ix++) {
    entry = name_get(path, ix);
    len = strlen(entry);
    sanitized_entry = (char *) new (len + ((*(entry + (len - 1)) != '/') ? 2 : 1));
    strcpy(sanitized_entry, entry);
    if (*(sanitized_entry + (strlen(sanitized_entry) - 1)) != '/') {
      strcat(sanitized_entry, "/");
    }
    name_extend(sanitized, sanitized_entry);
    free(sanitized_entry);
  }
  name_append(loader -> load_path, sanitized);
  name_free(sanitized);
  return loader;
}

static data_t * _scriptloader_get_object(scriptloader_t *loader, int count, ...) {
  va_list   args;
  int       ix;
  object_t *obj;
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
  if (data_is_module(data)) {
    mod = data_as_module(data);
    data = data_create(Object, mod -> obj);
  }
  return data;
}

static data_t * _scriptloader_set_value(scriptloader_t *loader, data_t *obj,
                                        char *name, data_t *value) {
  name_t *n;
  data_t *ret;
  
  n = name_create(1, name);
  ret = data_set(obj, n, value);
  if (!(ret && data_is_exception(ret))) {
    data_free(value);
    ret = NULL;
  }
  name_free(n);
  return ret;
}

static data_t * _scriptloader_import_sys(scriptloader_t *loader, 
                                         name_t *user_path) {
  name_t *name;
  data_t *sys;
  data_t *val;
  data_t *ret;
  
  name = name_create(1, "sys");
  sys = scriptloader_import(loader, name);
  name_free(name);
  if (!data_is_exception(sys)) {
    _scriptloader_extend_loadpath(loader, user_path);
    ret = _scriptloader_set_value(loader, sys, "path", 
                                  data_create(Name, loader -> load_path));
    data_free(sys);
  } else {
    ret = sys;
  }
  return ret;
}

/*
 * scriptloader_t - public functions
 */

typedef grammar_t * (*build_grammar_t)(void);

scriptloader_t * scriptloader_create(char *sys_dir, name_t *user_path, 
                                     char *grammarpath) {
  scriptloader_t   *ret;
  grammar_parser_t *gp;
  data_t           *file;
  data_t           *root;
  char             *userdir;
  int               ix;
  int               len;
  name_t           *upath = NULL;
  build_grammar_t   build_grammar;

  if (script_debug) {
    debug("Creating script loader");
  }
  assert(!_loader);
  ret = NEW(scriptloader_t);
  if (!sys_dir) {
    sys_dir = getenv("OBELIX_SYS_PATH");
  }
  if (!sys_dir) {
    sys_dir = OBELIX_DATADIR;
  }
  len = strlen(sys_dir);
  ret -> system_dir = (char *) new (len + ((*(sys_dir + (len - 1)) != '/') ? 2 : 1));
  strcpy(ret -> system_dir, sys_dir);
  if (*(ret -> system_dir + (strlen(ret -> system_dir) - 1)) != '/') {
    strcat(ret -> system_dir, "/");
  }
  
  if (!user_path || !name_size(user_path)) {
    upath = name_split(getenv("OBELIX_USER_PATH"), ":");
    user_path = upath;
  }
  if (!user_path || !name_size(user_path)) {
    upath = name_create(1, "./");
    user_path = upath;
  }

  if (script_debug) {
    debug("system dir: %s", ret -> system_dir);
    debug("user path: %s", name_tostring(user_path));
  }

  if (!grammarpath) {
    if (script_debug) {
      debug("Using stock, compiled-in grammar");
    }
    build_grammar = (build_grammar_t) resolve_function("build_grammar");
    ret -> grammar = build_grammar();
  } else {
    if (script_debug) {
      debug("grammar file: %s", grammarpath);
    }
    file = data_create(File, grammarpath);
    assert(file_isopen(data_as_file(file)));
    gp = grammar_parser_create(file);
    ret -> grammar = grammar_parser_parse(gp);
    assert(ret -> grammar);
    grammar_parser_free(gp);
    data_free(file);
  }

  if (script_debug) {
    debug("  Loaded grammar");
  }
  ret -> options = data_array_create((int) ObelixOptionLAST);
  for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
    scriptloader_set_option(ret, ix, 0L);
  }

  ret -> parser = parser_create(ret -> grammar);
  if (script_debug) {
    debug("  Created parser");
  }

  ret -> load_path = name_create(1, ret -> system_dir);
  ret -> ns = ns_create("loader", ret, (import_t) scriptloader_load);
  root = ns_import(ret -> ns, NULL);
  if (!data_is_module(root)) {
    error("Error initializing loader scope: %s", data_tostring(root));
    ns_free(ret -> ns);
    ret -> ns = NULL;
  } else {
    if (script_debug) {
      debug("  Created loader namespace");
    }
    _scriptloader_import_sys(ret, user_path);
  }
  data_free(root);
  if (!ret -> ns) {
    error("Could not initialize loader root namespace");
    scriptloader_free(ret);
    ret = NULL;
  } else {
    _loader = ret;
  }
  name_free(upath);
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
    name_free(loader -> load_path);
    array_free(loader -> options);
    grammar_free(loader -> grammar);
    parser_free(loader -> parser);
    ns_free(loader -> ns);
    free(loader);
  }
}

scriptloader_t * scriptloader_set_option(scriptloader_t *loader, obelix_option_t option, long value) {
  data_t *opt = data_create(Int, value);
  
  array_set(loader -> options, (int) option, opt);
  return loader;
}

long scriptloader_get_option(scriptloader_t *loader, obelix_option_t option) {
  data_t *opt;
  
  opt = data_array_get(loader -> options, (int) option);
  return data_intval(opt);
}

data_t * scriptloader_load_fromreader(scriptloader_t *loader, module_t *mod, data_t *reader) {
  data_t   *ret = NULL;
  script_t *script;
  char     *name;
  
  if (script_debug) {
    debug("scriptloader_load_fromreader('%s')", name_tostring(mod -> name));
  }
  parser_clear(loader -> parser);
  parser_set(loader -> parser, "module", data_create(Module, mod));
  name = strdup((name_size(mod -> name)) ? name_tostring(mod -> name) : "__root__");
  parser_set(loader -> parser, "name", data_create(String, name));
  parser_set(loader -> parser, "options", data_create_list(loader -> options));
  ret = parser_parse(loader -> parser, reader);
  if (!ret) {
    ret = data_copy(parser_get(loader -> parser, "script"));
  }
  return ret;
}

data_t * scriptloader_import(scriptloader_t *loader, name_t *name) {
  return ns_import(loader -> ns, name);
}

data_t * scriptloader_load(scriptloader_t *loader, module_t *mod) {
  data_t *rdr;
  data_t *ret;
  char   *script_name;
  name_t *name = mod -> name;

  assert(loader);
  assert(name);
  script_name = strdup((name && name_size(mod -> name)) ? name_tostring(mod -> name) : "__root__");
  if (script_debug) {
    debug("scriptloader_load('%s')", script_name);
  }
  if (mod -> state == ModStateLoading) {
    rdr = _scriptloader_open_reader(loader, mod);
    ret = (rdr)
            ? scriptloader_load_fromreader(loader, mod, rdr)
            : data_exception(ErrorName, "Could not load '%s'", script_name);
    data_free(rdr);
  } else {
    debug("Module '%s' is already active. Skipped.", 
          name_tostring(mod -> name));
  }
  free(script_name);
  return ret;
}

data_t * scriptloader_run(scriptloader_t *loader, name_t *name, array_t *args, dict_t *kwargs) {
  data_t   *data;
  object_t *obj;
  data_t   *sys;
  
  sys = _scriptloader_get_object(loader, 1, "sys");
  if (sys && !data_is_exception(sys)) {
    _scriptloader_set_value(loader, sys, "argv", data_create_list(args));
    data_free(sys);
    data = ns_execute(loader -> ns, name, args, kwargs);
    if (obj = data_as_object(data)) {
      data = data_copy(obj -> retval);
      object_free(obj);
    } else if (!data_is_exception(data)) {
      data = data_exception(ErrorInternalError,
			    "ns_execute '%s' returned '%s', a %s",
			    name_tostring(name),
			    data_tostring(data),
			    data_typename(data));
    }
  } else {
    data = (sys) ? sys : data_exception(ErrorName, "Could not resolve module 'sys'");
  }
  return data;
}
