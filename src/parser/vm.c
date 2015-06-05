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
#include <wrapper.h>

static void     _vm_init(void) __attribute__((constructor));
static data_t * _vm_create(int, va_list);
static void     _vm_free(vm_t *);
static char *   _vm_tostring(vm_t *);

int VM = -1;
int vm_debug = 0;

static vtable_t _vtable_vm[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _vm_create },
  { .id = FunctionFree,     .fnc = (void_t) _vm_free },
  { .id = FunctionToString, .fnc = (void_t) _vm_tostring },
  /* No cmp function. All vm-s are different */
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_vm = {
  .type = -1,
  .type_name = "vm",
  .vtable = _vtable_vm
};

/* ------------------------------------------------------------------------ */

void _vm_init(void) {
  logging_register_category("vm", &vm_debug);
  VM = typedescr_register(&_typedescr_vm);
}

/* ------------------------------------------------------------------------ */

data_t * _vm_create(int type, va_list args) {
  vm_t *vm = va_arg(args, vm_t *);
  
  return data_copy(&vm -> data);
}

void _vm_free(vm_t *vm) {
  if (vm) {
    bytecode_free(vm -> bytecode);
    datastack_free(vm -> contexts);
    datastack_free(vm -> stack);
    free(vm);
  }
}

char * _vm_tostring(vm_t *vm) {
  return bytecode_tostring(vm -> bytecode);
}

/* ------------------------------------------------------------------------ */

vm_t * vm_create(bytecode_t *bytecode) {
  vm_t *ret = NEW(vm_t);
  
  ret -> bytecode = bytecode_copy(bytecode);
  data_settype(&ret -> data, VM);
  ret -> data.ptrval = ret;
  return ret;
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

data_t * vm_execute(vm_t *vm, data_t *scope) {
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
  
  ret = data_thread_push_stackframe(&vm -> data);
  if (!data_is_exception(ret)) {
    bytecode_execute(vm -> bytecode, vm, scope);
    ret = (datastack_notempty(vm -> stack)) ? vm_pop(vm) : data_null();
    if (dbg) {
      debug("    Execution of %s done: %s", vm_tostring(vm), data_tostring(ret));
    }
  }
  data_thread_pop_stackframe();
  return ret;
}

/* ------------------------------------------------------------------------ */
