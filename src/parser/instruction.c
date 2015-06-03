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

int script_trace = 0;
extern int script_debug;

static void             _instruction_init(void) __attribute__((constructor(102)));

static data_t *         _instr_new(int, ...);
static void             _instr_free(instruction_t *);

static char *           _instruction_tostring_name(data_t *);
static char *           _instruction_tostring_value(data_t *);
static char *           _instruction_tostring_name_value(data_t *);
static char *           _instruction_tostring_value_or_name(data_t *);
static data_t *         _instruction_get_variable(instruction_t *, closure_t *);
static char *           _instruction_tostring(instruction_t *);

int Instruction = -1;

static vtable_t _vtable_instruction[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _instr_new },
  { .id = FunctionFree,     .fnc = (void_t) _instr_free },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_instruction = {
  .type =      Instruction,
  .type_name = "instruction",
  .vtable =    _vtable_instruction
};

#define data_is_instruction(d)  ((d) && data_hastype((d), Instruction))
#define data_instructionval(d)  (data_is_instruction((d)) ? ((instruction_t *) ((d) -> ptrval)) : NULL)

int ITByValue = -1;

static vtable_t _vtable_tostring_value[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_value },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_byvalue = {
  .type =      ITByValue,
  .type_name = "instruction_byvalue",
  .vtable =    _vtable_tostring_value
};

int ITByName = -1;

static vtable_t _vtable_tostring_name[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_name },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_byname = {
  .type =      ITByName,
  .type_name = "instruction_byname",
  .vtable =    _vtable_tostring_name
};

int ITByNameValue = -1;

static vtable_t _vtable_tostring_name_value[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_name_value },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_bynamevalue = {
  .type =      ITByNameValue,
  .type_name = "instruction_bynamevalue",
  .vtable =    _vtable_tostring_name_value
};

int ITByValueOrName = -1;

static vtable_t _vtable_tostring_value_or_name[] = {
  { .id = FunctionToString, .fnc = (void_t) _instruction_tostring_value_or_name },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_byvalue_or_name = {
  .type =      ITByValueOrName,
  .type_name = "instruction_byvalue_or_name",
  .vtable =    _vtable_tostring_value_or_name
};

#define InstructionType(t, s)                                              \
int             IT ## t = -1;                                              \
static data_t * _instruction_call_ ## t(data_t *, array_t *, dict_t *);    \
static data_t * _instruction_execute_ ## t(instruction_t *, data_t *);     \
static vtable_t _vtable_ ## t [] = {                                       \
  { .id = FunctionCall, .fnc = (void_t) _instruction_call_ ## t },         \
  { .id = FunctionNone, .fnc = NULL }                                      \
};                                                                         \
static typedescr_t _typedescr_ ## t = {                                    \
  .type =      IT ## t,                                                    \
  .type_name = #t,                                                         \
  .inherits  = { Instruction, ITBy ## s, NoType },                         \
  .vtable =    _vtable_ ## t                                               \
}                                                                          \
data_t * _instruction_call_ ## t(data_t *data, array_t *pars, dict_t *kwpars) { \
  instruction_t *instr = data_instructionval(data);                        \
  data_t        *scope = data_array_get(pars, 0);                          \
  if (script_debug) {                                                      \
    debug("Executing %s", instruction_tostring(instr));                    \
  }                                                                        \
  if (script_trace) {                                                      \
    fprintf(stderr, "%-60.60s%s\n",                                        \
            instruction_tostring(instr), data_tostring(scope));            \
  }                                                                        \
  return _instruction_execute_ ## t(instr, scope);                         \
}

InstructionType(Assign,       Value);
InstructionType(Decr,         Name);
InstructionType(Dup,          Name);
InstructionType(EnterContext, NameValue);
InstructionType(FunctionCall, Value);
InstructionType(Import,       Value);
InstructionType(Incr,         Name);
InstructionType(Iter,         Name);
InstructionType(Jump,         Name);
InstructionType(LeaveContext, Value);
InstructionType(Next,         Name);
InstructionType(Nop,          ValueOrName);
InstructionType(Pop,          Name);
InstructionType(PushCtx,      Name);
InstructionType(PushVal,      Value);
InstructionType(PushVar,      Value);
InstructionType(Return,       Name);
InstructionType(Stash,        Value);
InstructionType(Subscript,    Name);
InstructionType(Swap,         Name);
InstructionType(Test,         Name);
InstructionType(Throw,        Name);
InstructionType(Unstash,      Value);

typedef struct _function_call {
  data_t      data;
  name_t     *name;
  callflag_t  flags;
  int         arg_count;
  array_t    *kwargs;
  char       *str;
} function_call_t;

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

static name_t *name_empty = NULL;
static name_t *name_self = NULL;

