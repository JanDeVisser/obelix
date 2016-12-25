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

#include <readline/readline.h>
#include <readline/history.h>

#include <net.h>
#include <data.h>
#include <exception.h>
#include <loader.h>
#include <mutex.h>
#include <set.h>

#include "obelix.h"

#define PS1   ">> "
#define PS2   " - "

static inline void _obelix_init(void);

static data_t * _obelix_new(obelix_t *, va_list);
static void     _obelix_free(obelix_t *);
static char *   _obelix_tostring(obelix_t *);

static data_t * _obelix_get(data_t *, char *, array_t *, dict_t *);
static data_t * _obelix_run(data_t *, char *, array_t *, dict_t *);
static data_t * _obelix_mount(data_t *, char *, array_t *, dict_t *);
static data_t * _obelix_unmount(data_t *, char *, array_t *, dict_t *);

static void     _obelix_debug_settings(obelix_t *);
static data_t * _obelix_cmdline(obelix_t *);
static data_t * _obelix_run_local(obelix_t *, name_t *, array_t *, dict_t *);
static data_t * _obelix_dispatch(obelix_t *, name_t *, array_t *, dict_t *);
static data_t * _obelix_interactive(obelix_t *);

static vtable_t _vtable_Obelix[] = {
  { .id = FunctionNew,          .fnc = (void_t) _obelix_new },
  { .id = FunctionFree,         .fnc = (void_t) _obelix_free },
  { .id = FunctionStaticString, .fnc = (void_t) _obelix_tostring },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methods_Obelix[] = {
  { .type = Any,    .name = "obelix",  .method = _obelix_get,     .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "run",     .method = _obelix_run,     .argtypes = { String, Any, NoType },    .minargs = 1, .varargs = 1 },
  { .type = -1,     .name = "mount",   .method = _obelix_mount,   .argtypes = { String, String, NoType }, .minargs = 2, .varargs = 0 },
  { .type = -1,     .name = "unmount", .method = _obelix_unmount, .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,            .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

       int       Obelix = -1;
       int       ErrorProtocol = -1;
static obelix_t *_obelix;

/* ------------------------------------------------------------------------ */

void _obelix_init(void) {
  if (Obelix < 0) {
    logging_register_category("obelix", &obelix_debug);
    typedescr_register_with_methods(Obelix, obelix_t);
    exception_register(ErrorProtocol);

    debug(obelix, "Creating obelix kernel");
    _obelix = (obelix_t *) data_create(Obelix);
  }
}

/* ------------------------------------------------------------------------ */

data_t * _obelix_new(obelix_t *obelix, va_list args) {
  int ix;

  if (_obelix) {
    return data_exception(ErrorInternalError, "The Obelix kernel is a singleton");
  }
  obelix -> logfile = NULL;
  obelix -> options = data_array_create((int) ObelixOptionLAST);
  for (ix = 0; ix < (int) ObelixOptionLAST; ix++) {
    obelix_set_option(obelix, ix, 0L);
  }
  obelix -> script = NULL;
  obelix -> script_args = NULL;
  obelix -> mountpoints = hierarchy_create();
  return (data_t *) obelix;
}

char * _obelix_tostring(obelix_t *obelix) {
  return "Obelix Kernel";
}

void _obelix_free(obelix_t *obelix) {
  if (obelix) {
    array_free(obelix -> options);
    array_free(obelix -> script_args);
    name_free(obelix -> script);
    hierarchy_free(obelix -> mountpoints);
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

static data_t * _obelix_mount(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t  *ret;
  uri_t   *uri;

  (void) name;
  (void) kwargs;

  uri = uri_create(data_tostring(data_array_get(args, 1)));
  if (uri -> error) {
    ret = data_copy(uri -> error);
  } else {
    ret = obelix_mount((obelix_t *) self,
            data_tostring(data_array_get(args, 0)), uri);
    if (data_is_clientpool(ret)) {
      ret = data_copy(ret);
    }
  }
  uri_free(uri);
  return ret;
}

static data_t * _obelix_unmount(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t  *ret;

  (void) name;
  (void) kwargs;

  ret = obelix_unmount(
          (obelix_t *) self,
          data_tostring(data_array_get(args, 0)));
  if (!ret) {
    ret = self;
  }
  return ret;
}

/* ------------------------------------------------------------------------ */

void _obelix_debug_settings(obelix_t *obelix) {
  array_t *cats;
  int      ix;

  logging_reset();
  logging_set_level(obelix -> log_level);
  if (obelix -> debug) {
    debug(obelix, "debug optarg: %s", obelix -> debug);
    cats = array_split(obelix -> debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
  if (obelix -> logfile) {
    logging_set_file(obelix -> logfile);
  }
}

data_t * _obelix_cmdline(obelix_t *obelix) {
  return obelix_run(obelix, obelix -> script, obelix -> script_args, NULL);
}

/* ------------------------------------------------------------------------ */

name_t * obelix_build_name(char *scriptname) {
  char   *buf;
  char   *ptr;
  name_t *name;

  buf = strdup(scriptname);
  while ((ptr = strchr(buf, '/'))) {
    *ptr = '.';
  }
  if ((ptr = strrchr(buf, '.'))) {
    if (!strcmp(ptr, ".obl")) {
      *ptr = 0;
    }
  }
  name = name_split(buf, ".");
  free(buf);
  return name;
}

data_t * _obelix_dispatch(obelix_t *obelix, name_t *name, array_t *args, dict_t *kwargs) {
  data_t       *ret = NULL;
  int           match_lost;
  hierarchy_t  *mountpoint = hierarchy_match(obelix -> mountpoints, name, &match_lost);
  clientpool_t *pool;
  name_t       *remote;
  int           ix;

  if (match_lost > 0) {
    pool = (clientpool_t *) mountpoint -> data;
    remote = name_deepcopy(pool -> server -> path);
    for (ix = match_lost; ix < name_size(name); ix++) {
      name_extend(remote, name_get(name, ix));
    }
    ret = clientpool_run(pool, remote, args, kwargs);
    name_free(remote);
  }
  return ret;
}

data_t * _obelix_run_local(obelix_t *obelix, name_t *name, array_t *args, dict_t *kwargs) {
  scriptloader_t *loader;
  exception_t    *ex;
  data_t         *ret;

  loader = obelix_create_loader(obelix);
  if (loader) {
    debug(obelix, "_obelix_run_local %s(%s, %s)",
          name_tostring(name),
          (args) ? array_tostring(args) : "[]",
          (kwargs) ? dict_tostring(kwargs) : "{}");
    ret = scriptloader_run(loader, name, args, kwargs);
    debug(obelix, "Exiting with exit code %s [%s]", data_tostring(ret), data_typename(ret));
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

data_t * _obelix_interactive(obelix_t *obelix) {
  scriptloader_t *loader;
  char           *line = NULL;
  data_t         *dline;
  data_t         *ret;
  int             tty = isatty(fileno(stdin));
  char           *prompt;

  loader = obelix_create_loader(obelix);
  if (loader) {
    if (tty) {
      fprintf(stdout, "Welcome to " OBELIX_NAME " " OBELIX_VERSION "\n");
    }
    for (prompt = (tty) ? PS1 : "", line = readline(prompt); line; line = readline(prompt)) {
      strrtrim(line);
      if (*line) {
        add_history(line);
        debug(obelix, "Evaluating '%s'", line);
        ret = scriptloader_eval(loader, dline = (data_t *) str_copy_chars(line));
        if (!ret && isatty(fileno(stdin))) {
          prompt = PS2;
        } else {
          if (!data_isnull(ret)) {
            puts(data_tostring(ret));
          }
          prompt = (tty) ? PS1 : "";
        }
        data_free(ret);
        data_free(dline);
      }
      free(line);
      line = NULL;
    }
    fprintf(stdout, "\n");
    scriptloader_free(loader);
    ret = NULL;
  } else {
    ret = (data_t *) exception_create(ErrorInternalError,
                                      "Could not create script loader");
  }
  return ret;
}

/* -- O B E L I X  P U B L I C  F U N C T I O N S ------------------------- */

obelix_t * obelix_initialize(int argc, char **argv) {
  int opt;
  int ix;

  application_init("obelix", argc, argv);
  debug(obelix, "Initialize obelix kernel");
  _obelix_init();
  if (!_obelix) {
    return NULL;
  }
  _obelix -> argc = argc;
  _obelix -> argv = argv;
  while ((opt = getopt(argc, argv, "s:g:d:f:p:v:ltS")) != -1) {
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
      case 'f':
        _obelix -> logfile = optarg;
        break;
      case 'p':
        _obelix -> basepath = optarg;
        break;
      case 'v':
        _obelix -> log_level = optarg;
        break;
      case 'l':
        obelix_set_option(_obelix, ObelixOptionList, 1);
        break;
      case 't':
        obelix_set_option(_obelix, ObelixOptionList, 1);
        obelix_set_option(_obelix, ObelixOptionTrace, 1);
        break;
      case 'S':
        _obelix -> server = OBELIX_DEFAULT_PORT;
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
    }
  }
  _obelix_debug_settings(_obelix);
  return _obelix;
}

obelix_t * obelix_set_option(obelix_t *obelix, obelix_option_t option, long value) {
  data_t *opt = int_to_data(value);

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
    if (!obelix -> loaders) {
      obelix -> loaders = datadata_dict_create();
    }
    dict_put(obelix -> loaders,
	     str_wrap(loader -> cookie),
	     scriptloader_copy(loader));
  }
  array_free(path);
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
  data_t *ret;

  ret = _obelix_dispatch(obelix, name, args, kwargs);
  if (!ret) {
    ret = _obelix_run_local(obelix, name, args, kwargs);
  }
  return ret;
}

data_t * obelix_mount(obelix_t *obelix, char *prefix, uri_t *uri) {
  data_t      *pool;
  name_t      *n = name_parse(prefix);
  hierarchy_t *leaf = hierarchy_find(obelix -> mountpoints, n);

  if (leaf) {
    return data_exception(ErrorParameterValue,
      "Prefix '%s' is already mounted to URI '%s'", prefix,
      data_tostring(leaf -> data));
  }
  pool = data_create(ClientPool, obelix, prefix, uri);
  hierarchy_insert(obelix -> mountpoints, n, pool);
  return pool;
}

data_t * obelix_unmount(obelix_t *obelix, char *prefix) {
  name_t      *n = name_parse(prefix);
  hierarchy_t *leaf = hierarchy_find(obelix -> mountpoints, n);

  if (leaf) {
    return data_exception(ErrorParameterValue,
      "Prefix '%s' is not mounted", prefix);
  }
  hierarchy_remove(obelix -> mountpoints, n);
  return NULL;
}

/* ------------------------------------------------------------------------ */

int main(int argc, char **argv) {
  obelix_t    *obelix;
  int          ret = 0;
  data_t      *data = NULL;
  exception_t *ex;

  if (!(obelix = obelix_initialize(argc, argv))) {
    fprintf(stderr, "Usage: obelix [options ...] [<script filename> [script arguments]]\n");
    exit(1);
  }

  if (obelix -> server) {
    oblserver_start(obelix);
  } else if (obelix -> script) {
    data = _obelix_cmdline(obelix);
  } else {
    data = _obelix_interactive(obelix);
  }
  if (data) {
    if ((ex = data_as_exception(data))) {
      fprintf(stderr, "Error: %s\n", ex -> msg);
      ret = 0 - (int) ex -> code;
    } else {
      ret = data_intval(data);
    }
  }
  return ret;
}
