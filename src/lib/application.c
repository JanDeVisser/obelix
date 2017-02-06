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

#include <application.h>
#include <exception.h>

static void     _app_init(void);
static app_t *  _app_new(app_t *, va_list);
// static data_t * _app_resolve(app_t *, char *);
static void     _app_free(app_t *);

static vtable_t _vtable_URI[] = {
  { .id = FunctionNew,         .fnc = (void_t) _app_new },
  { .id = FunctionFree,        .fnc = (void_t) _app_free },
  // { .id = FunctionResolve,     .fnc = (void_t) _app_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

int                   Application = -1;
int                   ErrorCommandLine = -1;
static application_t *_app = NULL;

/* ------------------------------------------------------------------------ */

void _app_init(void) {
  if (Application < 1) {
    type_str = &_type_str;
    type_int = &_type_int;
    logging_init();
    data_init();
    typedescr_init();
    exception_register(ErrorCommandLine);
    typedescr_register(Application, application_t);
  }
}

application_t * _app_new(application_t *app, va_list args) {
  int                ix;
  int                ixx;
  cmdline_option_t  *opt;
  char             **argv;

  if (_app) {
    fprintf(stderr, "Trying to re-create singleton application object\n");
    abort();
  }
  _app = app;
  app -> descr = va_arg(args, app_description_t *);
  app -> argc = va_arg(args, int);
  argv = va_arg(args, char **);

  app -> argv = str_array_create(app -> argc);
  array_set_free(app -> argv, NULL);
  for (ix = 0; ix < argc; ix++) {
    array_push(app -> argv, argv[ix]);
  }

  app -> executable = strdup(argv[0]);
  app -> options = dictionary_create(NULL);
  app -> positional_args = datalist_create(NULL);
  app -> error = NULL;

  for (ix = 1; (ix < app -> argc) && !app -> error; ix++) {
    arg = argv[ix];
    if ((strlen(arg) > 1) && (arg[0] == '-')) {
      if ((strlen(arg) > 2) && (arg[0] == '-') && (arg[1] == '-')) {
        opt = _app_find_longopt(app, arg + 2);
        if (!opt) {
          app -> error = data_exception(ErrorCommandLine,
              "Unrecognized option '%s'", arg);
        } else {
          ix = _app_parse_option(app, opt, ix);
        }
      } else {
        for (ixx = 1; ixx < strlen(arg) && !app -> error; ixx++) {
          opt = _app_find_shortopt(app, arg[1]);
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
    } else if (!strcmp("--", arg)) {
      ix++;
      break;
    } else {
      break;
    }
  }

  if (app -> error) {
    for (; ix < app -> argc; ix++) {
      datalist_push(app -> positional_args, str_to_data(argv[ix]));
    }
  }
  return app;
}

void _app_free(application_t *app) {
  if (app) {
    free(app -> executable);
    dictionary_free(app -> options);
    datalist_free(app -> positional_args);
    data_free(app -> error);
  }
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
  if ((ix == (app -> argc - 1)) ||
      (*(str_array_get(app -> argv, ix + 1)) == '-') ||
      !(opt -> flags & CMDLINE_OPTION_FLAG_ALLOWS_ARG)) {
    dictionary_set(app -> options, opt -> longopt, data_true());
    return ix;
  }
  if ((opt -> flags & CMDLINE_OPTION_FLAG_REQUIRED_ARG) &&
      ((ix == (app -> argc - 1)) || (*(str_array_get(app -> argv, ix + 1)) == '-'))) {
    app -> error = data_exception(ErrorCommandLine,
        "Option '-%s' requires an argument", opt -> longopt);
    return ix;
  }
}

/* ------------------------------------------------------------------------ */

 application_t * application_create(app_description_t *descr, int argc, char **argv) {
   _app_init();
   return (application_t *) data_create(Application, descr, argc, argv);
 }

 void application_terminate(void) {
   application_free(_app);
 }
