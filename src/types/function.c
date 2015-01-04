/*
 * /obelix/src/data/data_int.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <error.h>
#include <resolve.h>

static data_t *      _fnc_new(data_t *, va_list);
static data_t *      _fnc_copy(data_t *, data_t *);
static int           _fnc_cmp(data_t *, data_t *);
static char *        _fnc_tostring(data_t *);
static data_t *      _fnc_parse(char *);
static unsigned int  _fnc_hash(data_t *);

static data_t *      _fnc_call(data_t *, char *, array_t *, dict_t *);

typedescr_t typedescr_fnc = {
  type:                  Function,
  typecode:              "U",
  typename:              "fnc",
  new:      (new_t)      _fnc_new,
  copy:     (copydata_t) _fnc_copy,
  cmp:      (cmp_t)      _fnc_cmp,
  free:     (free_t)     function_free,
  tostring: (tostring_t) _fnc_tostring,
  parse:    (parse_t)    _fnc_parse,
  cast:                  NULL,
  fallback:              NULL,
  hash:     (hash_t)     _fnc_hash
};

methoddescr_t methoddescr_fnc[] = {
  { type: Function, name: "call",  method: _fnc_call, argtypes: { Any, NoType, NoType },    minargs: 1, varargs: 1 },
  { type: NoType,   name: NULL,    method: NULL,      argtypes: { NoType, NoType, NoType }, minargs: 0, varargs: 0 },
};

/*
 * --------------------------------------------------------------------------
 * Function
 * --------------------------------------------------------------------------
 */

data_t * _fnc_new(data_t *target, va_list arg) {
  function_t *fnc;

  fnc = va_arg(arg, function_t *);
  target -> ptrval = function_copy(fnc);
  return target;
}

data_t * _fnc_copy(data_t *target, data_t *src) {
  target -> ptrval = function_copy(src -> ptrval);
  return target;
}

int _fnc_cmp(data_t *d1, data_t *d2) {
  function_t *fnc1;
  function_t *fnc2;

  fnc1 = d1 -> ptrval;
  fnc2 = d2 -> ptrval;
  return (int) ((long) fnc1 -> fnc) - ((long) fnc2 -> fnc);
}

char * _fnc_tostring(data_t *data) {
  function_t     *fnc;

  fnc = data -> ptrval;
  return fnc -> name;
}

data_t * _fnc_parse(char *str) {
  void_t      f = resolve_function(str);
  function_t *fnc;
  data_t     *ret = NULL;
  
  if (f) {
    fnc = function_create(str, (voidptr_t) f);
    ret = data_create(Function, fnc);
    function_free(fnc);
  }
  return ret;
}

unsigned int _fnc_hash(data_t *data) {
  function_t     *fnc;

  fnc = data -> ptrval;
  return hashptr(fnc -> fnc);
}


/* ----------------------------------------------------------------------- */

data_t * _fnc_call(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  /* FIXME is there a better way? */
  function_t *fnc = (function_t *) self -> ptrval;
  return fnc -> fnc(args);
}

