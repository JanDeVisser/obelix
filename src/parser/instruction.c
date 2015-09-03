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
#include <boundmethod.h>
#include <bytecode.h>
#include <exception.h>
#include <instruction.h>
#include <logging.h>
#include <name.h>
#include <nvp.h>
#include <script.h>
#include <thread.h>
#include <vm.h>

int script_trace = 0;
extern int script_debug;

static void             _instruction_init(void) __attribute__((constructor(102)));
static int              _instruction_type_register(char *, int, vtable_t *);
static void             _instruction_tracemsg(char *, ...);

static data_t *         _instr_new(int, va_list);
static void             _instr_free(instruction_t *);

static char *           _instruction_tostring_name(data_t *);
static char *           _instruction_tostring_value(data_t *);
static char *           _instruction_tostring_name_value(data_t *);
static char *           _instruction_tostring_value_or_name(data_t *);
static data_t *         _instruction_get_variable(instruction_t *, data_t *);
static char *           _instruction_tostring(instruction_t *, char *);

int Instruction = -1;

static vtable_t _vtable_instruction[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _instr_new },
  { .id = FunctionFree,     .fnc = (void_t) _instr_free },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_instruction = {
  .type =      -1,
  .type_name = "instruction",
  .vtable =    _vtable_instruction
};

int ITByValue = -1;

static vtable_t _vtable_tostring_value[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_value },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_byvalue = {
  .type =      -1,
  .type_name = "instruction_byvalue",
  .vtable =    _vtable_tostring_value
};

int ITByName = -1;

static vtable_t _vtable_tostring_name[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_name },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_byname = {
  .type =      -1,
  .type_name = "instruction_byname",
  .vtable =    _vtable_tostring_name
};

int ITByNameValue = -1;

static vtable_t _vtable_tostring_name_value[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_name_value },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_bynamevalue = {
  .type =      -1,
  .type_name = "instruction_bynamevalue",
  .vtable =    _vtable_tostring_name_value
};

int ITByValueOrName = -1;

static vtable_t _vtable_tostring_value_or_name[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_value_or_name },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_byvalue_or_name = {
  .type =      -1,
  .type_name = "instruction_byvalue_or_name",
  .vtable =    _vtable_tostring_value_or_name
};

#define InstructionType(t, s)                                                \
int             IT ## t = -1;                                                \
static data_t * _instruction_call_ ## t(data_t *, array_t *, dict_t *);      \
static data_t * _instruction_execute_ ## t(instruction_t *,                  \
                                           data_t *,                         \
                                           vm_t *,                           \
                                           bytecode_t *);                    \
static void     _register_ ## t(void) __attribute__((constructor(200)));     \
static vtable_t _vtable_ ## t [] = {                                         \
  { .id = FunctionCall, .fnc = (void_t) _instruction_call_ ## t },           \
  { .id = FunctionNone, .fnc = NULL }                                        \
};                                                                           \
void _register_ ## t(void) {                                                 \
  IT ## t = _instruction_type_register(#t, ITBy ## s, _vtable_ ## t);        \
}                                                                            \
data_t * _instruction_call_ ## t(data_t *data, array_t *p, dict_t *kw) {     \
  instruction_t *instr = data_as_instruction(data);                          \
  data_t        *scope = data_array_get(p, 0);                               \
  vm_t          *vm = data_as_vm(data_array_get(p, 1));                      \
  bytecode_t    *bytecode = data_as_bytecode(data_array_get(p, 2));          \
  if (script_debug) {                                                        \
    debug("Executing %s", instruction_tostring(instr));                      \
  }                                                                          \
  _instruction_tracemsg("%-60.60s%s",                                        \
                        instruction_tostring(instr),                         \
                        data_tostring(scope));                               \
  return _instruction_execute_ ## t(instr, scope, vm, bytecode);             \
}

