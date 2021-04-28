/*
 * /obelix/src/lib/application.c - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#include "libcore.h"
#include <application.h>

static void               _app_init(void);
static application_t *    _app_new(application_t *, va_list);
static data_t *           _app_resolve(application_t *, char *);
static data_t *           _app_set(application_t *, char *, data_t *);
static void               _app_free(application_t *);
static void *             _app_reduce_children(application_t *app, reduce_t reducer, void *ctx);

static void               _app_help(application_t *);
static void               _app_debug(application_t *, char *);
static cmdline_option_t * _app_find_longopt(application_t *, char *);
static cmdline_option_t * _app_find_shortopt(application_t *, char);
static int                _app_parse_option(application_t *, cmdline_option_t *, int);


static vtable_t _vtable_Application[] = {
  { .id = FunctionNew,         .fnc = (void_t) _app_new },
  { .id = FunctionFree,        .fnc = (void_t) _app_free },
  { .id = FunctionResolve,     .fnc = (void_t) _app_resolve },
  { .id = FunctionSet,         .fnc = (void_t) _app_set },
  { .id = FunctionReduce,      .fnc = (void_t) _app_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

int                   Application = -1;
int                   ErrorCommandLine = -1;
static application_t *_app = NULL;
static int            application_debug = 0;

/* ------------------------------------------------------------------------ */

application_t * _app_new(application_t *app, va_list args) {
  if (_app) {
    fprintf(stderr, "Trying to re-create singleton application object\n");
    abort();
  }
  _app = app;
  app -> executable = NULL;
  app -> descr = NULL;
  app -> argc = 0;
  app -> argv = str_array_create(0);
  app -> args = arguments_create(NULL, NULL);
  return app;
}

void _app_free(application_t *app) {
  if (app) {
    free(app -> executable);
  }
}

data_t * _app_resolve(application_t *app, char *name) {
  if (!strcmp(name, "args")) {
    return (data_t *) app -> args;
  } else if (!strcmp(name, "executable")) {
    return str_to_data(app -> executable);
  } else {
    return NULL;
  }
}

data_t * _app_set(application_t *app, char *name, data_t *value) {
  arguments_set_kwarg(app -> args, name, value);
  return (data_t *) app;
}

void * _app_reduce_children(application_t *app, reduce_t reducer, void *ctx) {
  return reducer(app->error, reducer(app->args, ctx));
}

_noreturn_ void _app_help(application_t *app) {
  int ix;

  if (app -> descr -> name) {
    fprintf(stderr, "%s", app->descr->name);
    if (app->descr->shortdescr) {
      fprintf(stderr, " - %s", app->descr->shortdescr);
    }
    fprintf(stderr, "\n\n");
  } else {
    fprintf(stderr, "%s\n\n", app->executable);
  }
  if (app->descr->description) {
    fprintf(stderr, "%s\n\n", app->descr->description);
  }
  if (app->descr->legal) {
    fprintf(stderr, "%s\n\n", app->descr->legal);
  }
  if (app->descr->options) {
    for (ix = 0; app->descr->options[ix].longopt; ix++) {
      fprintf(stderr, "\t--%s", app->descr->options[ix].longopt);
      if (app->descr->options[ix].shortopt) {
        fprintf(stderr, ", -%c", app->descr->options[ix].shortopt);
      }
      if (app->descr->options[ix].description) {
        fprintf(stderr, "\t%s", app->descr->options[ix].description);
      }
      fprintf(stderr, "\n");
    }
  }
  fprintf(stderr,
      "\t--debug, -d\tLog debug messages for the given comma-separated modules\n"
      "\t--loglevel, -v\tLog level (ERROR, WARN, INFO, DEBUG)\n"
      "\t--logfile\tLog file\n\n");
  exit(1);
}

void _app_debug(application_t *app, char *arg) {
  array_t *cats;
  int      ix;

  _debug("debug optarg: %s", arg);
  cats = array_split(arg, ",");
  for (ix = 0; ix < array_size(cats); ix++) {
    logging_enable(str_array_get(cats, ix));
  }
  array_free(cats);
}

cmdline_option_t * _app_find_longopt(application_t *app, char *opt) {
  cmdline_option_t *optdef;

  for (optdef = app -> descr -> options; optdef -> longopt; optdef++) {
    if (!strcmp(optdef -> longopt, opt)) {
      break;
    }
  }
  return (optdef -> longopt) ? optdef : NULL;
}

cmdline_option_t * _app_find_shortopt(application_t *app, char opt) {
  cmdline_option_t *optdef;

  for (optdef = app -> descr -> options; optdef -> longopt; optdef++) {
    if (optdef -> shortopt == opt) {
      break;
    }
  }
  return (optdef -> longopt) ? optdef : NULL;
}

