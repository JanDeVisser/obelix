/*
 * /obelix/src/instruction.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <stdlib.h>
#include <stdio.h>

#include <array.h>
#include <instruction.h>
#include <script.h>

typedef char *   (*instr_fnc_t)(instruction_t *, closure_t *);
typedef data_t * (*native_t)(closure_t *, char *, array_t *);

typedef struct _instruction_type_descr {
  instruction_type_t  type;
  instr_fnc_t         function;
  char               *name;
  free_t              free;
  tostring_t          tostring;
} instruction_type_descr_t;

static instruction_t *  _instruction_create(instruction_type_t);
static void             _instruction_free_name(instruction_t *);
static void             _instruction_free_value(instruction_t *);
static void             _instruction_free_name_value(instruction_t *);
static char *           _instruction_tostring_name(instruction_t *);
static char *           _instruction_tostring_value(instruction_t *);
static data_t *         _instruction_execute_script(closure_t *, char *, array_t *);
static char *           _instruction_execute_assign(instruction_t *, closure_t *);
static char *           _instruction_execute_pushvar(instruction_t *, closure_t *);
static char *           _instruction_execute_pushval(instruction_t *, closure_t *);
static char *           _instruction_execute_function(instruction_t *, closure_t *);
static char *           _instruction_execute_test(instruction_t *, closure_t *);
static char *           _instruction_execute_jump(instruction_t *, closure_t *);
static char *           _instruction_execute_pop(instruction_t *, closure_t *);
static char *           _instruction_execute_nop(instruction_t *, closure_t *);

static instruction_type_descr_t instruction_descr_map[] = {
    { type: ITAssign,   function: _instruction_execute_assign,
      name: "Assign",   free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITPushVar,  function: _instruction_execute_pushvar,
      name: "PushVar",  free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITPushVal,  function: _instruction_execute_pushval,
      name: "PushVal",  free: (free_t) _instruction_free_value,
      tostring: (tostring_t) _instruction_tostring_value },
    { type: ITFunction, function: _instruction_execute_function,
      name: "Function", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITTest, function: _instruction_execute_test,
      name: "Test", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITPop, function: _instruction_execute_pop,
      name: "Pop", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITJump, function: _instruction_execute_jump,
      name: "Jump", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
    { type: ITNop, function: _instruction_execute_nop,
      name: "Nop", free: (free_t) _instruction_free_name,
      tostring: (tostring_t) _instruction_tostring_name },
};

/*
 * instruction_t static functions
 */

