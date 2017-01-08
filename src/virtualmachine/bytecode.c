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

#include "libvm.h"
#include <thread.h>

static inline void  _bytecode_init(void);

static bytecode_t * _bytecode_new(bytecode_t *, va_list);
static void         _bytecode_free(bytecode_t *);
static char *       _bytecode_allocstring(bytecode_t *);

static bytecode_t * _bytecode_set_instructions(bytecode_t *, list_t *);
static void         _bytecode_list_block(list_t *);

int bytecode_debug = 0;
int Bytecode = -1;

static vtable_t _vtable_Bytecode[] = {
  { .id = FunctionNew,         .fnc = (void_t) _bytecode_new },
  { .id = FunctionFree,        .fnc = (void_t) _bytecode_free },
  { .id = FunctionAllocString, .fnc = (void_t) _bytecode_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _bytecode_init(void) {
  if (Bytecode < 1) {
    logging_register_module(bytecode);
    typedescr_register(Bytecode, bytecode_t);
  }
}

/* -- S T A T I C  F U N C T I O N S -------------------------------------- */

bytecode_t * _bytecode_new(bytecode_t *bytecode, va_list args) {
  data_t *owner = va_arg(args, data_t *);

  debug(bytecode, "Creating bytecode for '%s'", data_tostring(owner));
  bytecode -> owner = data_copy(owner);

  bytecode -> main_block = data_list_create();
  _bytecode_set_instructions(bytecode, NULL);
  bytecode -> deferred_blocks = datastack_create("deferred blocks");
  datastack_set_debug(bytecode -> deferred_blocks, bytecode_debug);
  bytecode -> bookmarks = datastack_create("bookmarks");
  datastack_set_debug(bytecode -> bookmarks, bytecode_debug);
  bytecode -> labels = strvoid_dict_create();
  bytecode -> pending_labels = datastack_create("pending labels");
  datastack_set_debug(bytecode -> pending_labels, bytecode_debug);
  bytecode -> current_line = -1;
  return bytecode;
}

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
  _bytecode_init();
  return (bytecode_t *) data_create(Bytecode, owner);
}

bytecode_t * bytecode_push_instruction(bytecode_t *bytecode, data_t *instruction) {
  listnode_t    *node;
  data_t        *label;
  int            line;
  data_t        *last;
  instruction_t *instr = data_as_instruction(instruction);

  assert(bytecode);
  assert(instr);
  if (bytecode_debug) {
    warn("Instruction '%s'", data_tostring(instruction));
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

  debug(bytecode, "Start deferred block");
  block = data_list_create();
  _bytecode_set_instructions(bytecode, block);
  return bytecode;
}

bytecode_t * bytecode_end_deferred_block(bytecode_t *bytecode) {
  debug(bytecode, "End deferred block");
  datastack_push(bytecode -> deferred_blocks,
                 ptr_to_data(sizeof(list_t), bytecode -> instructions));
  _bytecode_set_instructions(bytecode, NULL);
  return bytecode;
}

bytecode_t * bytecode_pop_deferred_block(bytecode_t *bytecode) {
  list_t        *block;
  data_t        *data;

  debug(bytecode, "Popping deferred block");
  data = datastack_pop(bytecode -> deferred_blocks);
  block = data_unwrap(data);
  list_join(bytecode -> instructions, block);
  data_free(data);
  return bytecode;
}

bytecode_t * bytecode_bookmark(bytecode_t *bytecode) {
  listnode_t *node = list_tail_pointer(bytecode -> instructions);
  data_t     *data = ptr_to_data(sizeof(listnode_t), node);

  debug(bytecode, "Bookmarking block %p -> %p", data, node);
  assert(data_unwrap(data) == node);
  datastack_push(bytecode -> bookmarks, data);
  return bytecode;
}

bytecode_t * bytecode_discard_bookmark(bytecode_t *bytecode) {
  debug(bytecode, "Discard block bookmark");
  datastack_pop(bytecode -> bookmarks);
  return bytecode;
}

bytecode_t * bytecode_defer_bookmarked_block(bytecode_t *bytecode) {
  listnode_t    *node;
  data_t        *data;
  list_t        *block = bytecode -> instructions;

  data = datastack_pop(bytecode -> bookmarks);
  node = data_unwrap(data);
  debug(bytecode, "Deferring bookmark block %p -> %p", data, node);
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
