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

#include <stdio.h>

#include <closure.h>
#include <exception.h>
#include <stacktrace.h>
#include <typedescr.h>
#include <thread.h>
#include <vm.h>

static void     _vm_init(void) __attribute__((constructor));
static void     _vm_free(vm_t *);
static char *   _vm_tostring(vm_t *);
static data_t * _vm_call(vm_t *, array_t *, dict_t *);
static vm_t *   _vm_prepare(vm_t *, data_t *);

int VM = -1;

static vtable_t _vtable_vm[] = {
  { .id = FunctionFree,     .fnc = (void_t) _vm_free },
  { .id = FunctionToString, .fnc = (void_t) _vm_tostring },
  { .id = FunctionCall,     .fnc = (void_t) _vm_call },
  { .id = FunctionNone,     .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _vm_init(void) {
  VM = typedescr_create_and_register(VM, "vm", _vtable_vm, NULL);
}

extern int script_trace;

/* ------------------------------------------------------------------------ */

void _vm_free(vm_t *vm) {
  if (vm) {
    bytecode_free(vm -> bytecode);
    datastack_free(vm -> contexts);
    datastack_free(vm -> stack);
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
  data_t      *ret;
  exception_t *e;

  if (!vm -> stack) {
    asprintf(&str, "%s run-time stack", vm_tostring(vm));
    vm -> stack = datastack_create(vm_tostring(vm -> bytecode));
    free(str);
    datastack_set_debug(vm -> stack, dbg);
  } else {
    datastack_clear(vm -> stack);
  }

  if (!vm -> contexts) {
    asprintf(&str, "%s contexts", vm_tostring(vm));
    vm -> contexts = datastack_create(str);
    free(str);
    datastack_set_debug(vm -> contexts, dbg);
  } else {
    datastack_clear(vm -> contexts);
  }

  vm -> processor = NULL;
  return vm;
}

listnode_t * _vm_execute_instruction(data_t *instr, array_t *args) {
  data_t       *ret = NULL;
  data_t       *exit_code;
  char         *label = NULL;
  listnode_t   *node = NULL;
  data_t       *scope = data_array_get(args, 0);
  vm_t         *vm = data_as_vm(data_array_get(args, 1));
  bytecode_t   *bytecode = data_as_bytecode(data_array_get(args, 2));
  data_t       *catchpoint = NULL;
  int           datatype;
  exception_t  *ex = NULL;
  data_t       *ex_data;

  exit_code = data_thread_exit_code();

  /*
   * Execute the instruction if
   *  1. exit() has not been called OR
   *  2. A context manager's Leave function is being executed OR
   *  3. This instruction is a Leave instruction
   */
  if (!exit_code ||
    thread_has_status(thread_self(), TSFLeave) ||
    (instr -> type == ITLeaveContext)) {
    ret = data_call(instr, args, NULL);
  }

  if (!exit_code && ret) {
    if (data_type(ret) == String) {
      label = strdup(data_tostring(ret));
    } else if (data_type(ret) == Exception) {
      ex =  exception_copy(data_as_exception(ret));
      if (ex -> code == ErrorExit) {
        data_thread_set_exit_code(data_copy(ret));
      }
    } else {
      ex_data = data_exception(ErrorInternalError,
                                "Instruction '%s' returned %s '%s'",
                                data_tostring(instr),
                                data_typedescr(ret) -> type_name,
                                data_tostring(ret));
      ex = data_as_exception(ex_data);
    }
    data_free(ret);
    if (ex) {
      ex -> trace = data_create(Stacktrace, stacktrace_create());
      instruction_trace("Throws", "%s", exception_tostring(ex));
      vm -> exception = data_copy(ret);
      if (datastack_depth(vm -> contexts)) {
        catchpoint = datastack_peek(vm -> contexts);
        label = strdup(data_tostring(data_as_nvp(catchpoint) -> name));
      } else {
        node = ProcessEnd;
      }
    }
  }
  if (label) {
    if (script_debug) {
      debug("  Jumping to '%s'", label);
    }
    instruction_trace("Jump To", "%s", label);
    node = (listnode_t *) dict_get(bytecode -> labels, label);
    assert(node);
    free(label);
  }
  return node;
}

/* ------------------------------------------------------------------------ */

vm_t * vm_create(bytecode_t *bytecode) {
  vm_t *ret = data_new(VM, vm_t);

  ret -> bytecode = bytecode_copy(bytecode);
  return ret;
}

data_t * vm_pop(vm_t *vm) {
  data_t *ret = datastack_pop(vm -> stack);

  instruction_trace("Popped", "%s", data_tostring(ret));
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
  instruction_trace("Pushing", "%s", data_tostring(value));
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
  nvp_t *ret = nvp_create(data_create(String, label), data_copy(context));
  data_t *data = data_create(NVP, ret);

  datastack_push(vm -> contexts, data);
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
  int          dbg = logging_status("script");
  data_t      *ret;
  array_t     *args = data_array_create(3);
  exception_t *ex;

  ret = vm_initialize(vm, scope);
  if (!data_is_exception(ret)) {
    lp_run(vm -> processor);
    array_free(vm -> processor -> data);
    lp_free(vm -> processor);
    vm -> processor = NULL;
    if (vm -> exception) {
      ex = data_as_exception(vm -> exception);
      if (ex -> code == ErrorReturn) {
        ret = (ex -> throwable) ? data_copy(ex -> throwable) : data_create(Int, 0);
      } else {
        ret = data_copy(vm -> exception);
      }
    } else {
      ret = (datastack_notempty(vm -> stack) ? vm_pop(vm) : data_null());
    }
    if (dbg) {
      debug("    Execution of %s done: %s", vm_tostring(vm), data_tostring(ret));
    }
    data_thread_pop_stackframe();
  }
  return ret;
}

data_t * vm_initialize(vm_t *vm, data_t *scope) {
  int              dbg = logging_status("script");
  data_t          *ret;
  array_t         *args = data_array_create(3);
  exception_t     *ex;

  _vm_prepare(vm, scope);
  ret = data_thread_push_stackframe((data_t *) vm);
  if (!data_is_exception(ret)) {
    data_free(vm -> exception);
    vm -> exception = NULL;
    array_push(args, data_copy(scope));
    array_push(args, data_create(VM, vm));
    array_push(args, data_create(Bytecode, vm -> bytecode));

    vm -> processor = lp_create(vm -> bytecode -> instructions,
                                (reduce_t) _vm_execute_instruction,
                                args);
    ret = (data_t *) vm;
  }
  return ret;
}

/* ------------------------------------------------------------------------ */
