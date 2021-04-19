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

#include "libruntime.h"

static inline void _script_init(void);

static script_t *  _script_new(script_t *, va_list);
static void        _script_free(script_t *);
static char *      _script_tostring(script_t *);

/* -- data_t type description structures ---------------------------------- */

static vtable_t _vtable_Script[] = {
  { .id = FunctionNew,      .fnc = (void_t) _script_new },
  { .id = FunctionCmp,      .fnc = (void_t) script_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _script_free },
  { .id = FunctionToString, .fnc = (void_t) _script_tostring },
  { .id = FunctionHash,     .fnc = (void_t) script_hash },
  { .id = FunctionCall,     .fnc = (void_t) script_execute },
  { .id = FunctionNone,     .fnc = NULL }
};

int Script = -1;
int script_debug = 0;

/* ------------------------------------------------------------------------ */

void _script_init(void) {
  logging_register_module(script);
  typedescr_register(Script, script_t);
}

/* -- S C R I P T  S T A T I C  F U N C T I O N S  ------------------------ */

script_t * _script_new(script_t *script, va_list args) {
  data_t   *enclosing = va_arg(args, data_t *);
  char     *name = va_arg(args, char *);
  module_t *mod = NULL;
  script_t *up = NULL;
  char      anon[40];

  if (!name) {
    (size_t) snprintf(anon, 40, "__anon__%d__", hashptr(script));
    name = anon;
  }

  debug(script, "Creating script '%s'", name);
  if (data_is_mod(enclosing)) {
    mod = (module_t *) enclosing;
    script -> name = name_create(0);
  } else if (data_is_script(enclosing)) {
    up = (script_t *) enclosing;
    dictionary_set(up -> functions, name, script);
    mod = up -> mod;
    script -> name = name_deepcopy(up -> name);
    name_extend(script -> name, name);
  }
  assert(mod);
  script -> up = script_copy(up);
  script -> mod = mod_copy(mod);

  script -> functions = dictionary_create(NULL);
  script -> params = NULL;
  script -> type = STNone;

  script -> fullname = name_deepcopy(script -> mod -> name);
  name_append(script -> fullname, script -> name);
  script->_d.str = strdup(name_tostring(script->fullname));
  data_set_string_semantics(script, StrSemanticsStatic);
  script->ast = ast_Script_create(script_tostring(script));
  return script;
}

char * _script_tostring(script_t *script) {
  return name_tostring(script_fullname(script));
}

void _script_free(script_t *script) {
  if (script) {
    ast_Script_free(script->ast);
    array_free(script -> params);
    dictionary_free(script -> functions);
    script_free(script -> up);
    mod_free(script -> mod);
    name_free(script -> name);
    name_free(script -> fullname);
  }
}

/* -- S C R I P T  P U B L I C  F U N C T I O N S  ------------------------ */

script_t * script_create(data_t *enclosing, char *name) {
  _script_init();
  return (script_t *) data_create(Script, enclosing, name);
}

name_t * script_fullname(script_t *script) {
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
  return (script) ? name_hash(script_fullname(script)) : 0;
}

script_t * script_get_toplevel(script_t *script) {
  script_t *ret;

  for (ret = script; ret -> up; ret = ret -> up);
  return ret;
}

data_t * script_execute(script_t *script, arguments_t *args) {
  closure_t *closure;
  data_t    *retval;

  debug(script, "script_execute(%s)", script_tostring(script));
  closure = closure_create(script, NULL, NULL);
  retval = closure_execute(closure, args);
  closure_free(closure);
  debug(script, "  script_execute returns %s", data_tostring(retval));
  return retval;
}

data_t * script_create_object(script_t *script, arguments_t *args) {
  object_t       *retobj;
  data_t         *retval;
  closure_t      *closure;
  bound_method_t *bm;

  debug(script, "script_create_object(%s)", script_tostring(script));
  retobj = object_create((data_t *) script);
  if (!script -> up) {
    object_free(script -> mod -> obj);
    script -> mod -> obj = object_copy(retobj);
  }
  bm = data_as_bound_method(retobj -> constructor);
  closure = bound_method_get_closure(bm);
  retobj -> constructing = TRUE;
  retval = closure_execute(closure, args);
  retobj -> constructing = FALSE;
  if (!data_is_exception(retval)) {
    retobj -> retval = retval;
    retval = (data_t *) object_copy(retobj);
    if (!script -> up) {
      script -> mod -> closure = closure_copy(closure);
    }
  }
  debug(script, "  script_create_object returns %s", data_tostring(retval));
  closure_free(closure);
  return retval;
}

bound_method_t * script_bind(script_t *script, object_t *object) {
  return bound_method_create(script, object);
}