instruction_t * _instruction_create(instruction_type_t type) {
  instruction_t *ret;

  ret = NEW(instruction_t);
  ret -> type = type;
  return ret;
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

void _instruction_free_name(instruction_t *instruction) {
  free(instruction -> name);
}

void _instruction_free_value(instruction_t *instruction) {
  data_free(instruction -> value);
}

void _instruction_free_name_value(instruction_t *instruction) {
  free(instruction -> name);
  data_free(instruction -> value);
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

char * _instruction_tostring_name(instruction_t *instruction) {
  return instruction -> name;
}

char * _instruction_tostring_value(instruction_t *instruction) {
  return data_tostring(instruction -> value);
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_script(closure_t *closure, char *name, array_t *params) {
  data_t    *value;

  value = closure_execute(closure);
  value = value ? value : data_create_pointer(NULL);
  if (script_debug) {
    debug(" -- return: '%s'", data_tostring(value));
  }
  return value;
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

char * _instruction_execute_assign(instruction_t *instr, closure_t *closure) {
  char          *varname;
  data_t        *value;

  value = closure_pop(closure);
  assert(value);
  assert(instr -> name);
  if (script_debug) {
    debug(" -- value '%s'", data_tostring(value));
  }

  closure_set(closure, instr -> name, value);
  data_free(value);
  return NULL;
}

char * _instruction_execute_pushvar(instruction_t *instr, closure_t *closure) {
  data_t        *value;

  assert(instr -> name);
  value = closure_resolve(closure, instr -> name);
  assert(value);
  if (script_debug) {
    debug(" -- value '%s'", data_tostring(value));
  }
  closure_push(closure, data_copy(value));
  return NULL;
}

char * _instruction_execute_pushval(instruction_t *instr, closure_t *closure) {
  data_t        *value;

  assert(instr -> value);
  closure_push(closure, data_copy(instr -> value));
  return NULL;
}

char * _instruction_execute_function(instruction_t *instr, closure_t *closure) {
  data_t         *value;
  array_t        *params;
  int             ix;
  str_t          *debugstr;
  native_t        fnc;
  closure_t      *func_closure;
  data_t         *func_data;
  char           *func_name;

  if (script_debug) {
    debug(" -- #parameters: %d", instr -> num);
  }

  params = array_create(instr -> num);
  array_set_free(params, (free_t) data_free);
  array_set_tostring(params, (tostring_t) data_tostring);
  for (ix = 0; ix < instr -> num; ix++) {
    value = closure_pop(closure);
    assert(value);
    array_set(params, instr -> num - ix - 1, value);
  }
  if (script_debug) {
    debugstr = array_tostr(params);
    debug(" -- parameters: %s", str_chars(debugstr));
    str_free(debugstr);
  }

  func_data = closure_resolve(closure, instr -> name);
  // FIXME: Error handling
  assert(func_data);

  if (data_type(func_data) == Script) {
    func_closure = script_create_closure(
        (script_t *) func_data -> ptrval, params);
    func_name = script_get_fullname((script_t *) func_data -> ptrval);
    fnc = (native_t) _instruction_execute_script;
  } else if (data_type(func_data) == Function) {
    func_closure = closure;
    func_name = instr -> name;
    fnc = (native_t) (((function_t *) func_data -> ptrval) -> fnc);
  } else {
    error("_instruction_execute_function got a non-callable");
    assert(0);
  }

  value = fnc(func_closure, instr -> name, params);
  value = value ? value : data_null();
  if (script_debug) {
    debug(" -- return: '%s'", data_tostring(value));
  }
  closure_push(closure, value);
  array_free(params);
  return NULL;
}

char * _instruction_execute_test(instruction_t *instr, closure_t *closure) {
  char          *ret;
  data_t        *value;

  value = closure_pop(closure);
  assert(value);
  assert(instr -> name);

  /* FIXME - Convert other objects to boolean */
  ret = (!value -> intval) ? instr -> name : NULL;
  data_free(value);
  return ret;
}

char * _instruction_execute_jump(instruction_t *instr, closure_t *closure) {
  assert(instr -> name);
  return instr -> name;
}

char * _instruction_execute_pop(instruction_t *instr, closure_t *closure) {
  char          *ret;
  data_t        *value;

  value = closure_pop(closure);
  data_free(value);
  return NULL;
}

char * _instruction_execute_nop(instruction_t *instr, closure_t *closure) {
  return NULL;
}

/*
 * instruction_t public functions
 */

instruction_t * instruction_create_assign(char *varname) {
  instruction_t *ret;

  ret = _instruction_create(ITAssign);
  ret -> name = strdup(varname);
  return ret;
}

instruction_t * instruction_create_pushvar(char *varname) {
  instruction_t *ret;

  ret = _instruction_create(ITPushVar);
  ret -> name = strdup(varname);
  return ret;
}

instruction_t * instruction_create_pushval(data_t *value) {
  instruction_t *ret;

  ret = _instruction_create(ITPushVal);
  ret -> value = value;
  return ret;
}

instruction_t * instruction_create_function(char *name, int num_params) {
  instruction_t *ret;

  ret = _instruction_create(ITFunction);
  ret -> name = strdup(name);
  ret -> num = num_params;
  return ret;
}

instruction_t * instruction_create_test(char *label) {
  instruction_t *ret;

  ret = _instruction_create(ITTest);
  ret -> name = strdup(label);
  return ret;
}

instruction_t * instruction_create_jump(char *label) {
  instruction_t *ret;

  ret = _instruction_create(ITJump);
  ret -> name = strdup(label);
  return ret;
}


instruction_t * instruction_create_pop(void) {
  instruction_t *ret;

  ret = _instruction_create(ITPop);
  ret -> name = strdup("Discard top-of-stack");
  return ret;
}

instruction_t * instruction_create_nop(void) {
  instruction_t *ret;

  ret = _instruction_create(ITNop);
  ret -> name = strdup("Nothing");
  return ret;
}

char * instruction_assign_label(instruction_t *instruction) {
  char buf[10];

  strrand(buf, 9);
  instruction_set_label(instruction, buf);
  return instruction -> label;
}

instruction_t * instruction_set_label(instruction_t *instruction, char *label) {
  instruction -> label = strdup(label);
  return instruction;
}

char * instruction_execute(instruction_t *instr, closure_t *closure) {
  instr_fnc_t fnc;

  if (script_debug) {
    debug("Executing %s", instruction_tostring(instr));
  }
  fnc = instruction_descr_map[instr -> type].function;
  return fnc(instr, closure);
}

char * instruction_tostring(instruction_t *instruction) {
  tostring_t   tostring;
  char        *s;
  static char  buf[100];
  char        *lbl;

  tostring = instruction_descr_map[instruction -> type].tostring;
  if (tostring) {
    s = tostring(instruction);
  } else {
    s = "";
  }
  lbl = (instruction -> label) ? instruction -> label : "";
  snprintf(buf, 100, "%-11.11s%-15.15s%s", lbl, instruction_descr_map[instruction -> type].name, s);
  return buf;
}

void instruction_free(instruction_t *instruction) {
  free_t freefnc;

  if (instruction) {
    freefnc = instruction_descr_map[instruction -> type].free;
    if (freefnc) {
      freefnc(instruction);
    }
    if (instruction -> label) {
      free(instruction -> label);
    }
    free(instruction);
  }
}


