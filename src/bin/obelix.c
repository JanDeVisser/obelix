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
#include <socket.h>
#include <exception.h>
#include <file.h>
#include <lexer.h>
#include <loader.h>
#include <logging.h>
#include <name.h>
#include <namespace.h>
#include <resolve.h>
#include <script.h>

#include "obelix.h"
#include "server.h"

static void       _oblserver_init(void) __attribute__((constructor(130)));

static void       _obelix_free(obelix_t *);
static char *     _obelix_tostring(obelix_t *);

static data_t *   _obelix_get(data_t *, char *, array_t *, dict_t *);
static data_t *   _obelix_run(data_t *, char *, array_t *, dict_t *);

static void       _obelix_debug_settings(obelix_t *);
static int        _obelix_cmdline(obelix_t *);

static vtable_t _vtable_obelix[] = {
  { .id = FunctionFree,         .fnc = (void_t) _obelix_free },
  { .id = FunctionStaticString, .fnc = (void_t) _obelix_tostring },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methoddescr_obelix[] = {
  { .type = Any,    .name = "obelix", .method = _obelix_get, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "run",    .method = _obelix_run, .argtypes = { String, Any, NoType },    .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,     .method = NULL,        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

       int       Obelix = -1;
static obelix_t *_obelix;

/* ------------------------------------------------------------------------ */

void _obelix_init(void) {
  int       ix;
 
  Obelix = typedescr_create_and_register(Obelix, "obelix", 
                                         _vtable_obelix, _methoddescr_obelix);
  
  if (script_debug) {
    debug("Creating obelix kernel");
  }
  _obelix = data_new(Obelix, obelix_t);
  _obelix -> options = data_array_create((int) ObelixOptionLAST);
  for (ix = 0; ix < (int) ObelixOptionLAST; ix++) {
    obelix_set_option(_obelix, ix, 0L);
  }
}

/* -- S E R V E R  T Y P E  F U N C T I O N S  ---------------------------- */

char * _obelix_tostring(obelix_t *obelix) {
  return "Obelix Kernel";
}

void _obelix_free(obelix_t *obelix) {
  if (obelix) {
    array_free(obelix -> options);
    array_free(obelix -> script_args);
    name_free(obelix -> script);
  }
}

static data_t * _obelix_get(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) self;
  (void) name;
  (void) args;
  (void) kwargs;
  
  return (data_t *) _obelix;
}

static data_t * _obelix_run(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  name_t  *script;
  array_t *script_args;
  data_t  *ret;
  data_t  *name_arg = data_array_get(args, 0);
  
  (void) self;
  (void) name;
  
  script = (data_is_name(name_arg)) 
          ? name_copy(name_arg) 
          : obelix_build_name(data_tostring(name_arg));
  script_args = array_slice(args, 1, -1);
  ret = obelix_run(_obelix, script, script_args, kwargs);
  array_free(script_args);
  name_free(script);
  return ret;
}

/* ------------------------------------------------------------------------ */

void _obelix_debug_settings(obelix_t *obelix) {
  array_t *cats;
  int      ix;
  
  logging_reset();
  logging_set_level(obelix -> log_level);
  if (obelix -> debug) {
    debug("debug optarg: %s", obelix -> debug);
    cats = array_split(obelix -> debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
}

int _obelix_cmdline(obelix_t *obelix) {
  int             retval = 0;
  exception_t    *ex;
  data_t         *ret;
  
  ret = obelix_run(obelix, obelix -> script, obelix -> script_args, NULL);
  if (ex = data_as_exception(ret)) {
    fprintf(stderr, "Error: %s\n", ex -> msg);
    retval = 0 - (int) ex -> code;
  } else {
    retval = data_intval(ret);
  }
  data_free(ret);
  return retval;
}

/* ------------------------------------------------------------------------ */

name_t * obelix_build_name(char *scriptname) {
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

/* -- O B E L I X  P U B L I C  F U N C T I O N S ------------------------- */

obelix_t * obelix_initialize(int argc, char **argv) {
  int opt;
  int ix;

  if (script_debug) {
    debug("Initialize obelix kernel");
  }
  _obelix -> argc = argc;
  _obelix -> argv = argv;
  while ((opt = getopt(argc, argv, "s:g:d:p:v:ltS")) != -1) {
    switch (opt) {
      case 's':
        _obelix -> syspath = optarg;
        break;
      case 'g':
        _obelix -> grammar = optarg;
        break;
      case 'd':
        _obelix -> debug = optarg;
        break;
      case 'p':
        _obelix -> basepath = optarg;
        break;
      case 'v':
        _obelix -> log_level = atoi(optarg);
        break;
      case 'l':
        obelix_set_option(_obelix, ObelixOptionList, 1);
        break;
      case 't':
        obelix_set_option(_obelix, ObelixOptionList, 1);
        obelix_set_option(_obelix, ObelixOptionTrace, 1);
        break;
      case 'S':
        _obelix -> server = 14400;
        break;
    }
  }
  if (!_obelix -> server) {
    if (argc > optind) {
      _obelix -> script = obelix_build_name(argv[optind]);
      if (argc > optind + 1) {
        _obelix -> script_args = str_array_create(argc - optind);
        for (ix = optind + 1; ix < argc; ix++) {
          array_push(_obelix -> script_args, str_wrap(argv[ix]));
        }
      }
    } else {
      return NULL;
    }
  }
  _obelix_debug_settings(_obelix);
  return _obelix;
}

obelix_t * obelix_set_option(obelix_t *obelix, obelix_option_t option, long value) {
  data_t *opt = data_create(Int, value);
  
  array_set(obelix -> options, (int) option, opt);
  return obelix;
}

long obelix_get_option(obelix_t *obelix, obelix_option_t option) {
  data_t *opt;
  
  opt = data_array_get(obelix -> options, (int) option);
  return data_intval(opt);
}

scriptloader_t * obelix_create_loader(obelix_t *obelix) {
  array_t        *path;
  scriptloader_t *loader;
  
  path = (obelix -> basepath) ? array_split(obelix -> basepath, ":") 
                              : str_array_create(0);
  array_push(path, getcwd(NULL, 0));
  
  loader = scriptloader_create(obelix -> syspath, path, obelix -> grammar);
  if (loader) {
    scriptloader_set_options(loader, obelix -> options);
  }
  array_free(path);
  if (!obelix -> loaders) {
    obelix -> loaders = datadata_dict_create();
  }
  dict_put(obelix -> loaders, 
           str_wrap(loader -> cookie), 
           scriptloader_copy(loader));
  return loader;
}

scriptloader_t * obelix_get_loader(obelix_t *obelix, char *cookie) {
  scriptloader_t *loader = NULL;
  str_t          *wrap;
  
  if (obelix -> loaders) {
    loader = dict_get(obelix -> loaders, wrap = str_wrap(cookie));
    str_free(wrap);
  }
  return loader;
}

obelix_t * obelix_decommission_loader(obelix_t *obelix, scriptloader_t *loader) {
  str_t   *wrap;
  
  if (loader) {
    if (obelix -> loaders) {
      dict_remove(obelix -> loaders, wrap = str_wrap(loader -> cookie));
      str_free(wrap);
    }
    scriptloader_free(loader);
  }
  return obelix;
}

data_t * obelix_run(obelix_t *obelix, name_t *name, array_t *args, dict_t *kwargs) {
  scriptloader_t *loader;
  exception_t    *ex;
  data_t         *ret;
  
  loader = obelix_create_loader(obelix);
  if (loader) {
    if (script_debug) {
      debug("obelix_run %s(%s, %s)", 
            name_tostring(name),
            (args) ? array_tostring(args) : "[]",
            (kwargs) ? dict_tostring(kwargs) : "{}");
    }
    ret = scriptloader_run(loader, name, args, kwargs);
    if (script_debug) {
      debug("Exiting with exit code %s [%s]", data_tostring(ret), data_typename(ret));
    }
    if ((ex = data_as_exception(ret)) && (ex -> code == ErrorExit)) {
      ret = data_copy(ex -> throwable);
      exception_free(ex);
    }
    scriptloader_free(loader);
  } else {
    ret = (data_t *) exception_create(ErrorInternalError, 
                                      "Could not create script loader");
  }
  return ret;  
}

/* ------------------------------------------------------------------------ */

int main(int argc, char **argv) {
  obelix_t *obelix;

  if (!(obelix = obelix_initialize(argc, argv))) {
    fprintf(stderr, "Usage: obelix [options ...] <filename> [script arguments]\n");
    exit(1);
  }

  if (obelix -> server) {
    oblserver_start(obelix);
  } else {
    return _obelix_cmdline(obelix);
  }
}

