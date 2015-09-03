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
#include <stdio.h>

#include <bytecode.h>
#include <exception.h>
#include <instruction.h>
#include <stacktrace.h>
#include <thread.h>
#include <vm.h>

static void         _bytecode_init(void) __attribute__((constructor));

static data_t *     _bytecode_create(int, va_list);
static void         _bytecode_free(bytecode_t *);
static char *       _bytecode_allocstring(bytecode_t *);

static bytecode_t * _bytecode_set_instructions(bytecode_t *, list_t *);
static void         _bytecode_list_block(list_t *);

int bytecode_debug = 0;
int Bytecode = -1;

static vtable_t _vtable_bytecode[] = {
  { .id = FunctionFree,        .fnc = (void_t) _bytecode_free },
  { .id = FunctionAllocString, .fnc = (void_t) _bytecode_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _bytecode_init(void) {
  logging_register_category("bytecode", &bytecode_debug);
  Bytecode = typedescr_create_and_register(Bytecode,
					   "bytecode",
					   _vtable_bytecode,
					   NULL);
}

/* -- S T A T I C  F U N C T I O N S -------------------------------------- */

void _bytecode_free(bytecode_t *bytecode) {
  if (bytecode) {
    list_free(bytecode -> main_block);
    datastack_free(bytecode -> deferred_blocks);
    datastack_free(bytecode -> pending_labels);
    datastack_free(bytecode -> bookmarks);
  }
}

char * _bytecode_allocstring(bytecode_t *bytecode) {
  char *buf;

  asprintf(&buf, "Bytecode for %s", data_tostring(bytecode -> owner));
  return buf;
}

bytecode_t * _bytecode_set_instructions(bytecode_t *bytecode, list_t *block) {
  if (!block) {
    block = bytecode -> main_block;
  }
  bytecode -> instructions = block;
  return bytecode;
}

void _bytecode_list_block(list_t *block) {
  data_t *instr;

  for (list_start(block); list_has_next(block); ) {
    instr = (data_t *) list_next(block);
    fprintf(stderr, "%s\n", data_tostring(instr));
  }
}

/* -- P U B L I C  F U N C T I O N S -------------------------------------- */

bytecode_t * bytecode_create(data_t *owner) {
  bytecode_t *ret;

  if (bytecode_debug) {
    debug("Creating bytecode for '%s'", data_tostring(owner));
  }
  ret = data_new(Bytecode, bytecode_t);
  ret -> owner = data_copy(owner);

  ret -> main_block = data_list_create();
  _bytecode_set_instructions(ret, NULL);
  ret -> deferred_blocks = datastack_create("deferred blocks");
  datastack_set_debug(ret -> deferred_blocks, bytecode_debug);
  ret -> bookmarks = datastack_create("bookmarks");
  datastack_set_debug(ret -> bookmarks, bytecode_debug);
  ret -> labels = strvoid_dict_create();
  ret -> pending_labels = datastack_create("pending labels");
  datastack_set_debug(ret -> pending_labels, bytecode_debug);
  ret -> current_line = -1;
  return ret;
}

bytecode_t * bytecode_push_instruction(bytecode_t *bytecode, data_t *instruction) {
  listnode_t    *node;
  data_t        *label;
  int            line;
  data_t        *last;
  instruction_t *instr = data_as_instruction(instruction);

  if (bytecode_debug) {
    warning("Instruction '%s'", data_tostring(instruction));
  }
  last = list_tail(bytecode -> instructions);
  line = (last) ? data_as_instruction(last) -> line : -1;
  if (bytecode -> current_line > line) {
    instr -> line = bytecode -> current_line;
  }
  list_push(bytecode -> instructions, instruction);
  if (!datastack_empty(bytecode -> pending_labels)) {
    node = list_tail_pointer(bytecode -> instructions);
    while (!datastack_empty(bytecode -> pending_labels)) {
      label = datastack_pop(bytecode -> pending_labels);
      instruction_set_label(instr, label);
      dict_put(bytecode -> labels, strdup(data_tostring(label)), node);
      data_free(label);
    }
  }
  return bytecode;
}

bytecode_t * bytecode_start_deferred_block(bytecode_t *bytecode) {
  list_t *block;

  if (bytecode_debug) {
    debug("Start deferred block");
  }
  block = data_list_create();
  _bytecode_set_instructions(bytecode, block);
  return bytecode;
}

bytecode_t * bytecode_end_deferred_block(bytecode_t *bytecode) {
  if (bytecode_debug) {
    debug("End deferred block");
  }
  datastack_push(bytecode -> deferred_blocks,
                 data_create(Pointer, sizeof(list_t), bytecode -> instructions));
  _bytecode_set_instructions(bytecode, NULL);
  return bytecode;
}

bytecode_t * bytecode_pop_deferred_block(bytecode_t *bytecode) {
  list_t        *block;
  data_t        *data;
  instruction_t *instr;

  if (bytecode_debug) {
    debug("Popping deferred block");
  }
  data = datastack_pop(bytecode -> deferred_blocks);
  block = data_unwrap(data);
  list_join(bytecode -> instructions, block);
  data_free(data);
  return bytecode;
}

bytecode_t * bytecode_bookmark(bytecode_t *bytecode) {
  listnode_t *node = list_tail_pointer(bytecode -> instructions);
  data_t     *data = data_create(Pointer, sizeof(listnode_t), node);

  if (bytecode_debug) {
    debug("Bookmarking block %p -> %p", data, node);
  }
  assert(data_unwrap(data) == node);
  datastack_push(bytecode -> bookmarks, data);
  return bytecode;
}

bytecode_t * bytecode_discard_bookmark(bytecode_t *bytecode) {
  if (bytecode_debug) {
    debug("Discard block bookmark");
  }
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
  if (bytecode_debug) {
    debug("Deferring bookmark block %p -> %p", data, node);
  }
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
  fprintf(stderr, "// ---------------------------------------------------------------\n");
}
