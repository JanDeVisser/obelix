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

#include <ipc.h>
#include <net.h>
#include <data.h>
#include <exception.h>
#include <mutex.h>
#include <set.h>

#include "obelix.h"

#define PS1   ">> "
#define PS2   " - "

static inline void _obelix_init(void);

static data_t * _obelix_new(obelix_t *, va_list);
static void     _obelix_free(obelix_t *);
static char *   _obelix_tostring(obelix_t *);
static data_t * _obelix_resolve(obelix_t *, char *);

static data_t * _obelix_get(data_t *, char *, array_t *, dict_t *);
static data_t * _obelix_run(data_t *, char *, array_t *, dict_t *);
static data_t * _obelix_mount(data_t *, char *, array_t *, dict_t *);
static data_t * _obelix_startserver(data_t *, char *, array_t *, dict_t *);

static void *   _obelix_startserver_thread(void *);
static void     _obelix_debug_settings(obelix_t *);
static data_t * _obelix_cmdline(obelix_t *);
static data_t * _obelix_interactive(obelix_t *);

static vtable_t _vtable_Obelix[] = {
  { .id = FunctionNew,          .fnc = (void_t) _obelix_new },
  { .id = FunctionFree,         .fnc = (void_t) _obelix_free },
  { .id = FunctionStaticString, .fnc = (void_t) _obelix_tostring },
  { .id = FunctionResolve,      .fnc = (void_t) _obelix_resolve },
  { .id = FunctionNone,         .fnc = NULL }
};

static methoddescr_t _methods_Obelix[] = {
  { .type = Any,    .name = "obelix",  .method = _obelix_get,         .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "run",     .method = _obelix_run,         .argtypes = { String, Any, NoType },    .minargs = 1, .varargs = 1 },
  { .type = -1,     .name = "mount",   .method = _obelix_mount,       .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "server",  .method = _obelix_startserver, .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,                .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
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
  obelix -> cookie = strrand(NULL, COOKIE_SZ - 1);
  return (data_t *) obelix;
}

char * _obelix_tostring(obelix_t *obelix) {
  return OBELIX_NAME " " OBELIX_VERSION_MAJOR " " OBELIX_VERSION_MINOR;
}

void _obelix_free(obelix_t *obelix) {
  if (obelix) {
    free(obelix -> cookie);
    array_free(obelix -> options);
    array_free(obelix -> script_args);
    name_free(obelix -> script);
  }
}

/* ------------------------------------------------------------------------ */

data_t * _obelix_register_server(obelix_t *obelix, server_t *server, servermessage_t *msg) {
  scriptloader_t  *loader;
  data_t          *ret = NULL;
  dictionary_t    *dict;
  char            *mountpoint;
  name_t          *mp_name;

  ret = obelix_get_loader(obelix, NULL);
  if ((loader = data_as_scriptloader(ret))) {
    server->data = str_to_data(loader->cookie);
    mountpoint = data_tostring(data_uncopy(datalist_get(msg->args, 0)));
    mp_name = name_parse(mountpoint);
    ret = scriptloader_import(loader, mp_name);
    name_free(mp_name);
    if (data_is_module(ret)) {
      server->mountpoint = ret; // TODO: Decouple mountpoint from server object
                                // Is mountpoint even needed??
      dict = dictionary_create(NULL);
      dictionary_set(dict, "cookie", data_copy(server->data));
      ret = (data_t *) dict;
    }
  }
  return ret;
}

data_t * _obelix_remote_call(obelix_t *obelix, server_t *server, servermessage_t *msg) {
  scriptloader_t *loader = NULL;
  data_t         *ret;
  char           *name;
  name_t         *n;
  module_t       *mod;
  data_t         *obj;

  ret = obelix_get_loader(obelix, data_tostring(server -> data));
  if ((loader = data_as_scriptloader(ret))) {
    data_thread_set_kernel((data_t *) loader);
    ret = NULL;
    mod = data_as_module(server->mountpoint);
    name = data_tostring(data_uncopy(datalist_get(msg -> args, 0)));
    n = name_parse(name);
    obj = data_resolve((data_t *) mod, n);
    if (!obj)  {
      ret = data_exception(ErrorName,
          "Name '%s' could not be resolved in mountpoint'%s'",
          name, mod_tostring(mod));
    }
    if (!ret && !data_is_callable(obj)) {
      ret = data_exception(ErrorNotCallable,
          "Object '%s' of type '%s' is not callable",
          data_tostring(obj), data_typename(obj));
    }
    if (!ret) {
      ret = servermessage_match_payload(msg, Arguments);
    }
    if (!ret) {
      ret = data_call(obj, data_as_arguments(msg -> payload));
    }
    name_free(n);
    data_thread_set_kernel(NULL);
  }
  return ret;
}

/* ------------------------------------------------------------------------ */

static data_t * _obelix_get(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) self;
  (void) name;
  (void) args;
  (void) kwargs;

  return (data_t *) _obelix;
}