/* ----------------------------------------------------------------------- */

void _instruction_init(void) {
  int         ix;
  typedescr_t type;
  
  logging_register_category("trace", &script_trace);

  Instruction = typedescr_register(&_typedescr_instruction);
  interface_register(Scope, "scope",        
                     4, FunctionPush, FunctionPop, FunctionResolve, FunctionSet);
  // ...
  Call = typedescr_register(&_typedescr_call);
  name_empty = name_create(0);
  name_self = name_create(1, "self");
}

/* ----------------------------------------------------------------------- */

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
  call -> str = NULL;
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
  function_call_t *call = (function_call_t *) d -> ptrval;
  
  asprintf(&call -> data.str, "%s(argv[%d], %s)", 
           name_tostring(call -> name),
           call -> arg_count, 
           (call -> kwargs) ? array_tostring(call -> kwargs) : "");
  return NULL;
}

/* -- T O _ S T R I N G  F U N C T I O N S -------------------------------- */

char * _instruction_name(data_t *data) {
  instruction_t *instruction = data_instructionval(data);

  return instruction -> name ? instruction -> name : "";
}

char * _instruction_tostring_name(data_t *data) {
  instruction_t *instruction = data_instructionval(data);

  _instruction_tostring(instruction, _instruction_name(data));
  return NULL;
}

char * _instruction_tostring_value(data_t *data) {
  instruction_t *instruction = data_instructionval(data);
  
  _instruction_tostring(instruction, data_tostring(instruction -> value));
  return NULL;
}

char * _instruction_tostring_name_value(data_t *data) {
  instruction_t *instruction = data_instructionval(data);
  char *v = data_tostring(instruction -> value);
  char *s;
  char *free_me = NULL;

  if (v && strlen(v)) {
    asprintf(&free_me, "%s, %s", 
             v, _instruction_name(data));
    s = free_me;
  } else {
    s = _instruction_tostring_name(data);
  }
  _instruction_tostring(instruction, s);
  free(free_me);
  return NULL;
}

char * _instruction_tostring_value_or_name(data_t *data) {
  instruction_t *instruction = data_instructionval(data);

  if (instruction -> value) {
    _instruction_tostring_value(data);
  } else {
    _instruction_tostring_name(data);
  }
  return NULL;
}

char * _instruction_tostring(instruction_t *instruction, char *s) {
  char *lbl;
  char  line[7];
  
  if (instruction -> line > 0) {
    snprintf(line, 7, "%6d", instruction -> line);
  } else {
    line[0] = 0;
  }
  asprintf(&instruction -> data.str, "%-6s %-11.11s%-15.15s%-27.27s", 
           line,
           instruction -> label, 
           data_typedescr(&instruction -> data) -> type_name,
           s);
  return NULL;
}

/* -- H E L P E R  F U N C T I O N S -------------------------------------- */

data_t * _instruction_get_variable(instruction_t *instr, data_t *scope) {
  name_t *path = data_nameval(instr -> value);
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

data_t * _instruction_execute_Assign(instruction_t *instr, data_t *scope) {
  name_t *path = data_nameval(instr -> value);
  data_t *value;
  data_t *ret;

  value = data_pop(scope);
  assert(value);
  if (script_debug) {
    debug(" -- value '%s'", data_tostring(value));
  }
  ret = data_set(scope, path, value);
  data_free(value);
  return (data_is_unhandled_exception(ret)) ? ret : NULL;
}

data_t * _instruction_execute_PushVar(instruction_t *instr, data_t *scope) {
  data_t *value;
  data_t *ret;
  name_t *path = data_nameval(instr -> value);
  
  value = _instruction_get_variable(instr, scope);
  if (data_is_unhandled_exception(value)) {
    ret = value;
  } else {
    if (script_debug) {
      debug(" -- value '%s'", data_tostring(value));
    }
    data_push(scope, value);
    ret = NULL;
  }
  return ret;
}

data_t * _instruction_execute_Subscript(instruction_t *instr, data_t *scope) {
  data_t *subscript = data_pop(scope);
  data_t *subscripted  = data_pop(scope);
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
    data_push(scope, slice);
  }
  name_free(name);
  data_free(subscript);
  data_free(subscripted);
  return NULL;
}

/* -- E X C E P T I O N  H A N D L I N G ---------------------------------- */

data_t * _instruction_execute_EnterContext(instruction_t *instr, data_t *scope) {
  data_t    *context;
  data_t    *ret = NULL;
  data_t * (*fnc)(data_t *);
  data_t    *catchpoint;
  nvp_t     *nvp_cp;
  
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
    catchpoint = data_create(NVP, data_create(String, instr -> name), data_copy(context));
    datastack_push(closure -> catchpoints, catchpoint);
  }
  data_free(context);
  return ret;
}

