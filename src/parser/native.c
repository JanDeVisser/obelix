/*
 * /obelix/src/parser/logging.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <exception.h>
#include <native.h>
#include <script.h>
#include <resolve.h>

static void   _native_init(void) __attribute__((constructor(102)));
static void   _native_fnc_free(native_fnc_t *);

static vtable_t _vtable_native[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) native_fnc_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _native_fnc_free },
  { .id = FunctionToString, .fnc = (void_t) native_fnc_tostring },
  { .id = FunctionCall,     .fnc = (void_t) native_fnc_execute },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_native = {
  .type =      Native,
  .type_name = "native",
  .vtable =    _vtable_native
};

/* ------------------------------------------------------------------------ */

void _native_init(void) {
  typedescr_register(&_typedescr_native);
}

/* ------------------------------------------------------------------------ */

void _native_fnc_free(native_fnc_t *fnc) {
  if (fnc) {
    name_free(fnc -> name);
    array_free(fnc -> params);
    free(fnc);
  }
}

native_fnc_t * _native_fnc_resolve(native_fnc_t *fnc) {
  native_t      cfunc;
  name_t       *lib_func;
  
  if (fnc -> native_method) {
    return fnc;
  }
  lib_func = name_split(name_first(fnc -> name), ":");
  name_free(fnc -> name);
  fnc -> name = lib_func;
  if (name_size(lib_func) > 2 || name_size(lib_func) == 0) {
    return fnc;
  } else {
    if (name_size(lib_func) == 2) {
      if (!resolve_library(name_first(lib_func))) {
        error("Error loading library '%s': %s", name_first(lib_func), strerror(errno));
        return fnc;
      }
    }
    fnc -> native_method = (native_t) resolve_function(name_last(lib_func));
    if (!fnc -> native_method) {
      error("Error resolving function '%s': %s", name_tostring(lib_func), strerror(errno));
    }
  }
  return fnc;
}

/* --------------------------- -> --------------------------------------------- */

native_fnc_t * native_fnc_create(char *name, native_t c_func) {
  native_fnc_t *ret;
  name_t       *n;

  assert(name);
  if (script_debug) {
    debug("Creating native function '%s'", name);
  }
  ret = NEW(native_fnc_t);
  data_settype(&ret -> _d, Native);
  ret -> native_method = c_func;;
  ret -> params = NULL;
  ret -> name = name_create(1, name);
  ret -> async = 0;
  _native_fnc_resolve(ret);
  return ret;
}

data_t * native_fnc_execute(native_fnc_t *fnc, array_t *args, dict_t *kwargs) {
  if (fnc -> native_method) {
    return fnc -> native_method(name_last(fnc -> name), args, kwargs);  
  } else {
    return data_exception(ErrorInternalError,
                          "Call to unresolved native function '%s'",
                          native_fnc_tostring(fnc));
  }
}

int native_fnc_cmp(native_fnc_t *f1, native_fnc_t *f2) {
  return name_cmp(f1 -> name, f2 -> name);
}

char * native_fnc_tostring(native_fnc_t *fnc) {
  return name_tostring(fnc -> name);
}

