/*
 * /obelix/src/lib/function.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <errno.h>
#include <stdio.h>

#include "libcore.h"
#include <data.h>
#include <exception.h>
#include <function.h>
#include <name.h>
#include <resolve.h>
#include <str.h>
#include <typedescr.h>

static inline void  _function_init(void);
static function_t * _function_new(function_t *, va_list);
static void         _function_free(function_t *);
static char *       _function_allocstring(function_t *);
static data_t *     _function_cast(function_t *, int);
static data_t *     _function_call(function_t *, array_t *, dict_t *);

static vtable_t _vtable_Function[] = {
  { .id = FunctionNew,         .fnc = (void_t) _function_new },
  { .id = FunctionCmp,         .fnc = (void_t) function_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _function_free },
  { .id = FunctionAllocString, .fnc = (void_t) _function_allocstring },
  { .id = FunctionParse,       .fnc = (void_t) function_parse },
  { .id = FunctionCast,        .fnc = (void_t) _function_cast },
  { .id = FunctionHash,        .fnc = (void_t) function_hash },
  { .id = FunctionCall,        .fnc = (void_t) _function_call },
  { .id = FunctionNone,        .fnc = NULL }
};

int function_debug = 0;
int Function = -1;

/* -- F U N C T I O N  S T A T I C  F U N C T I O N S --------------------- */

void _function_init(void) {
  if (Function < 1) {
    logging_register_category("function", &function_debug);
    typedescr_register(Function, function_t);
  }
  assert(Function);
}

function_t * _function_new(function_t *function, va_list args) {
  char *name = va_arg(args, char *);

  debug(function, "_function_new('%s')", name);
  function -> params = NULL;
  function -> type = 0;
  function -> name = name_split(name, ":");
  function ->  fnc = NULL;
  return function;
}

char * _function_allocstring(function_t *fnc) {
  char  *buf;
  str_t *params;

  params = (fnc -> params && array_size(fnc -> params))
    ? array_join(fnc -> params, ",")
    : NULL;
  asprintf(&buf, "%s(%s)",
           name_tostring_sep(fnc -> name, ":"),
           ((params) ? str_chars(params) : ""));
  str_free(params);
  return buf;
}

void _function_free(function_t *fnc) {
  if (fnc) {
    name_free(fnc -> name);
    array_free(fnc -> params);
  }
}

data_t * _function_cast(function_t *fnc, int totype) {
  data_t     *ret = NULL;

  if (totype == Bool) {
    ret = int_as_bool(fnc -> fnc != NULL);
  } else if (totype == Int) {
    ret = int_to_data((intptr_t) fnc -> fnc);
  }
  return ret;
}

data_t * _function_call(function_t *fnc, array_t *args, dict_t *kwargs) {
  return function_call(fnc, function_funcname(fnc), args, kwargs);
}


/* -- F U N C T I O N  P U B L I C  F U N C T I O N S --------------------- */

function_t * function_create(char *name, void_t fnc) {
  function_t *ret;

  debug(function, "function_create(%s)", name);
  ret = function_create_noresolve(name);
  if (fnc) {
    ret -> fnc = fnc;
  } else {
    debug(function, "resolving...");
    function_resolve(ret);
  }
  return ret;
}

function_t * function_create_noresolve(char *name) {
  _function_init();
  assert(name);
  return (function_t *) data_create(Function, name);
}

function_t * function_parse(char *str) {
  name_t     *name_params = name_split(str, "(");
  function_t *ret;
  char       *p;
  name_t     *params = NULL;

  _function_init();
  if (!name_size(name_params) || (name_size(name_params) > 2)) {
    return NULL;
  }
  if (name_size(name_params) == 2) {
    p = name_last(name_params);
    if (p[strlen(p) - 1] != ')') {
      ret = NULL;
    } else {
      p[strlen(p) - 1] = 0;
      params = name_split(p, ",");
    }
  } else {
    params = name_create(0);
  }
  if (params) {
    ret = function_create(name_first(name_params), NULL);
    ret -> params = name_as_array(params);
  }
  name_free(params);
  name_free(name_params);
  return ret;
}

int function_cmp(function_t *fnc1, function_t *fnc2) {
  return name_cmp(fnc1 -> name, fnc2 -> name);
}

function_t * function_resolve(function_t *fnc) {
  if (fnc -> fnc) {
    return fnc;
  }
  if (!name_size(fnc -> name) || name_size(fnc -> name) > 2) {
    return fnc;
  } else {
    debug(function, "Resolving %s", name_tostring_sep(fnc -> name, ":"));
    if (name_size(fnc -> name) == 2) {
      if (!resolve_library(name_first(fnc -> name))) {
        error("Error loading library '%s'", name_first(fnc -> name));
        return fnc;
      }
    }
    fnc -> fnc = (void_t) resolve_function(name_last(fnc -> name));
    if (!fnc -> fnc && function_debug) {
      error("Error resolving function '%s'", name_tostring(fnc -> name));
    }
  }
  return fnc;
}

unsigned int function_hash(function_t *fnc) {
  return hashblend(name_hash(fnc -> name), hashptr(fnc -> fnc));
}

data_t * function_call(function_t *fnc, char *name, array_t *args, dict_t *kwargs) {
  if (!fnc -> fnc) {
    function_resolve(fnc);
  }
  if (!fnc -> fnc) {
    debug(function, "Cannot execute %s(%s): function could not be resolved",
      function_tostring(fnc), array_tostring(args));
    return data_exception(ErrorFunctionUndefined,
      "Cannot execute %s(%s): function could not be resolved",
      function_tostring(fnc), array_tostring(args));
  } else {
    debug(function, "Executing %s(%s)", function_tostring(fnc), array_tostring(args));
    return ((native_t) fnc -> fnc)(name, args, kwargs);
  }
}

char * function_funcname(function_t *fnc) {
  return name_last(fnc -> name);
}

char * function_libname(function_t *fnc) {
  return (name_size(fnc -> name) > 1) ? name_last(fnc -> name) : NULL;
}

/* ------------------------------------------------------------------------ */