int _app_parse_option(application_t *app, cmdline_option_t *opt, int ix) {
  char       *arg = str_array_get(app -> argv, ix);
  datalist_t *optargs;
  int         ret = ix;
  data_t     *err;

  debug(application, "parsing option '%s'", opt -> longopt);
  if ((ix == (app -> argc - 1)) ||                        /* Option is last arg                 */
      ((strlen(arg) > 2) && (arg[1] != '-')) ||           /* Arg is a sequence of short options */
      (*(str_array_get(app -> argv, ix + 1)) == '-') ||   /* Next arg is an option              */
      !(opt -> flags & CMDLINE_OPTION_FLAG_ALLOWS_ARG)) { /* Option doesn't allow args          */
    arguments_set_kwarg(app -> args, opt -> longopt, data_true());
    goto leave;
  }

  if ((opt -> flags & CMDLINE_OPTION_FLAG_REQUIRED_ARG) &&
      ((ix == (app -> argc - 1)) || (*(str_array_get(app -> argv, ix + 1)) == '-'))) {
    app -> error = data_exception(ErrorCommandLine,
        "Option '--%s' requires an argument", opt -> longopt);
    goto leave;
  }

  if (opt -> flags & CMDLINE_OPTION_FLAG_MANY_ARG) {
    optargs = datalist_create(NULL);
    for (ix++; (ix < app -> argc) && (*(str_array_get(app -> argv, ix)) != '-'); ix++) {
      debug(application, "pushing optarg %s to option '%s'", str_array_get(app -> argv, ix), opt -> longopt);
      datalist_push(optargs, str_to_data(str_array_get(app -> argv, ix)));
    }
    err = data_set_attribute((data_t *) app, opt -> longopt, (data_t *) optargs);
    if (data_is_exception(err)) {
      app -> error = err;
    }
    ret = ix - 1;
  } else {
    debug(application, "setting optarg %s for option '%s'", str_array_get(app -> argv, ix + 1), opt -> longopt);
    err = data_set_attribute((data_t *) app,
        opt -> longopt, str_to_data(str_array_get(app -> argv, ix + 1)));
    if (data_is_exception(err)) {
      app -> error = err;
    }
    ret = ix + 1;
  }

leave:
  debug(application, "app -> args[%s] = %s",
      opt -> longopt,
      data_tostring(arguments_get_kwarg(app -> args, opt -> longopt)));
  fprintf(stderr, "Return: %d\n", ret);
  return ret;
}

/* ------------------------------------------------------------------------ */

void application_init(void) {
  if (Application < 1) {
    core_init();
    logging_init();
    data_init();
    typedescr_init();
    exception_register(ErrorCommandLine);
    typedescr_register(Application, application_t);
    logging_register_module(application);
    assert(Application > 0);
  }
}

application_t * application_create(app_description_t *descr, int argc, char **argv) {
  application_t *app;

  application_init();
  app = (application_t *) data_create(Application);
  return application_parse_args(app, descr, argc, argv);
}

application_t * _application_parse_args(application_t *app, app_description_t *descr, int argc, char **argv) {
  int               ix;
  int               ixx;
  char             *arg;
  cmdline_option_t *opt;

  app -> descr = descr;
  app -> argc = argc;

  app -> argv = str_array_create(app -> argc);
  array_set_free(app -> argv, NULL);
  for (ix = 0; ix < app -> argc; ix++) {
    array_push(app -> argv, argv[ix]);
  }

  app -> executable = strdup(argv[0]);
  app -> error = NULL;

  logging_reset();
  for (ix = 1; (ix < app -> argc) && !app -> error; ix++) {
    arg = argv[ix];
    debug(application, "argv[%d] = %s", ix, arg);
    if (!strcmp(arg, "--help")) {
      _app_help(app);
    } else if (!strcmp(arg, "--debug") || !strcmp(arg, "-d")) {
      if (ix < (app -> argc - 1)) {
        _app_debug(app, argv[++ix]);
      } else {
        app -> error = data_exception(ErrorCommandLine,
            "Option '--debug' requires an argument");
      }
    } else if (!strcmp(arg, "--loglevel") || !strncmp(arg, "-v", 2)) {
      if (ix < (app -> argc - 1)) {
        logging_set_level(argv[++ix]);
      } else {
        app -> error = data_exception(ErrorCommandLine,
            "Option '--loglevel' requires an argument");
      }
    } else if (!strcmp(arg, "--logfile")) {
      if (ix < (app -> argc - 1) ) {
        logging_set_file(argv[++ix]);
      } else {
        app -> error = data_exception(ErrorCommandLine,
            "Option '--logfile' requires an argument");
      }
    } else if ((strlen(arg) > 1) && (arg[0] == '-')) {
      if ((strlen(arg) > 2) && (arg[1] == '-')) {
        opt = _app_find_longopt(app, arg + 2);
        if (!opt) {
          app -> error = data_exception(ErrorCommandLine,
              "Unrecognized option '%s'", arg);
        } else {
          ix = _app_parse_option(app, opt, ix);
        }
      } else if ((strlen(arg) == 2) && (arg[1] == '-')) {
        ix++;
        break;
      } else {
        for (ixx = 1; ixx < strlen(arg) && !app -> error; ixx++) {
          opt = _app_find_shortopt(app, arg[ixx]);
          if (!opt) {
            app -> error = data_exception(ErrorCommandLine,
                "Unrecognized option '-%c'", arg[ixx]);
          } else if ((strlen(arg) > 2) &&
                     (opt -> flags & CMDLINE_OPTION_FLAG_REQUIRED_ARG)) {
            app -> error = data_exception(ErrorCommandLine,
                "Short option '-%c' requires an argument", arg[ixx]);
          } else {
            ix = _app_parse_option(app, opt, ix);
          }
        }
      }
    } else {
      break;
    }
  }

  if (!app -> error) {
    for (; ix < app -> argc; ix++) {
      arguments_push(app -> args, str_to_data(argv[ix]));
    }
  }
  if (app -> error) {
    fprintf(stderr, "Error: %s\n", data_tostring(app -> error));
  }
  return app;
}

data_t * _application_get_option(application_t *app, char *option) {
  return arguments_get_kwarg(app -> args, option);
}

data_t * _application_get_arg(application_t *app, int ix) {
  return arguments_get_arg(app -> args, ix);
}

int _application_has_option(application_t *app, char *option) {
  return arguments_has_kwarg(app -> args, option);
}

int _application_args_size(application_t *app) {
  return arguments_args_size(app -> args);
}

void application_terminate(void) {
  application_free(_app);
}
