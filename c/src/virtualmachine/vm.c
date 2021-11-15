/*
 * /obelix/src/parser/vm.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libvm.h"
#include <thread.h>

static void         _vm_init(void);
static vm_t *       _vm_new(vm_t *, va_list);
static void         _vm_free(vm_t *);
static char *       _vm_tostring(vm_t *);
static data_t *     _vm_call(vm_t *, array_t *, dict_t *);
static vm_t *       _vm_prepare(vm_t *, data_t *);
static listnode_t * _vm_execute_instruction(data_t *, arguments_t *);

int VM = -1;

static vtable_t _vtable_VM[] = {
  { .id = FunctionNew,      .fnc = (void_t) _vm_new },
  { .id = FunctionFree,     .fnc = (void_t) _vm_free },
  { .id = FunctionToString, .fnc = (void_t) _vm_tostring },
  { .id = FunctionCall,     .fnc = (void_t) _vm_call },
  { .id = FunctionNone,     .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

extern int vm_trace;

void _vm_init(void) {
  typedescr_register(VM, vm_t);
}

/* ------------------------------------------------------------------------ */

vm_t * _vm_new(vm_t *vm, va_list args) {
  bytecode_t *bytecode = va_arg(args, bytecode_t *);

  vm -> bytecode = bytecode_copy(bytecode);
  vm -> stack = NULL;
  vm -> contexts = NULL;
  vm -> processor = NULL;
  vm -> exception = NULL;
  return vm;
}

void _vm_free(vm_t *vm) {
  if (vm) {
    bytecode_free(vm -> bytecode);
    datastack_free(vm -> contexts);
    datastack_free(vm -> stack);
    data_free(vm -> exception);
  }
}

char * _vm_tostring(vm_t *vm) {
  return bytecode_tostring(vm -> bytecode);
}

data_t * _vm_call(vm_t *vm, array_t *args, dict_t *kwargs) {
  return vm_execute(vm, data_array_get(args, 0));
}

vm_t * _vm_prepare(vm_t *vm, data_t *scope) {
  char        *str;
  int          dbg = logging_status("script");
  arguments_t *args;

  if (!vm -> stack) {
    vm -> stack = datastack_create(
      (dbg) ? bytecode_tostring(vm -> bytecode) : "VM");
    datastack_set_debug(vm -> stack, dbg);

    if (dbg) {
      asprintf(&str, "%s contexts", vm_tostring(vm));
    } else {
      str = "Contexts";
    }
    vm -> contexts = datastack_create(str);
    if (dbg) {
      free(str);
    }
    datastack_set_debug(vm -> contexts, dbg);

    args = arguments_create_args(3, scope, vm, vm -> bytecode);
    vm -> processor = lp_create(vm -> bytecode -> instructions,
                                (reduce_t) _vm_execute_instruction,
                                args);
  }
  return vm;
}

vm_t * _vm_cleanup(vm_t *vm) {
  data_free(vm -> processor -> data);
  lp_free(vm -> processor);
  vm -> processor = NULL;
  datastack_free(vm -> stack);
  vm -> stack = NULL;
  datastack_free(vm -> contexts);
  vm -> contexts = NULL;
  return vm;
}

listnode_t * _vm_execute_instruction(data_t *instr, arguments_t *args) {
  data_t      *ret = NULL;
  data_t      *exit_code = NULL;
  int          call_me = FALSE;
  data_t      *label = NULL;
  listnode_t  *node = NULL;
  vm_t        *vm = (vm_t *) data_uncopy(arguments_get_arg(args, 1));
  bytecode_t  *bytecode = (bytecode_t *) data_uncopy(arguments_get_arg(args, 2));
  exception_t *ex = NULL;
  data_t      *ex_data;
  nvp_t       *catchpoint;
  debugcmd_t   debugcmd;

  if (vm -> status != VMStatusExit) {
    exit_code = data_thread_exit_code();
    if (exit_code) {
      vm -> status = VMStatusExit;
    }
  }

  switch (vm -> status) {
    case VMStatusExit:
      if (thread_has_status(thread_self(), TSFLeave) || (instr -> type == ITLeaveContext)) {
        call_me = TRUE;
      }
      break;
    case VMStatusContinue:
    case VMStatusBreak:
      call_me = (instr -> type == ITEndLoop) || (instr -> type == ITLeaveContext);
      break;
    default:
      call_me = TRUE;
      break;
  }

  if (call_me) {
    debugcmd = debugger_step_before(vm -> debugger, data_as_instruction(instr));
    if (debugcmd == DebugCmdHalt) {
      ret = data_exception(ErrorExit, "Cancelled by debugger");
    } else {
      ret = data_call(instr, args);
      debugger_step_after(vm -> debugger, data_as_instruction(instr), ret);
    }
  }

  if (!exit_code && ret) {
    if (data_type(ret) == String) {
      label = data_copy(ret);
    } else if (data_type(ret) == Exception) {
      ex =  exception_copy(data_as_exception(ret));
      if (ex -> code == ErrorExit) {
        data_thread_set_exit_code(data_copy(ret));
      }
    } else {
      ex_data = data_exception(ErrorInternalError,
                               "Instruction '%s' returned %s '%s'",
                               data_tostring(instr),
                               data_typename(ret),
                               data_tostring(ret));
      ex = data_as_exception(ex_data);
    }
    if (ex) {
      if ((ex -> code != ErrorYield) && (ex -> code != ErrorExit) && (ex -> code != ErrorReturn)) {
        ex -> trace = (data_t *) stacktrace_create();
      }
      instruction_trace(data_as_instruction(instr), 
                        "Throws %s", exception_tostring(ex));
      vm -> exception = (data_t *) ex;
      if (ex -> code != ErrorYield) {
        if (datastack_depth(vm -> contexts)) {
          catchpoint = (nvp_t *) datastack_peek(vm -> contexts);
          label = data_copy(catchpoint -> name);
        } else {
          node = ProcessEnd;
        }
      }
    }
  }
  data_free(ret);
  if (label) {
    node = (listnode_t *) dict_get(bytecode -> labels,
      (data_is_string(label)) 
        ? str_chars((str_t *) label) 
        : data_tostring(label));
    if (!node) {
      fatal("Label %s not found", data_tostring(label));
    }
    data_free(label);
  }
  return node;
}

