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

#include <typedescr.h>
#include <vm.h>
#include <wrapper.h>

static void     _vm_init(void) __attribute__((constructor));

static vtable_t _wrapper_vtable_vm[] = {
  { .id = FunctionFactory,  .fnc = (void_t) vm_create },
  { .id = FunctionCopy,     .fnc = (void_t) vm_copy },
  { .id = FunctionFree,     .fnc = (void_t) vm_free },
  { .id = FunctionToString, .fnc = (void_t) vm_tostring },
  /* No cmp function. All vm-s are different */
  { .id = FunctionNone,     .fnc = NULL }
};

int VM = -1;
int vm_debug = 0;

/* ------------------------------------------------------------------------ */

void _vm_init(void) {
  logging_register_category("vm", &vm_debug);
  VM = wrapper_register(VM, "vm", _wrapper_vtable_vm);
}

/* ------------------------------------------------------------------------ */

listnode_t * _vm_execute_instruction(instruction_t *instr, closure_t *closure) {
  data_t       *ret;
  char         *label = NULL;
  listnode_t   *node = NULL;
  data_t       *catchpoint = NULL;
  int           datatype;
  exception_t  *ex;
  data_t       *ex_data;
  
  ret = ns_exit_code(closure -> script -> mod -> ns);
  
  /*
   * If we're exiting, we still need to unwind the context stack, but no other
   * instructions are executed.
   * 
   * FIXME: What we're effectively doing here is disable __exit__ handlers in
   * obelix objects, since they will pick up the exit code.
   */
  if (!ret || (instr -> type == ITLeaveContext)) {
    if (instr -> line > 0) {
      closure -> line = instr -> line;
    }
    ret = instruction_execute(instr, closure);
  }
  if (ret) {
    datatype = data_type(ret);
    if (datatype == String) {
      label = strdup(data_tostring(ret));
    } else {
      if (datatype == Exception) {
        ex = data_exceptionval(ret);
      } else {
        ex_data = data_exception(ErrorInternalError,
                                 "Instruction '%s' returned %s '%s'",
                                 instruction_tostring(instr),
                                 data_typedescr(ret) -> type_name,
                                 data_tostring(ret));
        ex = data_exceptionval(ex_data);
        ret = ex_data;
      }
      ex -> trace = data_create(Stacktrace, stacktrace_create());
      if (datastack_depth(closure -> catchpoints)) {
        catchpoint = datastack_peek(closure -> catchpoints);
        label = strdup(data_charval(data_nvpval(catchpoint) -> name));
      } else {
        /*
         * This is ugly. There should be a way to interrupt list_process.
         * What we do here is get the last node (which exists, because 
         * otherwise we wouldn't be here, processing a node), fondle that 
         * node to get it's next pointer (which points to the tail marker
         * node), and return that. 
         */
        node = list_tail_pointer(closure -> script -> instructions) -> next;
      }
      closure_push(closure, data_copy(ret));
    }
    data_free(ret);
  }
  if (label) {
    if (script_debug) {
      debug("  Jumping to '%s'", label);
    }
    node = (listnode_t *) dict_get(closure -> script -> labels, label);
    assert(node);
    free(label);
  }
  return node;
}



/* ------------------------------------------------------------------------ */

vm_t * vm_create(va_list args) {
  vm_t *ret = NEW(vm_t);
  
  ret -> bytecode = bytecode_copy(va_arg(args, bytecode_t *));
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

void vm_free(vm_t *vm) {
  if (vm && (--vm -> refs <= 0)) {
    /* Do not free bytecode. It is a straight copy from script */
    datastack_free(vm -> contexts);
    free(vm -> str);
    free(vm);
  }
}

vm_t * vm_copy(vm_t *vm) {
  if (vm) {
    vm -> refs++;
  }
  return vm;
}

char * vm_tostring(vm_t *vm) {
  if (!vm -> str) {
    
  }
  return vm -> str;
}

data_t * vm_pop(vm_t *vm) {
  return datastack_pop(vm -> stack);
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
  datastack_push(vm -> stack, data_copy(value));
  return value;
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
  data_t *data = data_create_noinit(NVP);
  
  data -> ptrval = ret;
  datastack_push(vm -> contexts, data);
  return ret;
}

nvp_t * vm_peek_context(vm_t *vm) {
  return data_nvpval(datastack_peek(vm -> contexts));
}

nvp_t * vm_pop_context(vm_t *vm) {
  return data_nvpval(datastack_pop(vm -> contexts));
}

data_t * vm_execute(vm_t *vm, array_t *args, dict_t *kwargs) {
  char *str;
  int   dbg = logging_status("script");
  
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
}

/* ------------------------------------------------------------------------ */
