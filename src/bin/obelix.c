/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
 * 
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <data.h>
#include <exception.h>
#include <file.h>
#include <lexer.h>
#include <loader.h>
#include <logging.h>
#include <name.h>
#include <namespace.h>
#include <resolve.h>
#include <script.h>

void debug_settings(char *debug) {
  int      debug_all = 0;
  array_t *cats;
  int      ix;
  
  logging_reset();
  if (debug) {
    debug("debug optarg: %s", debug);
    cats = array_split(debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
}

int run_script(scriptloader_t *loader, name_t *name, array_t *argv) {
  data_t      *ret;
  object_t    *obj;
  module_t    *mod;
  int          retval;
  exception_t *ex;
  int          type;
  
  ret = scriptloader_run(loader, name, argv, NULL);
  if (script_debug) {
    debug("Exiting with exit code %s", data_tostring(ret));
  }
  if (ex = data_as_exception(ret)) {
    if (ex -> code != ErrorExit) {
      error("Error: %s", data_tostring(ret));
      retval = 0 - (int) ex -> code;
    } else {
      retval = data_intval(ex -> throwable);
    }
  } else {
    retval = data_intval(ret);
  }    
  data_free(ret);
  return retval;  
}

name_t * build_name(char *scriptname) {
  char   *buf;
  char   *ptr;
  name_t *name;
  
  buf = strdup(scriptname);
  while (ptr = strchr(buf, '/')) {
    *ptr = '.';
  }
  if (ptr = strrchr(buf, '.')) {
    if (!strcmp(ptr, ".obl")) {
      *ptr = 0;
    }
  }
  name = name_split(buf, ".");
  free(buf);
  return name;
}

int main(int argc, char **argv) {
  char           *grammar = NULL;
  char           *debug = NULL;
  char           *basepath = NULL;
  array_t        *path;
  name_t         *name;
  array_t        *obl_argv;
  int             ix;
  char           *syspath = NULL;
  int             opt;
  scriptloader_t *loader;
  int             retval = 0;
  int             list = 0;
  int             trace = 0;

  while ((opt = getopt(argc, argv, "s:g:d:p:v:lt")) != -1) {
    switch (opt) {
      case 's':
        syspath = optarg;
        break;
      case 'g':
        grammar = optarg;
        break;
      case 'd':
        debug = optarg;
        break;
      case 'p':
        basepath = optarg;
        break;
      case 'v':
        log_level = atoi(optarg);
        break;
      case 'l':
        list = 1;
        break;
      case 't':
	trace = 1;
	list = 1;
	break;
    }
  }
  if (argc == optind) {
    fprintf(stderr, "Usage: obelix <filename> [options ...]\n");
    exit(1);
  }
  debug_settings(debug);
  
  path = (basepath) ? array_split(basepath, ":") : str_array_create(0);
  array_push(path, getcwd(NULL, 0));
  
  loader = scriptloader_create(syspath, path, grammar);
  if (loader) {
    scriptloader_set_option(loader, ObelixOptionList, list);
    scriptloader_set_option(loader, ObelixOptionTrace, trace);

    name = build_name(argv[optind]);
    obl_argv = str_array_create(argc - optind);
    for (ix = optind + 1; ix < argc; ix++) {
      array_push(obl_argv, data_create(String, argv[ix]));
    }
    retval = run_script(loader, name, obl_argv);
    array_free(obl_argv);
    name_free(name);
    scriptloader_free(loader);
  }
  return retval;
}

