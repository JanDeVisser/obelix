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
#include <file.h>
#include <lexer.h>

#include <loader.h>
#include <name.h>
#include <namespace.h>
#include <script.h>

#include "logging.h"

void debug_settings(char *debug) {
  int      debug_all = 0;
  array_t *cats;
  int      ix;
  
  if (debug) {
    debug("debug optarg: %s", debug);
    cats = array_split(debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
}

int run_script(scriptloader_t *loader, name_t *name) {
  data_t   *data;
  data_t   *ret;
  object_t *obj;
  module_t *mod;
  int       retval;
  
  data = scriptloader_import(loader, name);
  if (data_is_module(data)) {
    mod = mod_copy(data_moduleval(data));
    data_free(data);
    obj = object_copy(mod_get(mod));
    data = data_create(Object, obj);
    ret = data_execute(data, "main", NULL, NULL);
    data_free(data);
    object_free(obj);
    mod_free(mod);
  }
  debug("Exiting with exit code %s", data_tostring(ret));
  retval = (ret && data_type(ret) == Int) ? (int) ret -> intval : 0;
  data_free(ret);
  return retval;  
}

int main(int argc, char **argv) {
  char           *grammar = NULL;
  char           *debug = NULL;
  char           *basepath = NULL;
  name_t         *path;
  name_t         *name;
  char           *syspath = NULL;
  int             opt;
  scriptloader_t *loader;
  int             retval;

  while ((opt = getopt(argc, argv, "s:g:d:p:l:")) != -1) {
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
        basepath = strdup(optarg);
        break;
      case 'l':
        log_level = atoi(optarg);
        break;
    }
  }
  if (argc == optind) {
    fprintf(stderr, "Usage: obelix <filename>\n");
    exit(1);
  }
  debug_settings(debug);
  
  if (!basepath) {
    basepath = getcwd(NULL, 0);
  }
  
  path = name_split(basepath, ":");
  free(basepath);
  name = name_split(argv[optind], ".");
  
  loader = scriptloader_create(syspath, path, grammar);
  retval = run_script(loader, name);
  scriptloader_free(loader);
  return retval;
}

