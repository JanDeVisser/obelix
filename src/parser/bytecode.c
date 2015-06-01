/*
 * /obelix/src/parser/bytecode.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <stdarg.h>

#include <bytecode.h>
#include <instruction.h>
#include <vm.h>

static void         _bytecode_init(void) __attribute__((constructor));

static data_t *     _bytecode_create(int, va_list);
static bytecode_t * _bytecode_set_instructions(bytecode_t *, list_t *);
static void         _bytecode_list_block(list_t *);

static vtable_t _vtable_bytecode[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _bytecode_create },
  { .id = FunctionFree,     .fnc = (void_t) bytecode_free },
  { .id = FunctionToString, .fnc = (void_t) bytecode_tostring },
  /* No cmp function. All bytecodes are different */
  { .id = FunctionNone,     .fnc = NULL }
};

int Bytecode = -1;
int bytecode_debug = 0;

/* ------------------------------------------------------------------------ */

void _bytecode_init(void) {
  logging_register_category("bytecode", &bytecode_debug);
  Bytecode = typedescr_register(VM, "bytecode", _vtable_bytecode);
}

/* -- S T A T I C  F U N C T I O N S -------------------------------------- */

data_t * _bytecode_create(int type, va_list args) {
  bytecode_t *bytecode = va_arg(args, bytecode_t *);
  
  return &bytecode -> data;
}

bytecode_t * _bytecode_set_instructions(bytecode_t *bytecode, list_t *block) {
  if (!block) {
    block = bytecode -> main_block;
  }
  bytecode -> instructions = block;
  return bytecode;
}

void _bytecode_list_block(list_t *block) {
  instruction_t *instr;
  
  for (list_start(block); list_has_next(block); ) {
    instr = (instruction_t *) list_next(block);
    fprintf(stderr, "%s\n", instruction_tostring(instr));
  }
}

listnode_t * _bytecode_execute_instruction(instruction_t *instr, reduce_ctx *ctx) {
  data_t       *ret;
  char         *label = NULL;
  listnode_t   *node = NULL;
  vm_t         *vm = ctx -> data;
  data_t       *scope = ctx -> obj;
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


/* -- P U B L I C  F U N C T I O N S -------------------------------------- */

bytecode_t * bytecode_create(data_t *owner) {
  bytecode_t *ret;
  
  if (bytecode_debug) {
    debug("Creating bytecode for '%s'", data_tostring(owner));
  }
  ret = NEW(bytecode_t);
  data_settype(&ret -> data, Bytecode);
  ret -> data.ptrval = ret;
  ret -> owner = data_copy(owner);
  
  ret -> main_block = list_create();
  list_set_free(ret -> main_block, (free_t) instruction_free);
  list_set_tostring(ret -> main_block, (tostring_t) instruction_tostring);
  _bytecode_set_instructions(ret, NULL);
  ret -> deferred_blocks = datastack_create("deferred blocks");
  ret -> bookmarks = datastack_create("bookmarks");
  
  ret -> labels = strvoid_dict_create();
  ret -> pending_labels = datastack_create("pending labels");
  ret -> current_line = -1;
  
  return ret;
}

void bytecode_free(bytecode_t *bytecode) {
  if (bytecode) {
    list_free(bytecode -> main_block);
    datastack_free(bytecode -> deferred_blocks);
    datastack_free(bytecode -> pending_labels);
    datastack_free(bytecode -> bookmarks);
    free(bytecode);
  }
}

char * bytecode_tostring(bytecode_t *bytecode) {
  if (!bytecode -> data.str) {
    asprintf(&bytecode -> data.str, "Bytecode for %s",
             data_tostring(bytecode -> owner));
  }
  return NULL;
}

bytecode_t * bytecode_push_instruction(bytecode_t *bytecode, instruction_t *instruction) {
  listnode_t    *node;
  data_t        *label;
  int            line;
  instruction_t *last;
  
  last = list_tail(bytecode -> instructions);
  line = (last) ? last -> line : -1;
  if (bytecode -> current_line > line) {
    instruction -> line = bytecode -> current_line;
  }
  list_push(bytecode -> instructions, instruction);
  if (!datastack_empty(bytecode -> pending_labels)) {
    label = datastack_peek(bytecode -> pending_labels);
    instruction_set_label(instruction, label);
    node = list_tail_pointer(bytecode -> instructions);
    while (!datastack_empty(bytecode -> pending_labels)) {
      label = datastack_pop(bytecode -> pending_labels);
      dict_put(bytecode -> labels, strdup(data_charval(label)), node);
      data_free(label);
    }
  }
  return bytecode;
}

bytecode_t * bytecode_start_deferred_block(bytecode_t *bytecode) {
  list_t *block;
  
  block = list_create();
  list_set_free(block, (free_t) instruction_free);
  list_set_tostring(block, (tostring_t) instruction_tostring);
  _bytecode_set_instructions(bytecode, block);
  return bytecode;
}

bytecode_t * bytecode_end_deferred_block(bytecode_t *bytecode) {
  datastack_push(bytecode -> deferred_blocks, 
                 data_create(Pointer, sizeof(list_t), bytecode -> instructions));
  _bytecode_set_instructions(bytecode, NULL);
  return bytecode;
}

bytecode_t * bytecode_pop_deferred_block(bytecode_t *bytecode) {
  list_t        *block;
  data_t        *data;
  instruction_t *instr;
  
  data = datastack_pop(bytecode -> deferred_blocks);
  block = data_unwrap(data);
  list_join(bytecode -> instructions, block);
  data_free(data);
  return bytecode;
}

bytecode_t * bytecode_bookmark(bytecode_t *bytecode) {
  listnode_t *node = list_tail_pointer(bytecode -> instructions);
  data_t     *data = data_create(Pointer, sizeof(listnode_t), node);
  
  assert(data_unwrap(data) == node);
  datastack_push(bytecode -> bookmarks, data);
  return bytecode;
}

bytecode_t * bytecode_discard_bookmark(bytecode_t *bytecode) {
  datastack_pop(bytecode -> bookmarks);
  return bytecode;
}

bytecode_t * bytecode_defer_bookmarked_block(bytecode_t *bytecode) {
  listnode_t    *node;
  data_t        *data;
  list_t        *block = bytecode -> instructions;
  instruction_t *instr;
  
  data = datastack_pop(bytecode -> bookmarks);
  node = data_unwrap(data);
  bytecode_start_deferred_block(bytecode);
  if (node) {
    list_position(node);
    list_next(node -> list);
    block = list_split(block);
  }
  list_join(bytecode -> instructions, block);
  bytecode_end_deferred_block(bytecode);
  return bytecode;
}

void bytecode_list(bytecode_t *bytecode) {
  fprintf(stderr, "// ===============================================================\n");
  fprintf(stderr, "// Bytecode Listing - %s\n", bytecode_tostring(bytecode));
  fprintf(stderr, "// ---------------------------------------------------------------\n");
  fprintf(stderr, "// %-6s %-11.11s%-15.15s\n", "Line", "Label", "Instruction");
  fprintf(stderr, "// ---------------------------------------------------------------\n");
  _bytecode_list_block(bytecode -> instructions);
  fprintf(stderr, "// ===============================================================\n");
}

data_t * bytecode_execute(bytecode_t *bytecode, vm_t *vm, data_t *scope) {
  reduce_ctx ctx;
  
  ctx.data = vm;
  ctx.obj = scope;
  list_process(bytecode -> instructions,
               (reduce_t) _bytecode_execute_instruction,
               &ctx);
}
