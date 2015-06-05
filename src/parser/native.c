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

#include <script.h>
#include <native.h>

static void             _native_init(void) __attribute__((constructor));
static data_t *         _data_new_native(data_t *, va_list);
static data_t *         _data_copy_native(data_t *, data_t *);
static int              _data_cmp_native(data_t *, data_t *);
static char *           _data_tostring_native(data_t *);
static data_t *         _data_call_native(data_t *, array_t *, dict_t *);


static vtable_t _vtable_native[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_native },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_native },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_native },
  { .id = FunctionFree,     .fnc = (void_t) native_fnc_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_native },
  { .id = FunctionCall,     .fnc = (void_t) _data_call_native },
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

data_t * _data_new_native(data_t *ret, va_list arg) {
  native_fnc_t *fnc;

  fnc = va_arg(arg, native_fnc_t *);
  ret -> ptrval = fnc;
  fnc -> refs++;
  return ret;
}

data_t * _data_copy_native(data_t *target, data_t *src) {
  target -> ptrval = src -> ptrval;
  ((native_fnc_t *) target -> ptrval) -> refs++;
  return target;
}

int _data_cmp_native(data_t *d1, data_t *d2) {
  native_fnc_t *f1;
  native_fnc_t *f2;

  f1 = d1 -> ptrval;
  f2 = d2 -> ptrval;
  return native_fnc_cmp(f1, f2);
}

char * _data_tostring_native(data_t *d) {
  return native_fnc_tostring((native_fnc_t *) d -> ptrval);
}

data_t * _data_call_native(data_t *data, array_t *params, dict_t *kwargs) {
  native_fnc_t *fnc = (native_fnc_t *) data -> ptrval;
  
  return native_fnc_execute(fnc, params, kwargs);
}

/* ------------------------------------------------------------------------ */

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
      if (parser_debug) {
        debug("Library: %s", name_first(lib_func));
      }
      if (!resolve_library(name_first(lib_func))) {
        error("Error loading library '%s': %s", name_first(lib_func), strerror(errno));
        return fnc;
      }
    }
    if (parser_debug) {
      debug("C Function: %s", name_last(lib_func));
    }
    fnc -> native_method = (native_t) resolve_function(name_last(lib_func));
  }
  return fnc;
}
  

/* ------------------------------------------------------------------------ */

native_fnc_t * native_fnc_create(script_t *script, char *name, native_t c_func) {
  native_fnc_t *ret;
  
  assert(name);
  if (script_debug) {
    debug("Creating native function '%s'", name);
  }
  ret = NEW(native_fnc_t);
  ret -> native_method = c_func;;
  ret -> params = NULL;
  ret -> name = name_create(1, name);
  ret -> async = 0;
  ret -> refs = 1;
  if (script) {
    name_append(ret -> name, script -> name);
    dict_put(script -> functions, strdup(name), data_create(Native, ret));
    ret -> script = script_copy(script);
  }
  _native_fnc_resolve(ret);
  return ret;
}

native_fnc_t * native_fnc_copy(native_fnc_t *src) {
  src -> refs++;
  return src;
}

void native_fnc_free(native_fnc_t *fnc) {
  if (fnc) {
    fnc -> refs--;
    if (!fnc -> refs) {
      script_free(fnc -> script);
      name_free(fnc -> name);
      array_free(fnc -> params);
      free(fnc);
    }
  }
}

data_t * native_fnc_execute(native_fnc_t *fnc, array_t *args, dict_t *kwargs) {
  if (fnc -> native_method) {
    return fnc -> native_method(name_last(fnc -> name), args, kwargs);  
  } else {
    return data_exception();
  }
}

char * native_fnc_tostring(native_fnc_t *fnc) {
  return name_tostring(fnc -> name);
}

int native_fnc_cmp(native_fnc_t *f1, native_fnc_t *f2) {
  return name_cmp(f1 -> name, f2 -> name);
}