data_t * _instruction_execute_LeaveContext(instruction_t *instr, data_t *scope) {
  data_t      *error;
  exception_t *e = NULL;
  data_t      *context;
  data_t      *param;
  data_t      *ret = NULL;
  data_t *   (*fnc)(data_t *, data_t *);
  nvp_t       *cp;
  data_t      *cp_data;
  
  error = data_pop(scope);
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
  } else {
    if (context && data_hastype(context, CtxHandler)) {
      fnc = (data_t * (*)(data_t *, data_t *)) data_get_function(context, FunctionLeave);
      if (fnc) {
        if (e && (e -> code != ErrorLeave) 
              && (e -> code != ErrorReturn) 
              && (e -> code != ErrorExit)) {
          param = data_copy(error);
        } else {
          param = data_false();
        }
        ret = fnc(context, param);
      }
    } else {
      data_push(scope, data_copy(error));
    }
  }
  if (e && (e -> code == ErrorExit) && (e -> code == ErrorReturn)) {
    /*
    * If the error is ErrorExit or ErrorReturn  needs to be bubbled up, and we 
    * really don't care what else happens.
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

data_t * _instruction_execute_Throw(instruction_t *instr, data_t *scope) {
  data_t *exception = data_pop(scope);

  return !data_is_exception(exception) ? data_throwable(exception) : exception;
}

data_t * _instruction_execute_Return(instruction_t *instr, data_t *scope) {
  data_t      *retval = data_pop(scope);
  exception_t *ex;
  
  ex = exception_create(ErrorReturn, "Return Value");
  ex -> throwable = retval;
  return data_create(Exception, ex);
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_PushCtx(instruction_t *instr, data_t *scope) {
  data_t *cp_data;
  data_t *context;
  nvp_t  *cp;
  
  if (datastack_depth(closure -> catchpoints)) {
    cp_data = datastack_peek(closure -> catchpoints);
    cp = data_nvpval(cp_data);
    context = data_copy(cp -> value);
    data_push(scope, context);
    return NULL;
  } else {
    return data_exception(ErrorInternalError,
                          "%s: No context set",
                          data_tostring(&instr -> data));
  }
}

data_t * _instruction_execute_PushVal(instruction_t *instr, data_t *scope) {
  assert(instr -> value);
  data_push(scope, instr -> value);
  return NULL;
}

name_t * _instruction_setup_constructor(data_t *scope, 
                                        function_call_t *constructor) {
  data_t         *self;
  object_t       *obj;
  data_t         *s = NULL;
  script_t       *script;
  data_t         *dscript;
  bound_method_t *bm = NULL;
  char           *name;
  char           *ptr;
  name_t         *ret = NULL;
  
  self = data_get(scope, name_self);
  name_free(name_self);
  if (data_is_object(self)) {
    obj = data_objectval(self);
    s = data_resolve(scope, constructor -> name);
    if (data_is_script(s)) {
      script = data_scriptval(s);
    } else if (data_is_boundmethod(s)) {
      script = data_boundmethodval(s) -> script;
    } else if (data_is_closure(s)) {
      script = data_closureval(s) -> script;
    }
    if (script) {
      asprintf(&name, "$%s", name_tostring(constructor -> name));
      for (ptr = strchr(name, '.'); ptr; ptr = strchr(ptr + 1, '.')) {
        *ptr = '_';
      }
      ret = name_create(1, name);
      free(name);
      data_set(scope, ret, 
               data_create(BoundMethod, script_bind(script, obj)));
      object_bind_all(self, dscript = data_create(Script, script));
      data_free(dscript);
    }
  }
  return ret;
}

data_t * _instruction_execute_FunctionCall(instruction_t *instr, data_t *scope) {
  function_call_t *call = (function_call_t *) instr -> value -> ptrval;
  data_t          *value;
  data_t          *ret = NULL;
  data_t          *self;
  data_t          *arg_name;
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
      value = data_pop(scope);
      assert(value);
      arg_name = data_array_get(call -> kwargs, num - ix - 1);
      dict_put(kwargs, strdup(data_tostring(arg_name)), value);
    }
  }

  if (call -> flags & CFVarargs) {
    value = data_pop(scope);
    num = data_intval(value);
    data_free(value);    
  } else {
    num = call -> arg_count;
  }
  if (script_debug) {
    debug(" -- #arguments: %d", num);
  }
  if (num) {
    args = data_array_create(num);
    for (ix = 0; ix < num; ix++) {
      value = data_pop(scope);
      assert(value);
      array_set(args, num - ix - 1, value);
    }
    if (script_debug) {
      array_debug(args, " -- arguments: %s");
    }
  }
  self = (call -> flags & CFInfix) ? NULL : scope;
  if (call -> flags & CFConstructor) {
    name = _instruction_setup_constructor(self, call);
  } else {
    name = name_copy(call -> name);
  }
  ret = data_invoke(self, name, args, kwargs);
  name_free(name);
  array_free(args);
  dict_free(kwargs);
  if (ret && !data_is_exception(ret)) {
    data_push(scope, ret);
    ret = NULL;
  }
  if (script_debug) {
    debug(" -- return value '%s'", data_tostring(ret));
  }
  return ret;
}

data_t * _instruction_execute_Decr(instruction_t *instr, data_t *scope) {
  data_t *value = data_pop(scope);
  
  data_push(scope, data_create(Int, data_intval(value) - 1));
  data_free(value);
  return NULL;
}

data_t * _instruction_execute_Incr(instruction_t *instr, data_t *scope) {
  data_t *value = data_pop(scope);
  
  data_push(scope, data_create(Int, data_intval(value) + 1));
  data_free(value);
  return NULL;
}

/* -- F L O W  C O N T R O L ---------------------------------------------- */

data_t * _instruction_execute_Jump(instruction_t *instr, data_t *scope) {
  assert(instr -> name);
  return data_create(String, instr -> name);
}

data_t * _instruction_execute_Test(instruction_t *instr, data_t *scope) {
  data_t *ret;
  data_t *value;
  data_t *casted = NULL;
 
  value = data_pop(scope);
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

data_t * _instruction_execute_Iter(instruction_t *instr, data_t *scope) {
  data_t *value;
  data_t *iter;
  data_t *ret = NULL;
  
  value = data_pop(scope);
  iter = data_iter(value);
  if (data_is_exception(iter)) {
    ret = iter;
  } else {
    data_push(scope, iter);
  }
  data_free(value);
  return ret;
}

data_t * _instruction_execute_Next(instruction_t *instr, data_t *scope) {
  data_t  *ret = NULL;
  data_t  *iter;
  data_t  *next;
 
  iter = data_pop(scope);
  assert(iter);
  assert(instr -> name);
  
  next = data_next(iter);
  if (data_is_exception(next) && (data_exceptionval(next) -> code == ErrorExhausted)) {
    data_free(iter);
    ret = data_create(String, instr -> name);
  } else {
    data_push(scope, iter);
    data_push(scope, next);
  }
  return ret;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_Nop(instruction_t *instr, data_t *scope) {
  if (instr -> value) {
    closure_set_location(closure, instr -> value);
  }
  return NULL;
}

data_t * _instruction_execute_Pop(instruction_t *instr, data_t *scope) {
  data_t *value;

  value = data_pop(scope);
  data_free(value);
  return NULL;
}

data_t * _instruction_execute_Dup(instruction_t *instr, data_t *scope) {
  data_t *value;
  
  value = data_pop(scope);
  data_push(scope, data_copy(value));
  data_push(scope, value);
  return NULL;
}

data_t * _instruction_execute_Swap(instruction_t *instr, data_t *scope) {
  data_t *v1;
  data_t *v2;

  v1 = data_pop(scope);
  v2 = data_pop(scope);
  data_push(scope, v1);
  data_push(scope, v2);
  return NULL;
}

/* ----------------------------------------------------------------------- */

data_t * _instruction_execute_Stash(instruction_t *instr, data_t *scope) {
  assert(data_intval(instr -> value) < NUM_STASHES);
  closure_stash(closure,
                data_intval(instr -> value),
                data_copy(data_pop(scope)));
  return NULL;
}

data_t * _instruction_execute_Unstash(instruction_t *instr, data_t *scope) {
  assert(data_intval(instr -> value) < NUM_STASHES);
  data_push(scope, 
            data_copy(closure_unstash(closure,
                                      data_intval(instr -> value))));
  return NULL;
}

/* -- P U B L I C  F U N C T I O N S -------------------------------------- */

data_t * _instr_new(int type, va_list args) {
  instruction_t *ret;
  data_t        *data;
  char          *name = va_arg(args, char *);
  data_t        *value = va_arg(args, data_t *);

  ret = NEW(instruction_t);
  data = &ret -> data;
  data_settype(data, type);
  data -> free_me = DontFreeData;
  data -> ptrval = ret;
  ret -> line = -1;
  ret -> name = (name) ? strdup(name) : NULL;
  ret -> value = value;
  memset(ret -> label, 0, 9);
  return data;
}

void _instr_free(instruction_t *instr) {
  free(instr -> name);
  data_free(instr -> value);
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
  instruction_t *ret;
  data_t        *call;

  call = data_create(Call, name, flags, num_args, kwargs);
  ret = data_create(ITFunctionCall, name_tostring(name), call);
  return ret;
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
