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
#include <string.h>

#include <array.h>
#include <exception.h>
#include <instruction.h>
#include <logging.h>
#include <name.h>
#include <nvp.h>
#include <script.h>
#include <math.h>

int script_trace;

typedef data_t * (*instr_fnc_t)(instruction_t *, closure_t *);

typedef struct _instruction_type_descr {
  instruction_type_t  type;
  instr_fnc_t         function;
  char               *name;
  tostring_t          tostring;
} instruction_type_descr_t;

static instruction_type_descr_t instruction_descr_map[];

static void             _instruction_init(void) __attribute__((constructor(102)));

static char *           _instruction_tostring_name(instruction_t *);
static char *           _instruction_tostring_value(instruction_t *);
static char *           _instruction_tostring_name_value(instruction_t *);
static char *           _instruction_tostring_value_or_name(instruction_t *);
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
static data_t *         _instruction_execute_dup(instruction_t *, closure_t *);
static data_t *         _instruction_execute_swap(instruction_t *, closure_t *);

static data_t *         _instruction_execute_stash(instruction_t *, closure_t *);
static data_t *         _instruction_execute_unstash(instruction_t *, closure_t *);

static data_t *         _instruction_execute_pushval(instruction_t *, closure_t *);
static data_t *         _instruction_execute_pushvar(instruction_t *, closure_t *);
static data_t *         _instruction_execute_subscript(instruction_t *, closure_t *);
static data_t *         _instruction_execute_test(instruction_t *, closure_t *);
static data_t *         _instruction_execute_throw(instruction_t *, closure_t *);

static instruction_type_descr_t instruction_descr_map[] = {
  { .type = ITAssign,   
    .function = _instruction_execute_assign,
    .name = "Assign",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITDup,   
    .function = _instruction_execute_dup,
    .name = "Dup",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITEnterContext,
    .function = _instruction_execute_enter_context,
    .name = "Enter",
    .tostring = (tostring_t) _instruction_tostring_name_value },
  { .type = ITFunctionCall, 
    .function = _instruction_execute_function,
    .name = "FunctionCall",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITImport, 
    .function = _instruction_execute_import,
    .name = "Import",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITIter, 
    .function = _instruction_execute_iter,
    .name = "Iterator",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITJump, 
    .function = _instruction_execute_jump,
    .name = "Jump",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITLeaveContext,
    .function = _instruction_execute_leave_context,
    .name = "Leave",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITNext,
    .function = _instruction_execute_next,
    .name = "Next",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITNop,
    .function = _instruction_execute_nop,
    .name = "Nop",
    .tostring = (tostring_t) _instruction_tostring_value_or_name },
  { .type = ITPop,
    .function = _instruction_execute_pop,
    .name = "Pop",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITPushVal,
    .function = _instruction_execute_pushval,
    .name = "PushVal",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITPushVar,
    .function = _instruction_execute_pushvar,
    .name = "PushVar",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITStash,
    .function = _instruction_execute_stash,
    .name = "Stash",
    .tostring = (tostring_t) _instruction_tostring_value },
  { .type = ITSubscript,
    .function = _instruction_execute_subscript,
    .name = "Subscript",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITSwap,   
    .function = _instruction_execute_swap,
    .name = "Swap",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITTest,
    .function = _instruction_execute_test,
    .name = "Test",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITThrow,
    .function = _instruction_execute_throw,
    .name = "Throw",
    .tostring = (tostring_t) _instruction_tostring_name },
  { .type = ITUnstash,
    .function = _instruction_execute_unstash,
    .name = "Unstash",
    .tostring = (tostring_t) _instruction_tostring_value },
};

typedef struct _function_call {
  data_t      data;
  name_t     *name;
  callflag_t  flags;
  int         arg_count;
  array_t    *kwargs;
} function_call_t;

static void          _data_init_call(void) __attribute__((constructor));
static data_t *      _call_new(int, va_list);
static void          _call_free(function_call_t *);
static data_t *      _call_copy(data_t *, data_t *);
static char *        _call_tostring(data_t *);

static int Call = -1;

static vtable_t _vtable_call[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _call_new },
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

void _instruction_init(void) {
  logging_register_category("trace", &script_trace);
}

void _data_init_call(void) {
  Call = typedescr_register(&_typedescr_call);  
}