/* ------------------------------------------------------------------------ */

vm_t * vm_create(bytecode_t *bytecode) {
  _vm_init();
  return (vm_t *) data_create(VM, bytecode);
}

data_t * vm_pop(vm_t *vm) {
  data_t *ret = datastack_pop(vm -> stack);

  debug(vm, "Popped", "%s", data_tostring(ret));
  return ret;
}

data_t * vm_peek(vm_t *vm) {
  return datastack_peek(vm -> stack);
}

/**
 * @brief Pushes a data value onto the closure run-time stack.
 *
 * @param closure The closure on whose stack the value should be
 * pushed.
 * @param value The data value to push onto the closure's stack.
 * @return closure_t* The same closure as the one passed in.
 */
data_t * vm_push(vm_t *vm, data_t *value) {
  debug(vm, "Pushing", "%s", data_tostring(value));
  datastack_push(vm -> stack, data_copy(value));
  return value;
}

vm_t * vm_dup(vm_t *vm) {
  datastack_push(vm -> stack, data_copy(datastack_peek(vm -> stack)));
  return vm;
}

data_t * vm_stash(vm_t *vm, unsigned int stash, data_t *data) {
  if (stash < NUM_STASHES) {
    vm -> stashes[stash] = data;
    return data;
  } else {
    return NULL;
  }
}

data_t * vm_unstash(vm_t *vm, unsigned int stash) {
  if (stash < NUM_STASHES) {
    return vm -> stashes[stash];
  } else {
    return NULL;
  }
}

nvp_t * vm_push_context(vm_t *vm, char *label, data_t *context) {
  data_t *name;
  nvp_t  *ret;

  name = str_to_data(label);
  ret = nvp_create(name, context);
  data_free(name);
  datastack_push(vm -> contexts, (data_t *) nvp_copy(ret));
  return ret;
}

nvp_t * vm_peek_context(vm_t *vm) {
  data_t *d;

  d = datastack_peek(vm -> contexts);
  return data_as_nvp(d);
}

nvp_t * vm_pop_context(vm_t *vm) {
  data_t *d;

  d = datastack_pop(vm -> contexts);
  return data_as_nvp(d);
}

data_t * vm_execute(vm_t *vm, data_t *scope) {
  data_t      *ret = NULL;
  exception_t *ex;

  _vm_prepare(vm, scope);
  ret = data_thread_push_stackframe((data_t *) vm);
  if (!data_is_exception(ret)) {
    ret = NULL;
    data_free(vm -> exception);
    vm -> exception = NULL;

    vm -> debugger = debugger_create(vm, scope);
    if (vm_trace) {
      vm -> debugger -> status = DebugStatusSingleStep;
    }
    debugger_start(vm -> debugger);
    while (lp_step(vm -> processor)) {
      if (vm -> exception) {
        ex = data_as_exception(vm -> exception);
        if (ex -> code == ErrorYield) {
          ret = data_copy(vm -> exception);
          break;
        }
      }
    }

    // ret is == NULL if the execution didn't yield.
    if (!ret) {
      if (vm -> exception) {
        ex = data_as_exception(vm -> exception);
        if (ex -> code == ErrorReturn) {
          ret = (ex -> throwable) ? data_copy(ex -> throwable) : int_to_data(0);
        } else if (ex -> code == ErrorYield) {
          ret = NULL;
        } else {
          ret = data_copy(vm -> exception);
        }
      } else {
        ret = (datastack_notempty(vm -> stack) ? vm_pop(vm) : data_null());
      }
    }
    debugger_exit(vm -> debugger, ret);
    debugger_free(vm -> debugger);
    data_thread_pop_stackframe();
  }
  _vm_cleanup(vm);
  return ret;
}

/* ------------------------------------------------------------------------ */
