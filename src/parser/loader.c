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
#include <grammarparser.h>
#include <loader.h>
#include <namespace.h>

static file_t *         _scriptloader_open_file(scriptloader_t *, char *, name_t *);
static reader_t *       _scriptloader_open_reader(scriptloader_t *, name_t *);
static scriptloader_t * _scriptloader_extend_loadpath(scriptloader_t *, name_t *);

static scriptloader_t * _loader = NULL;

/*
 * scriptloader_t - static functions
 */

static file_t * _scriptloader_open_file(scriptloader_t *loader, 
                                        char *basedir, 
                                        name_t *n) {
  char      *fname;
  char      *ptr;
  fsentry_t *e;
  fsentry_t *init;
  file_t    *ret;
  char      *name;

  assert(*(basedir + (strlen(basedir) - 1)) == '/');
  name = name_tostring_sep(n, "/");
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
    assert(ret -> fh > 0);
  } else {
    ret = NULL;
  }
  fsentry_free(e);
  free(fname);
  return ret;
}

reader_t * _scriptloader_open_reader(scriptloader_t *loader, name_t *name) {
  file_t   *text = NULL;
  char     *path_entry;
  reader_t *rdr = NULL;
  int       ix;

  assert(loader);
  assert(name);
  if (script_debug) {
    debug("_scriptloader_open_reader('%s')", name);
  }
  for (ix = 0; ix < name_size(loader -> load_path); ix++) {
    path_entry = name_get(loader -> load_path, ix);
    text = _scriptloader_open_file(loader, path_entry, name);
  }
  if (text) {
    rdr = (reader_t *) text;
  }
  return rdr;
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

/*
 * scriptloader_t - public functions
 */

#define GRAMMAR_FILE    "grammar.txt"
/* #define OBELIX_SYS_PATH "/usr/share/obelix" */
#define OBELIX_SYS_PATH "install/share/"

scriptloader_t * scriptloader_create(char *sys_dir, name_t *user_path, char *grammarpath) {
  scriptloader_t   *ret;
  grammar_parser_t *gp;
  file_t           *file;
  data_t           *root;
  reader_t         *rdr;
  char             *userdir;
  int               ix;
  int               len;
  name_t           *upath = NULL;

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
  
  if (!user_path || !name_size(user_path)) {
    upath = name_split(getenv("OBELIX_USER_PATH"), ":");
    user_path = upath;
  }
  if (!user_path || !name_size(user_path)) {
    upath = name_create(1, "./");
    user_path = upath;
  }

  if (!grammarpath) {
    grammarpath = (char *) new(strlen(ret -> system_dir) + strlen(GRAMMAR_FILE) + 1);
    strcpy(grammarpath, ret -> system_dir);
    strcat(grammarpath, GRAMMAR_FILE);
  }
  debug("system dir: %s", ret -> system_dir);
  debug("user path: %s", name_tostring(user_path));
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

  ret -> load_path = name_create(1, ret -> system_dir);
  ret -> ns = ns_create_root(ret, (import_t) scriptloader_load);
  root = ns_import(ret -> ns, NULL);
  if (!data_is_module(root)) {
    error("Error initializing loader scope: %s", data_tostring(root));
    ns_free(ret -> ns);
    ret -> ns = NULL;
  } else {
    if (script_debug) {
      debug("  Created loader namespace");
    }
    _scriptloader_extend_loadpath(ret, user_path);
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

data_t * scriptloader_import(scriptloader_t *loader, name_t *name) {
  return ns_import(loader -> ns, name);
}

data_t * scriptloader_load(scriptloader_t *loader, name_t *name) {
  reader_t *rdr;
  data_t   *ret;
  char     *script_name;

  assert(loader);
  assert(name);
  script_name = (name_size(name)) ? name_tostring(name) : "__root__";
  if (script_debug) {
    debug("scriptloader_load('%s')", script_name);
  }
  ret = ns_get(loader -> ns, name);
  if (!ret) {
    rdr = _scriptloader_open_reader(loader, name);
    ret = (rdr)
            ? scriptloader_load_fromreader(loader, script_name, rdr)
            : data_error(ErrorName, "Could not load '%s'", script_name);
    reader_free(rdr);
  }
  return ret;
}