static data_t * _obelix_run(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  name_t      *script;
  array_t     *script_args;
  data_t      *ret;
  data_t      *name_arg = data_array_get(args, 0);
  arguments_t *a = arguments_create(args, kwargs);

  (void) self;
  (void) name;

  script = (data_is_name(name_arg))
          ? name_copy(name_arg)
          : obelix_build_name(data_tostring(name_arg));
  script_args = array_slice(args, 1, -1);

  ret = obelix_run(_obelix, script, a);
  array_free(script_args);
  name_free(script);
  return ret;
}

static data_t * _obelix_mount(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t         *ret;
  uri_t          *uri;
  scriptloader_t *loader;

  (void) name;
  (void) kwargs;

  uri = uri_create(data_tostring(data_array_get(args, 1)));
  if (uri -> error) {
    ret = data_copy(uri -> error);
  } else {
    loader = data_as_scriptloader(data_thread_kernel());
    if (!loader) {
      return data_exception(ErrorInternalError, "No scriptloader associated with current thread");
    }
    ret = mountpoint_create(uri, loader -> cookie);
    if (data_is_mountpoint(ret)) {
      ret = data_copy(ret);
    }
  }
  uri_free(uri);
  return ret;
}

void * _obelix_startserver_thread(void *port) {
  return (void *) (intptr_t) server_start((data_t *) _obelix, (int) (intptr_t) port);
}

data_t * _obelix_startserver(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int       port = data_intval(data_array_get(args, 0));
  thread_t *thread;
  data_t   *ret = self;

  (void) name;
  (void) kwargs;

  thread = thread_new(
      "Obelix server thread",
      _obelix_startserver_thread, (void *) (intptr_t) port);
  if (thread -> _errno) {
    ret = data_exception(ErrorInternalError,
        "Error starting server thread: %d", thread -> _errno);
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

data_t * _obelix_interactive(obelix_t *obelix) {
  scriptloader_t *loader;
  char           *line = NULL;
  data_t         *dline;
  data_t         *ret;
  int             tty = isatty(fileno(stdin));
  char           *prompt;

  loader = obelix_create_loader(obelix);
  if (loader) {
    scriptloader_source_initfile(loader);
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
  int    opt;
  int    ix;
  char  *optarg_copy;
  char  *ptr;
  uri_t *uri = NULL;

  application_init("obelix", argc, argv);
  debug(obelix, "Initialize obelix kernel");
  _obelix_init();
  if (!_obelix) {
    return NULL;
  }
  _obelix -> argc = argc;
  _obelix -> argv = argv;
  while ((opt = getopt(argc, argv, "s:g:d:f:p:v:ltS:i:")) != -1) {
    switch (opt) {
      case 'd':
        _obelix -> debug = optarg;
        break;
      case 'f':
        _obelix -> logfile = optarg;
        break;
      case 'g':
        _obelix -> grammar = optarg;
        break;
      case 'i':
        _obelix -> init_file = optarg;
        break;
      case 'l':
        obelix_set_option(_obelix, ObelixOptionList, 1);
        break;
      case 'p':
        _obelix -> basepath = optarg;
        break;
      case 'S':
        _obelix -> server = -1;
        break;
      case 's':
        _obelix -> syspath = optarg;
        break;
      case 't':
        obelix_set_option(_obelix, ObelixOptionList, 1);
        obelix_set_option(_obelix, ObelixOptionTrace, 1);
        break;
      case 'v':
        _obelix -> log_level = optarg;
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

data_t * obelix_get_loader(obelix_t *obelix, char *cookie) {
  array_t        *path;
  scriptloader_t *loader = NULL;
  data_t         *ret = NULL;

  if (!obelix->loaders) {
    obelix->loaders = dictionary_create(NULL);
  }
  if (cookie) {
    ret = dictionary_get(obelix->loaders, cookie);
    if (!ret) {
      ret = data_exception(ErrorInternalError,
          "No loader with cookie %s registered", cookie);
    }
  }
  if (!ret) {
    path = (obelix->basepath) ? array_split(obelix->basepath, ":")
                              : str_array_create(0);
    array_push(path, getcwd(NULL, 0));
    loader = scriptloader_create(obelix->syspath, path, obelix->grammar);
    if (loader) {
      scriptloader_set_options(loader, obelix->options);
      dictionary_set(obelix->loaders, cookie, loader);
    }
    array_free(path);
    ret = (data_t *) loader;
  }
  return ret;
}

obelix_t * obelix_decommission_loader(obelix_t *obelix, char *cookie) {
  scriptloader_t *loader = NULL;

  if (obelix -> loaders) {
    loader = (scriptloader_t *) dictionary_pop(obelix -> loaders, cookie);
  }
  scriptloader_free(loader);
  return obelix;
}

data_t * obelix_run(obelix_t *obelix, name_t *name, arguments_t *args) {
  scriptloader_t *loader;
  exception_t    *ex;
  data_t         *ret;

  loader = obelix_create_loader(obelix);
  if (loader) {
    debug(obelix, "obelix_run %s(%s, %s)",
        name_tostring(name), arguments_tostring(args));
    ret = scriptloader_run(loader, name, args);
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
    server_start((data_t *) obelix, obelix -> server);
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
