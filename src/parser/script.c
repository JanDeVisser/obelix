/*
 * /obelix/src/script.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <boundmethod.h>
#include <closure.h>
#include <exception.h>
#include <logging.h>
#include <namespace.h>
#include <script.h>
#include <str.h>
#include <stacktrace.h>
#include <thread.h>
#include <wrapper.h>

static void       _script_init(void) __attribute__((constructor(102)));

static script_t * _script_set_instructions(script_t *, list_t *);
static void       _script_list_block(list_t *);

static void       _script_free(script_t *);
static char *     _script_tostring(script_t *);

/* -- data_t type description structures ---------------------------------- */

static vtable_t _vtable_script[] = {
  { .id = FunctionCmp,      .fnc = (void_t) script_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _script_free },
  { .id = FunctionToString, .fnc = (void_t) _script_tostring },
  { .id = FunctionCall,     .fnc = (void_t) script_execute },
  { .id = FunctionNone,     .fnc = NULL }
};

int Script = -1;
int script_debug = 0;

/* ------------------------------------------------------------------------ */

void _script_init(void) {
  logging_register_category("script", &script_debug);
  Script = typedescr_create_and_register(Script, "script", _vtable_script, NULL);
}

/* -- S C R I P T  S T A T I C  F U N C T I O N S  ------------------------ */

char * _script_tostring(script_t *script) {
  return name_tostring(script_fullname(script));
}

void _script_free(script_t *script) {
  if (script) {
    bytecode_free(script -> bytecode);
    array_free(script -> params);
    dict_free(script -> functions);
    script_free(script -> up);
    mod_free(script -> mod);
    name_free(script -> name);
    name_free(script -> fullname);
  }
}

/* -- S C R I P T  P U B L I C  F U N C T I O N S  ------------------------ */

script_t * script_create(module_t *mod, script_t *up, char *name) {
  script_t   *ret;
  char       *anon = NULL;

  if (!name) {
    asprintf(&anon, "__anon__%d__", hashptr(ret));
    name = anon;
  }
  if (script_debug) {
    debug("Creating script '%s'", name);
  }
  ret = data_new(Script, script_t);

  ret -> functions = strdata_dict_create();
  ret -> params = NULL;
  ret -> async = 0;

  if (up) {
    dict_put(up -> functions, strdup(name), data_create_script(ret));
    ret -> up = script_copy(up);
    ret -> mod = mod_copy(up -> mod);
    ret -> name = name_deepcopy(up -> name);
    name_extend(ret -> name, name);
  } else {
    assert(mod);
    ret -> mod = mod_copy(mod);
    ret -> up = NULL;
    ret -> name = name_create(0);
  }
  ret -> fullname = NULL;
  ret -> bytecode = bytecode_create(data_create(Script, ret));
  free(anon);
  return ret;
}

name_t * script_fullname(script_t *script) {
  if (!script -> fullname) {
    script -> fullname = name_deepcopy(script -> mod -> name);
    name_append(script -> fullname, script -> name);
  }
  return script -> fullname;
}

int script_cmp(script_t *s1, script_t *s2) {
  if (s1 && !s2) {
    return 1;
  } else if (!s1 && s2) {
    return -1;
  } else if (!s1 && !s2) {
    return 0;
  } else {
    return name_cmp(script_fullname(s1), script_fullname(s2));
  }
}

unsigned int script_hash(script_t *script) {
  return (script) ? name_hash(script_fullname(script)) : 0L;
}

script_t * script_get_toplevel(script_t *script) {
  script_t *ret;

  for (ret = script; ret -> up; ret = ret -> up);
  return ret;
}

data_t * script_execute(script_t *script, array_t *args, dict_t *kwargs) {
  closure_t *closure;
  data_t    *retval;

  if (script_debug) {
    debug("script_execute(%s)", script_tostring(script));
  }
  closure = script_create_closure(script, NULL, NULL);
  retval = closure_execute(closure, args, kwargs);
  closure_free(closure);
  if (script_debug) {
    debug("  script_execute returns %s", data_tostring(retval));
  }
  return retval;
}

data_t * script_create_object(script_t *script, array_t *params, dict_t *kwparams) {
  object_t *retobj;
  data_t   *retval;
  data_t   *dscript;
  data_t   *bm;
  data_t   *self;

  if (script_debug) {
    debug("script_create_object(%s)", script_tostring(script));
  }
  retobj = object_create(dscript = data_create(Script, script));
  if (!script -> up) {
    script -> mod -> obj = object_copy(retobj);
  }
  retobj -> constructing = TRUE;
  retval = bound_method_execute(data_as_bound_method(retobj -> constructor),
                                params, kwparams);
  retobj -> constructing = FALSE;
  if (!data_is_exception(retval)) {
    retobj -> retval = retval;
    retval = data_create(Object, retobj);
  }
  if (script_debug) {
    debug("  script_create_object returns %s", data_tostring(retval));
  }
  data_free(dscript);
  return retval;
}

bound_method_t * script_bind(script_t *script, object_t *object) {
  return bound_method_create(script, object);
}

closure_t * script_create_closure(script_t *script, closure_t *up, data_t *self) {
  return closure_create(script, up, self);
}