data_t * _call_new(int type, va_list arg) {
  function_call_t *call;
  array_t         *kwargs;

  call = NEW(function_call_t);
  call -> name = name_copy(va_arg(arg, name_t *));
  call -> flags = va_arg(arg, int);
  call -> arg_count = va_arg(arg, int);
  kwargs = va_arg(arg, array_t *);
  call -> kwargs = (kwargs) ? array_copy(kwargs) : NULL;
  call -> data.ptrval = call;
  return &call -> data;
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
  newcall -> flags = srccall -> flags;
  newcall -> arg_count = srccall -> arg_count;
  newcall -> kwargs = array_copy(srccall -> kwargs);
  target -> ptrval = newcall;
  return target;
}

char * _call_tostring(data_t *d) {
  return name_tostring(((function_call_t *) d -> ptrval) -> name);
}

/* -- T O _ S T R I N G  F U N C T I O N S -------------------------------- */

char * _instruction_tostring_name(instruction_t *instruction) {
  return instruction -> name ? instruction -> name : "";
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

char * _instruction_tostring_value_or_name(instruction_t *instruction) {
  if (instruction -> value) {
    return _instruction_tostring_value(instruction);
  } else {
    return _instruction_tostring_name(instruction);
  }
}

/* -- H E L P E R  F U N C T I O N S -------------------------------------- */

data_t * _instruction_get_variable(instruction_t *instr, closure_t *closure) {
  name_t *path = data_nameval(instr -> value);
  data_t *variable;
  data_t *c;
  
  c = data_create(Closure, closure);
  variable = data_get(c, path);
  data_free(c);
  if (script_debug) {
    debug("%s.get(%s) = %s", closure_tostring(closure), name_tostring(path), data_tostring(variable));
  }
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
  return (data_is_unhandled_exception(ret)) ? ret : NULL;
}

data_t * _instruction_execute_pushvar(instruction_t *instr, closure_t *closure) {
  data_t *value;
  data_t *ret;
  name_t *path = data_nameval(instr -> value);
  data_t *c;
  
  value = _instruction_get_variable(instr, closure);
  if (data_is_unhandled_exception(value)) {
    ret = value;
  } else {
    if (script_debug) {
      debug(" -- value '%s'", data_tostring(value));
    }
    closure_push(closure, value);
    ret = NULL;
  }
  return ret;
}

data_t * _instruction_execute_subscript(instruction_t *instr, closure_t *closure) {
  data_t *subscript = closure_pop(closure);
  data_t *subscripted  = closure_pop(closure);
  name_t *name = name_create(1, data_tostring(subscript));
  data_t *slice;
  data_t *ret;

  slice = data_resolve(subscripted, name);
  if (!slice) {
    ret = data_exception(ErrorName,
                         "Subscript '%s' not valid for %s '%s'",
                         data_tostring(subscript),
                         data_typedescr(subscript) -> type_name,
                         data_tostring(subscripted));
  } else if (data_is_unhandled_exception(slice)) {
    ret = slice;
  } else {
    closure_push(closure, slice);
  }
  name_free(name);
  data_free(subscript);
  data_free(subscripted);
  return NULL;
}

/* -- E X C E P T I O N  H A N D L I N G ---------------------------------- */

data_t * _instruction_execute_enter_context(instruction_t *instr, closure_t *closure) {
  data_t    *context;
  data_t    *ret = NULL;
  data_t * (*fnc)(data_t *);
  data_t    *catchpoint;
  nvp_t     *nvp_cp;
  
  context = _instruction_get_variable(instr, closure);
  if (data_hastype(context, CtxHandler)) {
    fnc = (data_t * (*)(data_t *)) data_get_function(context, FunctionEnter);
    if (fnc) {
      ret = fnc(context);
    }
  }
  if (ret && !data_is_exception(ret)) {
    ret = NULL;
  }
  if (!ret) {
    catchpoint = data_create(NVP, data_create(String, instr -> name), data_copy(context));
    datastack_push(closure -> catchpoints, catchpoint);
  }
  data_free(context);
  return ret;
}

data_t * _instruction_execute_leave_context(instruction_t *instr, closure_t *closure) {
  data_t        *error;
  exception_t   *e = NULL;
  data_t        *context;
  data_t        *param;
  data_t        *ret = NULL;
  data_t *     (*fnc)(data_t *, data_t *);
  nvp_t         *cp;
  data_t        *cp_data;
  
  error = closure_pop(closure);
  if (data_is_exception(error)) {
    e = data_exceptionval(error);
    e -> handled = TRUE;
  }
  cp_data = datastack_pop(closure -> catchpoints);
  cp = data_nvpval(cp_data);
  context = data_copy(cp -> value);
  data_free(cp_data);
  if (data_is_exception(context)) {
    ret = data_copy(context);
  } else if (data_hastype(context, CtxHandler)) {
    fnc = (data_t * (*)(data_t *, data_t *)) data_get_function(context, FunctionLeave);
    if (fnc) {
      if (e && (e -> code != ErrorLeave) && (e -> code != ErrorExit)) {
        param = data_copy(error);
      } else {
        param = data_create(Bool, 0);
      }
      ret = fnc(context, param);
    }
  }
  if (e && (e -> code == ErrorExit)) {
    /*
    * If the error is ErrorExit it needs to be bubbled up, and we really 
    * don't care what else happens.
    */
    ret = data_copy(error);
  } else if (!data_is_exception(ret)) {
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

  return !data_is_exception(exception) ? data_throwable(exception) : exception;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_pushval(instruction_t *instr, closure_t *closure) {
  assert(instr -> value);
  closure_push(closure, instr -> value);
  return NULL;
}

name_t * _instruction_setup_constructor(data_t *closure, 
                                        function_call_t *constructor) {
  data_t         *self;
  data_t         *s = NULL;
  script_t       *script;
  data_t         *dscript;
  bound_method_t *bm = NULL;
  char           *name;
  char           *ptr;
  name_t         *ret = NULL;
  name_t         *name_self = name_create(1, "self");
  
  self = data_get(closure, name_self);
  name_free(name_self);
  if (data_is_object(self)) {
    s = data_resolve(closure, constructor -> name);
    if (data_is_script(s)) {
      script = data_scriptval(s);
    } else if (data_is_boundmethod(s)) {
      script = data_boundmethodval(s) -> script;
    } else if (data_is_closure(s)) {
      script = data_closureval(s) -> script;
    }
    if (script) {
      bm = script_bind(script, data_objectval(self));
      asprintf(&name, "$%s", name_tostring(constructor -> name));
      for (ptr = strchr(name, '.'); ptr; ptr = strchr(ptr + 1, '.')) {
        *ptr = '_';
      }
      ret = name_create(1, name);
      data_set(closure, ret, data_create(BoundMethod, bm));
      object_bind_all(data_objectval(self), dscript = data_create(Script, script));
      data_free(dscript);
    }
  }
  return ret;
}

data_t * _instruction_execute_function(instruction_t *instr, closure_t *closure) {
  function_call_t *call = (function_call_t *) instr -> value -> ptrval;
  data_t          *value;
  data_t          *ret = NULL;
  data_t          *self;
  data_t          *arg_name;
  data_t          *caller;
  dict_t          *kwargs = NULL;
  array_t         *args = NULL;
  int              ix;
  int              num;
  name_t          *name;
  
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
      dict_put(kwargs, strdup(data_tostring(arg_name)), value);
    }
  }

  num = call -> arg_count;
  if (script_debug) {
    debug(" -- #arguments: %d", num);
  }
  if (num) {
    args = data_array_create(num);
    for (ix = 0; ix < num; ix++) {
      value = closure_pop(closure);
      assert(value);
      array_set(args, num - ix - 1, value);
    }
    if (script_debug) {
      array_debug(args, " -- arguments: %s");
    }
  }
  caller = data_create(Closure, closure);
  self = (call -> flags & CFInfix) ? NULL : caller;
  if (call -> flags & CFConstructor) {
    name = _instruction_setup_constructor(self, call);
  } else {
    name = name_copy(call -> name);
  }
  ret = data_invoke(self, name, args, kwargs);
  name_free(name);
  data_free(caller);
  array_free(args);
  dict_free(kwargs);
  if (ret && !data_is_exception(ret)) {
    closure_push(closure, ret);
    ret = NULL;
  }
  if (script_debug) {
    debug(" -- return value '%s'", data_tostring(ret));
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
    ret = data_exception(ErrorType, "Cannot convert %s '%s' to boolean",
                         data_typedescr(value) -> type_name,
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
  
  if (data_is_exception(next) && (data_exceptionval(next) -> code == ErrorExhausted)) {
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
  if (!data_is_exception(ret)) {
    data_free(ret);
    ret = NULL;
  }
  return ret;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_nop(instruction_t *instr, closure_t *closure) {
  if (instr -> value) {
    closure_set_location(closure, instr -> value);
  }
  return NULL;
}

data_t * _instruction_execute_pop(instruction_t *instr, closure_t *closure) {
  data_t *value;

  value = closure_pop(closure);
  data_free(value);
  return NULL;
}

data_t * _instruction_execute_dup(instruction_t *instr, closure_t *closure) {
  closure_push(closure, data_copy(closure_peek(closure)));
  return NULL;
}

data_t * _instruction_execute_swap(instruction_t *instr, closure_t *closure) {
  data_t *v1;
  data_t *v2;

  v1 = closure_pop(closure);
  v2 = closure_pop(closure);
  closure_push(closure, v1);
  closure_push(closure, v2);
  return NULL;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_stash(instruction_t *instr, closure_t *closure) {
  assert(data_intval(instr -> value) < NUM_STASHES);
  closure_stash(closure,
                data_intval(instr -> value),
                data_copy(closure_pop(closure)));
  return NULL;
}

data_t * _instruction_execute_unstash(instruction_t *instr, closure_t *closure) {
  assert(data_intval(instr -> value) < NUM_STASHES);
  closure_push(closure, 
               data_copy(closure_unstash(closure,
                                         data_intval(instr -> value))));
  return NULL;
}

/* -- P U B L I C  F U N C T I O N S -------------------------------------- */

instruction_t * instruction_create(int type, char *name, data_t *value) {
  instruction_t *ret;

  ret = NEW(instruction_t);
  ret -> type = type;
  ret -> line = -1;
  ret -> str = NULL;
  ret -> name = (name) ? strdup(name) : NULL;
  ret -> value = value;
  memset(ret -> label, 0, 9);
  return ret;
}

instruction_t * instruction_create_assign(name_t *varname) {
  return instruction_create(ITAssign,
                            name_tostring(varname), 
			    data_create(Name, varname));
}

instruction_t *  instruction_create_enter_context(name_t *varname, data_t *catchpoint) {
  return instruction_create(ITEnterContext,
                            data_charval(catchpoint), 
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

instruction_t * instruction_create_function(name_t *name, callflag_t flags, 
                                            long num_args, array_t *kwargs) {
  instruction_t *ret;
  data_t        *call;

  call = data_create(Call, name, flags, num_args, kwargs);
  ret = instruction_create(ITFunctionCall, name_tostring(name), call);
  return ret;
}

instruction_t * instruction_create_test(data_t *label) {
  return instruction_create(ITTest, data_charval(label), NULL);
}

instruction_t * instruction_create_jump(data_t *label) {
  return instruction_create(ITJump, data_charval(label), NULL);
}

instruction_t * instruction_create_import(name_t *module) {
  return instruction_create(ITImport, NULL, data_create(Name, module));
}

instruction_t * instruction_create_next(data_t *end_label) {
  return instruction_create(ITNext, data_charval(end_label), NULL);
}

instruction_t * instruction_create_mark(int line) {
  instruction_t *nop = instruction_create_nop();
  nop -> value = data_create(Int, line);
  return nop;
}

instruction_t * instruction_create_stash(unsigned int stash) {
  assert(stash < NUM_STASHES);
  return instruction_create(ITStash, "Stash", data_create(Int, stash));
}

instruction_t * instruction_create_unstash(unsigned int stash) {
  assert(stash < NUM_STASHES);
  return instruction_create(ITUnstash, "Unstash", data_create(Int, stash));
}

char * instruction_assign_label(instruction_t *instruction) {
  strrand(instruction -> label, 8);
  return instruction -> label;
}

instruction_t * instruction_set_label(instruction_t *instruction, data_t *label) {
  memset(instruction -> label, 0, 9);
  strncpy(instruction -> label, data_charval(label), 8);
  return instruction;
}

data_t * instruction_execute(instruction_t *instr, closure_t *closure) {
  instr_fnc_t  fnc;

  if (script_debug) {
    debug("Executing %s", instruction_tostring(instr));
  }
  if (script_trace) {
    debug("%-20.20s %s", closure_tostring(closure), instruction_tostring(instr));
  }
  fnc = instruction_descr_map[instr -> type].function;
  return fnc(instr, closure);
}

char * instruction_tostring(instruction_t *instruction) {
  tostring_t   tostring;
  char        *s;
  char        *free_me = NULL;
  char        *lbl;
  char         line[7];

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
    if (instruction -> line > 0) {
      snprintf(line, 7, "%6d", instruction -> line);
    } else {
      line[0] = 0;
    }
    asprintf(&instruction -> str, "%-6s %-11.11s%-15.15s%s", 
             line,
             instruction -> label, 
             instruction_descr_map[instruction -> type].name, 
             s);
    free(free_me);
  }
  return instruction -> str;
}

void instruction_free(instruction_t *instruction) {
  if (instruction) {
    free(instruction -> name);
    data_free(instruction -> value);
    free(instruction -> str);
    free(instruction);
  }
}
