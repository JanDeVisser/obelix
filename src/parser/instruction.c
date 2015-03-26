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
#include <exception.h>
#include <instruction.h>
#include <name.h>
#include <script.h>
#include <math.h>

typedef data_t * (*instr_fnc_t)(instruction_t *, closure_t *);

typedef struct _instruction_type_descr {
  instruction_type_t  type;
  instr_fnc_t         function;
  char               *name;
  free_t              free;
  tostring_t          tostring;
} instruction_type_descr_t;

static instruction_type_descr_t instruction_descr_map[];

static void             _instruction_free_name(instruction_t *);
static void             _instruction_free_value(instruction_t *);
static void             _instruction_free_name_value(instruction_t *);
static char *           _instruction_tostring_name(instruction_t *);
static char *           _instruction_tostring_value(instruction_t *);
static char *           _instruction_tostring_name_value(instruction_t *);
static data_t *         _instruction_get_variable(instruction_t *, closure_t *);

static data_t *         _instruction_execute_assign(instruction_t *, closure_t *);
static data_t *         _instruction_execute_catch(instruction_t *, closure_t *);
static data_t *         _instruction_execute_enter_context(instruction_t *, closure_t *);
static data_t *         _instruction_execute_function(instruction_t *, closure_t *);
static data_t *         _instruction_execute_jump(instruction_t *, closure_t *);
static data_t *         _instruction_execute_import(instruction_t *, closure_t *);
static data_t *         _instruction_execute_iter(instruction_t *, closure_t *);
static data_t *         _instruction_execute_leave_context(instruction_t *, closure_t *);
static data_t *         _instruction_execute_next(instruction_t *, closure_t *);
static data_t *         _instruction_execute_new(instruction_t *, closure_t *);
static data_t *         _instruction_execute_nop(instruction_t *, closure_t *);
static data_t *         _instruction_execute_pop(instruction_t *, closure_t *);
static data_t *         _instruction_execute_pushval(instruction_t *, closure_t *);
static data_t *         _instruction_execute_pushvar(instruction_t *, closure_t *);
static data_t *         _instruction_execute_test(instruction_t *, closure_t *);
static data_t *         _instruction_execute_throw(instruction_t *, closure_t *);