InstructionType(Assign,       Value);
InstructionType(Decr,         Name);
InstructionType(Dup,          Name);
InstructionType(EndLoop,      Name);
InstructionType(EnterContext, Name);
InstructionType(FunctionCall, NameValue);
InstructionType(Incr,         Name);
InstructionType(Iter,         Name);
InstructionType(Jump,         Name);
InstructionType(LeaveContext, Name);
InstructionType(Next,         Name);
InstructionType(Nop,          ValueOrName);
InstructionType(Pop,          Name);
InstructionType(PushCtx,      Name);
InstructionType(PushVal,      Value);
InstructionType(Deref,        Value);
InstructionType(PushScope,    Name);
InstructionType(Return,       Name);
InstructionType(Stash,        Value);
InstructionType(Subscript,    Name);
InstructionType(Swap,         Name);
InstructionType(Test,         Name);
InstructionType(Throw,        Name);
InstructionType(Unstash,      Value);
InstructionType(VMStatus,     Value);
InstructionType(Yield,        Name);

static function_call_t * _call_new(int, va_list);
static void              _call_free(function_call_t *);
static char *            _call_allocstring(function_call_t *);
static dict_t *          _call_build_kwargs(function_call_t *, vm_t *);

static int Call = -1;

static vtable_t _vtable_call[] = {
  { .id = FunctionFactory,     .fnc = (void_t) _call_new },
  { .id = FunctionFree,        .fnc = (void_t) _call_free },
  { .id = FunctionAllocString, .fnc = (void_t) _call_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

static name_t *name_empty = NULL;
static name_t *name_self = NULL;

/* ----------------------------------------------------------------------- */

void _instruction_init(void) {
  int         ix;
  typedescr_t type;

  logging_register_category("trace", &script_trace);

  Instruction = typedescr_register(&_typedescr_instruction);
  ITByName = typedescr_register(&_typedescr_byname);
  ITByValue = typedescr_register(&_typedescr_byvalue);
  ITByNameValue = typedescr_register(&_typedescr_bynamevalue);
  ITByValueOrName = typedescr_register(&_typedescr_byvalue_or_name);
  Call = typedescr_create_and_register(Call, "call", _vtable_call, NULL);
  interface_register(Scope, "scope", 2, FunctionResolve, FunctionSet);
  name_empty = name_create(0);
  name_self = name_create(1, "self");
}

int _instruction_type_register(char *name, int inherits, vtable_t *vtable) {
  typedescr_t td;

  memset(&td, 0, sizeof(typedescr_t));
  td.type = -1;
  td.type_name = name;
  td.inherits[0] = Instruction;
  td.inherits[1] = inherits;
  td.inherits[2] = NoType;
  td.vtable = vtable;
  return typedescr_register(&td);
}

void _instruction_tracemsg(char *fmt, ...) {
  va_list  args;

  if (script_trace) {
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
}

void instruction_trace(char *op, char *fmt, ...) {
  va_list  args;
  char    *buf;

  if (script_trace) {
    va_start(args, fmt);
    vasprintf(&buf, fmt, args);
    va_end(args);
    _instruction_tracemsg("%-16.16s%s", op, buf);
    free(buf);
  }
}

/* ----------------------------------------------------------------------- */

function_call_t * _call_new(int type, va_list arg) {
  function_call_t *call;
  array_t         *kwargs;

  call = data_new(Call, function_call_t);
  call -> flags = va_arg(arg, int);
  call -> arg_count = va_arg(arg, int);
  kwargs = va_arg(arg, array_t *);
  call -> kwargs = (kwargs) ? array_copy(kwargs) : NULL;
  return call;
}

void _call_free(function_call_t *call) {
  if (call) {
    array_free(call -> kwargs);
    free(call);
  }
}

char * _call_allocstring(function_call_t *call) {
  char *buf;

  asprintf(&buf, "(argv[%d]%s%s)",
           call -> arg_count,
           (call -> kwargs && array_size(call -> kwargs)) ? ", " : "",
           (call -> kwargs && array_size(call -> kwargs)) ? array_tostring(call -> kwargs) : "");
  return buf;
}

dict_t * _call_build_kwargs(function_call_t *call, vm_t *vm) {
  int     num;
  dict_t *ret = NULL;
  data_t *value;
  int     ix;
  data_t *arg_name;

  num = (call -> kwargs) ? array_size(call -> kwargs) : 0;
  if (script_debug) {
    debug(" -- #kwargs: %d", num);
  }
  if (num) {
    ret = strdata_dict_create();
    for (ix = 0; ix < num; ix++) {
      value = vm_pop(vm);
      assert(value);
      arg_name = data_array_get(call -> kwargs, num - ix - 1);
      dict_put(ret, strdup(data_tostring(arg_name)), value);
    }
  }
  return ret;
}

array_t * _call_build_args(function_call_t *call, vm_t *vm) {
  int      num;
  array_t *ret = NULL;
  data_t  *value;
  int      ix;

  num = call -> arg_count;
  if (call -> flags & CFVarargs) {
    value = vm_pop(vm);
    num += data_intval(value);
    data_free(value);
  }
  if (script_debug) {
    debug(" -- #arguments: %d", num);
  }
  if (num) {
    ret = data_array_create(num);
    for (ix = 0; ix < num; ix++) {
      value = vm_pop(vm);
      assert(value);
      array_set(ret, num - ix - 1, value);
    }
  }
  return ret;
}

/* -- T O _ S T R I N G  F U N C T I O N S -------------------------------- */

char * _instruction_name(data_t *data) {
  instruction_t *instruction = data_as_instruction(data);

  return instruction -> name ? instruction -> name : "";
}

char * _instruction_tostring_name(data_t *data) {
  instruction_t *instruction = data_as_instruction(data);

  _instruction_tostring(instruction, _instruction_name(data));
  return NULL;
}

char * _instruction_tostring_value(data_t *data) {
  instruction_t *instruction = data_as_instruction(data);

  _instruction_tostring(instruction, data_tostring(instruction -> value));
  return NULL;
}

char * _instruction_tostring_name_value(data_t *data) {
  instruction_t *instruction = data_as_instruction(data);
  char *v = data_tostring(instruction -> value);
  char *s = NULL;
  char *free_me = NULL;

  if (v && strlen(v)) {
    asprintf(&free_me, "%s%s",
             _instruction_name(data), v);
    s = free_me;
  } else {
    s = _instruction_tostring_name(data);
  }
  _instruction_tostring(instruction, s);
  free(free_me);
  return NULL;
}

char * _instruction_tostring_value_or_name(data_t *data) {
  instruction_t *instruction = data_as_instruction(data);

  if (instruction -> value) {
    _instruction_tostring_value(data);
  } else {
    _instruction_tostring_name(data);
  }
  return NULL;
}

int * _instruction_label_length(char *label, int *count) {
  if (*count == 0) {
    *count = 12;
  } else {
    *count += 20;
  }
  return count;
}

char * _instruction_label_string(char *label, char *buffer) {
  if (strlen(buffer) == 0) {
    sprintf(buffer, " %-11.11s", label);
  } else {
    sprintf(buffer, "\n%7.7s%-11.11s", "", label);
  }
  return buffer;
}

char * _instruction_tostring(instruction_t *instruction, char *s) {
  char *lbl;
  char  line[7];
  int   count = 0;

  if (instruction -> line > 0) {
    snprintf(line, 7, "%6d", instruction -> line);
  } else {
    line[0] = 0;
  }
  set_reduce(instruction -> labels, (reduce_t) _instruction_label_length, &count);
  if (count) {
    lbl = (char *) new(count);
    set_reduce(instruction -> labels, (reduce_t) _instruction_label_string, lbl);
  } else {
    lbl = (char *) new(12);
    sprintf(lbl, "%12.12s", "");
  }
  asprintf(&instruction -> _d.str, "%-6s %s%-15.15s%-27.27s",
           line, lbl,
           data_typedescr((data_t *) instruction) -> type_name,
           (s) ? s : "");
  free(lbl);
  return NULL;
}

/* -- H E L P E R  F U N C T I O N S -------------------------------------- */

data_t * _instruction_get_variable(instruction_t *instr, data_t *scope) {
  name_t *path = data_as_name(instr -> value);
  data_t *variable = NULL;

  if (path && name_size(path)) {
    variable = data_get(scope, path);
    if (script_debug) {
      debug("%s.get(%s) = %s", data_tostring(scope), name_tostring(path), data_tostring(variable));
    }
  }
  return variable;
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

/* -- V A R I A B L E  M A N A G E M E N T ------------------------------- */

data_t * _instruction_execute_Assign(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  name_t *path = data_as_name(instr -> value);
  data_t *value;
  data_t *ret;

  value = vm_pop(vm);
  assert(value);
  if (script_debug) {
    debug(" -- value '%s'", data_tostring(value));
  }
  ret = data_set(scope, path, value);
  data_free(value);
  return (data_is_unhandled_exception(ret)) ? ret : NULL;
}

data_t * _instruction_execute_Deref(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *value;
  data_t *ret;
  data_t *start_obj = vm_pop(vm);
  name_t *path = data_as_name(instr -> value);

  value = _instruction_get_variable(instr, start_obj);
  if (data_is_unhandled_exception(value)) {
    ret = value;
  } else {
    if (script_debug) {
      debug(" -- value '%s'", data_tostring(value));
    }
    vm_push(vm, value);
    ret = NULL;
  }
  return ret;
}

data_t * _instruction_execute_Subscript(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *subscript = vm_pop(vm);
  data_t *subscripted  = vm_pop(vm);
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
    vm_push(vm, slice);
  }
  name_free(name);
  data_free(subscript);
  data_free(subscripted);
  return NULL;
}

data_t * _instruction_execute_PushScope(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  vm_push(vm, scope);
  return NULL;
}

/* -- E X C E P T I O N  H A N D L I N G ---------------------------------- */

data_t * _instruction_execute_EnterContext(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t     *context;
  data_t     *ret = NULL;
  data_t *  (*fnc)(data_t *);
  vm_t       *v;

  context = _instruction_get_variable(instr, scope);
  if (context && data_hastype(context, CtxHandler)) {
    fnc = (data_t * (*)(data_t *)) data_get_function(context, FunctionEnter);
    if (fnc) {
      ret = fnc(context);
    }
  }
  if (ret && !data_is_exception(ret)) {
    ret = NULL;
  }
  if (!ret) {
    vm_push_context(vm, instr -> name, context);
  }
  data_free(context);
  return ret;
}

data_t * _instruction_execute_LeaveContext(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t      *error;
  exception_t *e = NULL;
  data_t      *context;
  data_t      *param;
  data_t      *ret = NULL;
  data_t *   (*fnc)(data_t *, data_t *);
  nvp_t       *cp;
  data_t      *cp_data;
  thread_t    *thread;
  int          is_leaving;

  error = vm -> exception;
  if (data_is_exception(error)) {
    e = data_as_exception(error);
    e -> handled = TRUE;
  }
  cp = vm_pop_context(vm);
  context = data_copy(cp -> value);
  nvp_free(cp);
  if (data_is_exception(context)) {
    ret = data_copy(context);
  } else {
    if (context && data_hastype(context, CtxHandler)) {
      fnc = (data_t * (*)(data_t *, data_t *)) data_get_function(context, FunctionLeave);
      if (fnc) {
        if (e && (e -> code != ErrorLeave)
              && (e -> code != ErrorReturn)) {
          param = data_copy(error);
        } else {
          param = data_false();
        }
        thread = thread_self();
        is_leaving = thread_has_status(thread, TSFLeave);
        if (!is_leaving) {
          thread_set_status(thread, TSFLeave);
        }
        ret = fnc(context, param);
        if (!is_leaving) {
          thread_unset_status(thread, TSFLeave);
        }
      }
    } else {
      vm_push(vm, data_copy(error));
    }
  }
  if (e && ((e -> code == ErrorExit) || (e -> code == ErrorReturn))) {
    /*
    * If the error is ErrorExit or ErrorReturn  needs to be bubbled up, and we
    * really don't care what else happens.
    */
    ret = data_copy(error);
  } else if (!data_is_exception(ret)) {
    data_free(vm -> exception);
    vm -> exception = NULL;
    ret = NULL;
  }
  data_free(error);
  data_free(context);
  if (script_debug && ret) {
    debug("    Leave: retval '%s'", data_tostring(ret));
  }
  return ret;
}

data_t * _instruction_execute_Throw(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *exception = vm_pop(vm);

  return !data_is_exception(exception) ? data_throwable(exception) : exception;
}

data_t * _instruction_execute_Return(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t      *retval = vm_pop(vm);
  exception_t *ex;

  ex = exception_create(ErrorReturn, "Return Value");
  ex -> throwable = retval;
  return data_create(Exception, ex);
}

data_t * _instruction_execute_Yield(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t      *retval = vm_pop(vm);
  exception_t *ex;

  ex = exception_create(ErrorYield, "Yield Value");
  ex -> throwable = retval;
  return data_create(Exception, ex);
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_PushCtx(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *cp_data;
  data_t *context;
  nvp_t  *cp;

  if (datastack_depth(vm -> contexts)) {
    cp = vm_peek_context(vm);
    context = data_copy(cp -> value);
    vm_push(vm, context);
    return NULL;
  } else {
    return data_exception(ErrorInternalError,
                          "%s: No context set",
                          instruction_tostring(instr));
  }
}

data_t * _instruction_execute_PushVal(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  assert(instr -> value);
  vm_push(vm, instr -> value);
  return NULL;
}

data_t * _instruction_setup_constructor(data_t *callable, data_t *scope,
                                        function_call_t *constructor) {
  data_t         *self;
  object_t       *obj;
  script_t       *script;
  data_t         *dscript;
  data_t         *ret;

  self = data_get(scope, name_self);
  if (data_is_object(self)) {
    obj = data_as_object(self);
    if (data_is_script(callable)) {
      script = data_as_script(callable);
    } else if (data_is_bound_method(callable)) {
      script = data_as_bound_method(callable) -> script;
    } else if (data_is_closure(callable)) {
      script = data_as_closure(callable) -> script;
    }
    if (script) {
      ret = data_create(BoundMethod, script_bind(script, obj));
      object_bind_all(obj, dscript = data_create(Script, script));
      data_free(dscript);
    }
  }
  return ret;
}

data_t * _instruction_execute_FunctionCall(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  function_call_t *call = (function_call_t *) instr -> value;
  data_t          *ret = NULL;
  data_t          *callable;
  dict_t          *kwargs = NULL;
  array_t         *args = NULL;

  if (call -> flags & CFInfix) {
    callable = vm_pop(vm);
  }
  kwargs = _call_build_kwargs(call, vm);
  args = _call_build_args(call, vm);
  if (!(call -> flags & CFInfix)) {
    callable = vm_pop(vm);
  }

  if (call -> flags & CFConstructor) {
    callable = _instruction_setup_constructor(callable, scope, call);
  }
  if (!data_is_callable(callable)) {
    ret = data_exception(ErrorNotCallable,
                         "Atom '%s' is not callable",
                         data_tostring(callable));
  } else {
    if (script_debug) {
      debug(" -- Calling %s(%s, %s)",
            instr -> name,
            data_tostring(callable),
            (args) ? array_tostring(args) : "[]",
            (kwargs) ? dict_tostring(kwargs) : "{}");
    }
    instruction_trace("Calling", "%s(%s, %s)",
                      instr -> name,
                      (args) ? array_tostring(args) : "[]",
                      (kwargs) ? dict_tostring(kwargs) : "{}");
    ret = data_call(callable, args, kwargs);
    if (ret) {
      if (data_is_exception(ret)) {
        if (script_debug) {
          debug(" -- exception '%s' thrown",
                data_tostring(ret));
        }
      } else {
        if (script_debug) {
          debug(" -- return value '%s' [%s]",
                data_tostring(ret), data_typename(ret));
        }
        vm_push(vm, ret);
        ret = NULL;
      }
    } else {
      if (script_debug) {
        debug(" -- return value NULL");
      }
    }
  }
  data_free(callable);
  array_free(args);
  dict_free(kwargs);
  return ret;
}

data_t * _instruction_execute_Decr(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *value = vm_pop(vm);

  vm_push(vm, data_create(Int, data_intval(value) - 1));
  data_free(value);
  return NULL;
}

data_t * _instruction_execute_Incr(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *value = vm_pop(vm);

  vm_push(vm, data_create(Int, data_intval(value) + 1));
  data_free(value);
  return NULL;
}

/* -- F L O W  C O N T R O L ---------------------------------------------- */

data_t * _instruction_execute_VMStatus(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  vm -> status |= data_intval(instr -> value);
  return NULL;
}

data_t * _instruction_execute_Jump(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  assert(instr -> name);
  return data_create(String, instr -> name);
}

data_t * _instruction_execute_EndLoop(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  vm -> status &= ~(VMStatusBreak | VMStatusContinue);
  return (vm -> status != VMStatusBreak) ? _instruction_call_Jump(instr, scope, vm, bytecode) : NULL;
}

/**
 * Executes a Test instruction. Pops the top entry off the VM stack and casts
 * it to the Bool data type. If this casted value is equal to bool:False, a jump
 * to the instruction's label field is indicated. If the popped value cannot be
 * converted to Bool, an exception is thrown. If the casted value is bool:True,
 * no jump is indicated.
 */
data_t * _instruction_execute_Test(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *ret;
  data_t *value;
  data_t *casted = NULL;

  value = vm_pop(vm);
  assert(value);
  assert(instr -> name);

  casted = data_cast(value, Bool);
  if (!casted) {
    ret = data_exception(ErrorType, "Cannot convert %s '%s' to boolean",
                         data_typedescr(value) -> type_name,
                         data_tostring(value));
  } else {
    ret = (!data_intval(casted)) ? data_create(String, instr -> name) : NULL;
  }
  data_free(casted);
  data_free(value);
  return ret;
}

data_t * _instruction_execute_Iter(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *value;
  data_t *iter;
  data_t *ret = NULL;

  value = vm_pop(vm);
  iter = data_iter(value);
  if (data_is_exception(iter)) {
    ret = iter;
  } else {
    vm_push(vm, iter);
  }
  data_free(value);
  return ret;
}

data_t * _instruction_execute_Next(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t  *ret = NULL;
  data_t  *iter;
  data_t  *next;

  iter = vm_pop(vm);
  assert(iter);
  assert(instr -> name);

  next = data_next(iter);
  if (data_is_exception(next) && (data_as_exception(next) -> code == ErrorExhausted)) {
    data_free(iter);
    ret = data_create(String, instr -> name);
  } else {
    vm_push(vm, iter);
    vm_push(vm, next);
  }
  return ret;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_Nop(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  //if (instr -> value) {
  //  vm_set_location(closure, instr -> value);
  //}
  return NULL;
}

data_t * _instruction_execute_Pop(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *value;

  value = vm_pop(vm);
  data_free(value);
  return NULL;
}

data_t * _instruction_execute_Dup(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  vm_push(vm, data_copy(vm_peek(vm)));
  return NULL;
}

data_t * _instruction_execute_Swap(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  data_t *v1;
  data_t *v2;

  v1 = vm_pop(vm);
  v2 = vm_pop(vm);
  vm_push(vm, v1);
  vm_push(vm, v2);
  return NULL;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_Stash(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  assert(data_intval(instr -> value) < NUM_STASHES);
  vm_stash(vm, data_intval(instr -> value), data_copy(vm_pop(vm)));
  return NULL;
}

data_t * _instruction_execute_Unstash(instruction_t *instr, data_t *scope, vm_t *vm, bytecode_t *bytecode) {
  assert(data_intval(instr -> value) < NUM_STASHES);
  vm_push(vm, data_copy(vm_unstash(vm, data_intval(instr -> value))));
  return NULL;
}

/* -- P U B L I C  F U N C T I O N S -------------------------------------- */

data_t * _instr_new(int type, va_list args) {
  instruction_t *ret;
  data_t        *data;
  char          *name = va_arg(args, char *);
  data_t        *value = va_arg(args, data_t *);

  ret = data_new(type, instruction_t);
  data = (data_t *) ret;
  ret -> line = -1;
  ret -> name = (name) ? strdup(name) : NULL;
  ret -> value = value;
  ret -> labels = strset_create();
  return data;
}

void _instr_free(instruction_t *instr) {
  free(instr -> name);
  data_free(instr -> value);
  set_free(instr -> labels);
  free(instr);
}

data_t *  instruction_create_enter_context(name_t *varname, data_t *catchpoint) {
  if (!varname) {
    varname = name_empty;
  }
  return data_create(ITEnterContext,
                     data_tostring(catchpoint),
                     data_create(Name, varname));
}

data_t * instruction_create_function(name_t *name, callflag_t flags,
                                     long num_args, array_t *kwargs) {
  data_t *ret;
  data_t *call;

  call = data_create(Call, flags, num_args, kwargs);
  ret = data_create(ITFunctionCall, name_last(name), call);
  return ret;
}

instruction_t * instruction_assign_label(instruction_t *instruction) {
  char *lbl = (char *) new(9);

  strrand(lbl, 8);
  set_add(instruction -> labels, lbl);
  return instruction;
}

instruction_t * instruction_set_label(instruction_t *instruction, data_t *label) {
  char *lbl = (char *) new(9);

  strncpy(lbl, data_tostring(label), 8);
  set_add(instruction -> labels, lbl);
  return instruction;
}