static instruction_type_descr_t instruction_descr_map[] = {
  { type: ITAssign,   function: _instruction_execute_assign,
    name: "Assign",   free: (free_t) _instruction_free_value,
    tostring: (tostring_t) _instruction_tostring_value },
  { type: ITEnterContext,   function: _instruction_execute_enter_context,
    name: "Enter",   free: (free_t) _instruction_free_name_value,
    tostring: (tostring_t) _instruction_tostring_name_value },
  { type: ITFunctionCall, function: _instruction_execute_function,
    name: "FunctionCall", free: (free_t) _instruction_free_value,
    tostring: (tostring_t) _instruction_tostring_value },
  { type: ITImport, function: _instruction_execute_import,
    name: "Import", free: (free_t) _instruction_free_value,
    tostring: (tostring_t) _instruction_tostring_value },
  { type: ITIter, function: _instruction_execute_iter,
    name: "Iterator", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
  { type: ITJump, function: _instruction_execute_jump,
    name: "Jump", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
  { type: ITLeaveContext,   function: _instruction_execute_leave_context,
    name: "Leave",   free: (free_t) _instruction_free_value,
    tostring: (tostring_t) _instruction_tostring_value },
  { type: ITNext, function: _instruction_execute_next,
    name: "Next", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
  { type: ITNop, function: _instruction_execute_nop,
    name: "Nop", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
  { type: ITPop, function: _instruction_execute_pop,
    name: "Pop", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
  { type: ITPushVal,  function: _instruction_execute_pushval,
    name: "PushVal",  free: (free_t) _instruction_free_value,
    tostring: (tostring_t) _instruction_tostring_value },
  { type: ITPushVar,  function: _instruction_execute_pushvar,
    name: "PushVar",  free: (free_t) _instruction_free_value,
    tostring: (tostring_t) _instruction_tostring_value },
  { type: ITTest, function: _instruction_execute_test,
    name: "Test", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
  { type: ITThrow, function: _instruction_execute_throw,
    name: "Throw", free: (free_t) _instruction_free_name,
    tostring: (tostring_t) _instruction_tostring_name },
};

typedef struct _function_call {
  name_t  *name;
  int      infix;
  int      arg_count;
  array_t *kwargs;
} function_call_t;

static void          _data_init_call(void) __attribute__((constructor));
static data_t *      _call_new(data_t *, va_list);
static void          _call_free(function_call_t *);
static data_t *      _call_copy(data_t *, data_t *);
static char *        _call_tostring(data_t *);

static int Call = -1;

static vtable_t _vtable_call[] = {
  { .id = FunctionNew,      .fnc = (void_t) _call_new },
  { .id = FunctionCopy,     .fnc = (void_t) _call_copy },
  { .id = FunctionFree,     .fnc = (void_t) _call_free },
  { .id = FunctionToString, .fnc = (void_t) _call_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_call = {
  .type =      -1,
  .type_name = "call",
  .vtable =    _vtable_call
};

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

void _data_init_call(void) {
  Call = typedescr_register(&_typedescr_call);  
}

data_t * _call_new(data_t *ret, va_list arg) {
  function_call_t *call;
  array_t         *kwargs;

  call = NEW(function_call_t);
  call -> name = name_copy(va_arg(arg, name_t *));
  call -> infix = va_arg(arg, int);
  call -> arg_count = va_arg(arg, int);
  kwargs = va_arg(arg, array_t *);
  call -> kwargs = (kwargs) ? array_copy(kwargs) : NULL;
  ret -> ptrval = call;
  return ret;
}

void _call_free(function_call_t *call) {
  if (call) {
    name_free(call -> name);
    array_free(call -> kwargs);
    free(call);
  }
}

data_t * _call_copy(data_t *target, data_t *src) {
  function_call_t *newcall;
  function_call_t *srccall = (function_call_t *) src -> ptrval;

  newcall = NEW(function_call_t);
  newcall -> name = name_copy(srccall -> name);
  newcall -> infix = srccall -> infix;
  newcall -> arg_count = srccall -> arg_count;
  newcall -> kwargs = array_copy(srccall -> kwargs);
  target -> ptrval = newcall;
  return target;
}

char * _call_tostring(data_t *d) {
  return name_tostring(((function_call_t *) d -> ptrval) -> name);
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

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

char * _instruction_tostring_name_value(instruction_t *instruction) {
  asprintf(&instruction -> str, "%s, %s", 
           data_tostring(instruction -> value), 
           instruction -> name);
  return NULL;
}

data_t * _instruction_get_variable(instruction_t *instr, closure_t *closure) {
  name_t *path = data_nameval(instr -> value);
  data_t *variable;
  data_t *c;
  
  c = data_create(Closure, closure);
  variable = data_get(c, path);
  data_free(c);
  return variable;
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/* -- V A R I A B L E  M A N A G E M E N T ------------------------------- */

data_t * _instruction_execute_assign(instruction_t *instr, closure_t *closure) {
  name_t *path = data_nameval(instr -> value);
  data_t *c;
  data_t *value;
  data_t *ret;

  value = closure_pop(closure);
  assert(value);
  if (script_debug) {
    debug(" -- value '%s'", data_tostring(value));
  }
  c = data_create(Closure, closure);
  ret = data_set(c, path, value);
  data_free(c);
  data_free(value);
  return (data_is_error(ret)) ? ret : NULL;
}

data_t * _instruction_execute_pushvar(instruction_t *instr, closure_t *closure) {
  data_t *value;
  data_t *ret;
  name_t *path = data_nameval(instr -> value);
  data_t *c;
  
  value = _instruction_get_variable(instr, closure);
  if (!data_is_error(value)) {
    if (script_debug) {
      debug(" -- value '%s'", data_tostring(value));
    }
    closure_push(closure, value);
    ret = NULL;
  } else {
    ret = value;
  }
  return ret;
}

/* -- E X C E P T I O N  H A N D L I N G ---------------------------------- */

data_t * _instruction_execute_enter_context(instruction_t *instr, closure_t *closure) {
  data_t *context;
  data_t *ret = NULL;
  name_t *enter = name_create(1, "__enter__");
  
  context = _instruction_get_variable(instr, closure);
  if (data_has_callable(context, enter)) {
    ret = data_invoke(context, enter, NULL, NULL);
  }
  data_free(context);
  if (ret && !data_is_error(ret)) {
    ret = NULL;
  }
  if (!ret) {
    datastack_push(closure -> catchpoints, data_create(String, instr -> name));
  }
  return ret;
}

data_t * _instruction_execute_leave_context(instruction_t *instr, closure_t *closure) {
  data_t  *error;
  error_t *e = NULL;
  data_t  *context;
  data_t  *ret = NULL;
  array_t *params;
  name_t  *exit = name_create(1, "__exit__");
  
  error = closure_pop(closure);
  context = _instruction_get_variable(instr, closure);
  if (data_is_error(error)) {
    e = data_errorval(error);
  } else {
    /*
     * If there is an error the catchpoint was already popped by 
     * _closure_execute_instruction:
     */
    data_free(datastack_pop(closure -> catchpoints));
  }
  if (data_has_callable(context, exit)) {
    params = data_array_create(1);
    if (e) {
      if ((e -> code != ErrorLeave) && (e -> code != ErrorExit)) {
        array_push(params, data_copy((e -> exception) ? e -> exception : error));
      }
    }
    if (!array_size(params)) {
      array_push(params, data_create(Bool, 0));
    }
    ret = data_invoke(context, exit, params, NULL);
    array_free(params);
    name_free(exit);
  }
  if (e && (e -> code == ErrorExit)) {
    /*
     * If the error is ErrorExit it needs to be bubbled up, and we really 
     * don't care what else happens.
     */
    ret = data_copy(error);
  } else if (!data_is_error(ret)) {
    ret = NULL;
  }
  data_free(error);
  data_free(context);
  if (script_debug && ret) {
    debug("    Leave: retval '%s'", data_tostring(ret));
  }
  return ret;
}

data_t * _instruction_execute_throw(instruction_t *instr, closure_t *closure) {
  data_t *exception = closure_pop(closure);

  return !data_is_error(exception) ? data_exception(exception) : exception;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_pushval(instruction_t *instr, closure_t *closure) {
  assert(instr -> value);
  closure_push(closure, instr -> value);
  return NULL;
}

data_t * _instruction_execute_function(instruction_t *instr, closure_t *closure) {
  function_call_t *call = (function_call_t *) instr -> value -> ptrval;
  data_t          *value;
  data_t          *ret = NULL;
  data_t          *self;
  data_t          *arg_name;
  dict_t          *kwargs = NULL;
  array_t         *args;
  int              ix;
  int              num;
  
  num = (call -> kwargs) ? array_size(call -> kwargs) : 0;
  if (script_debug) {
    debug(" -- #kwargs: %d", num);
  }
  if (num) {
    kwargs = strdata_dict_create();
    for (ix = 0; ix < num; ix++) {
      value = closure_pop(closure);
      assert(value);
      arg_name = data_array_get(call -> kwargs, num - ix - 1);
      dict_put(kwargs, data_tostring(arg_name), value);
    }
  }

  num = call -> arg_count;
  if (script_debug) {
    debug(" -- #arguments: %d", num);
  }
  args = data_array_create(num);
  for (ix = 0; ix < num; ix++) {
    value = closure_pop(closure);
    assert(value);
    array_set(args, num - ix - 1, value);
  }
  if (script_debug) {
    array_debug(args, " -- arguments: %s");
  }
  self = (call -> infix) ? NULL : data_create(Closure, closure);
  ret = data_invoke(self, call -> name, args, kwargs);
  data_free(self);
  array_free(args);
  dict_free(kwargs);
  if (ret && !data_is_error(ret)) {
    closure_push(closure, ret);
    ret = NULL;
  }
  return ret;
}

/* -- F L O W  C O N T R O L ---------------------------------------------- */

data_t * _instruction_execute_jump(instruction_t *instr, closure_t *closure) {
  assert(instr -> name);
  return data_create(String, instr -> name);
}

data_t * _instruction_execute_test(instruction_t *instr, closure_t *closure) {
  data_t  *ret;
  data_t  *value;
  data_t  *casted = NULL;
 
  value = closure_pop(closure);
  assert(value);
  assert(instr -> name);

  casted = data_cast(value, Bool);
  if (!casted) {
    ret = data_error(ErrorType, "Cannot convert '%s' to boolean",
                     data_tostring(value));
  } else {
    ret = (!casted -> intval) ? data_create(String, instr -> name) : NULL;
  }
  data_free(casted);
  data_free(value);
  return ret;
}

data_t * _instruction_execute_iter(instruction_t *instr, closure_t *closure) {
  data_t *value;
  data_t *iter;
  
  value = closure_pop(closure);
  iter = data_iter(value);
  closure_push(closure, iter);
  data_free(value);
  return NULL;
}

data_t * _instruction_execute_next(instruction_t *instr, closure_t *closure) {
  data_t  *ret = NULL;
  data_t  *iter;
  data_t  *next;
 
  iter = closure_pop(closure);
  assert(iter);
  assert(instr -> name);
  next = data_next(iter);
  
  if (data_is_error(next) && (data_errorval(next) -> code == ErrorExhausted)) {
    data_free(iter);
    ret = data_create(String, instr -> name);
  } else {
    closure_push(closure, iter);
    closure_push(closure, next);
  }
  return ret;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_import(instruction_t *instr, closure_t *closure) {
  name_t  *imp = data_nameval(instr -> value);
  data_t  *ret;

  ret = closure_import(closure, imp);
  name_free(imp);
  return (data_is_error(ret)) ? ret : NULL;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_nop(instruction_t *instr, closure_t *closure) {
  return NULL;
}

data_t * _instruction_execute_pop(instruction_t *instr, closure_t *closure) {
  data_t *value;

  value = closure_pop(closure);
  data_free(value);
  return NULL;
}

/* -- P U B L I C  F U N C T I O N S -------------------------------------- */

instruction_t * instruction_create(int type, char *name, data_t *value) {
  instruction_t *ret;

  ret = NEW(instruction_t);
  ret -> type = type;
  ret -> str = NULL;
  if (name) {
    ret -> name = strdup(name);
  }
  if (value) {
    ret -> value = value;
  }
  return ret;
}

instruction_t * instruction_create_assign(name_t *varname) {
  return instruction_create(ITAssign,
                            name_tostring(varname), 
			    data_create(Name, varname));
}

instruction_t *  instruction_create_enter_context(name_t *varname, char *catchpoint) {
  return instruction_create(ITEnterContext,
                            catchpoint, 
			    data_create(Name, varname));    
}

instruction_t *  instruction_create_leave_context(name_t *varname) {
  return instruction_create(ITLeaveContext,
                            name_tostring(varname), 
			    data_create(Name, varname));    
}

instruction_t * instruction_create_pushvar(name_t *varname) {
  return instruction_create(ITPushVar,
                            name_tostring(varname), 
			    data_create(Name, varname));
}

instruction_t * instruction_create_pushval(data_t *value) {
  return instruction_create(ITPushVal, NULL, data_copy(value));
}

instruction_t * instruction_create_function(name_t *name, int infix, 
                                            long num_args, array_t *kwargs) {
  instruction_t *ret;
  data_t        *call;

  call = data_create(Call, name, infix, num_args, kwargs);
  ret = instruction_create(ITFunctionCall, name_tostring(name), call);
  return ret;
}

instruction_t * instruction_create_test(char *label) {
  return instruction_create(ITTest, label, NULL);
}

instruction_t * instruction_create_jump(char *label) {
  return instruction_create(ITJump, label, NULL);
}

instruction_t * instruction_create_import(name_t *module) {
  return instruction_create(ITImport, NULL, data_create(Name, module));
}

instruction_t * instruction_create_iter() {
  return instruction_create(ITIter, "Iterate", NULL);
}

instruction_t * instruction_create_next(char *end_label) {
  return instruction_create(ITNext, end_label, NULL);
}

instruction_t * instruction_create_pop(void) {
  return instruction_create(ITPop, "Discard top-of-stack", NULL);
}

instruction_t * instruction_create_nop(void) {
  return instruction_create(ITNop, "Nothing", NULL);
}

instruction_t * instruction_create_throw(void) {
  return instruction_create(ITThrow, "Throw Exception", NULL);
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

data_t * instruction_execute(instruction_t *instr, closure_t *closure) {
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
  char        *free_me = NULL;
  char        *lbl;

  if (!instruction -> str) {
    tostring = instruction_descr_map[instruction -> type].tostring;
    if (tostring) {
      s = tostring(instruction);
      if (!s) {
        free_me = instruction -> str;
        s = free_me;
      }
    } else {
      s = "";
    }
    lbl = (instruction -> label) ? instruction -> label : "";
    asprintf(&instruction -> str, "%-11.11s%-15.15s%s", 
             lbl, 
             instruction_descr_map[instruction -> type].name, 
             s);
    free(free_me);
  }
  return instruction -> str;
}

void instruction_free(instruction_t *instruction) {
  free_t freefnc;

  if (instruction) {
    freefnc = instruction_descr_map[instruction -> type].free;
    if (freefnc) {
      freefnc(instruction);
    }
    free(instruction -> label);
    data_free(instruction -> value);
    free(instruction -> str);
    free(instruction);
  }
}


