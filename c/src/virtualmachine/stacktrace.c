/*
 * /obelix/src/parser/stacktrace.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

static inline void    _stacktrace_init(void);

static stackframe_t * _stackframe_new(stackframe_t *, va_list);
static void           _stackframe_free(stackframe_t *);
static char *         _stackframe_allocstring(stackframe_t *);

static stacktrace_t * _stacktrace_new(stacktrace_t *, va_list);
static void           _stacktrace_free(stacktrace_t *);
static char *         _stacktrace_allocstring(stacktrace_t *);

static vtable_t _vtable_Stackframe[] = {
  { .id = FunctionNew,         .fnc = (void_t) _stackframe_new },
  { .id = FunctionCmp,         .fnc = (void_t) stackframe_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _stackframe_free },
  { .id = FunctionAllocString, .fnc = (void_t) _stackframe_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_Stacktrace[] = {
  { .id = FunctionNew,         .fnc = (void_t) _stacktrace_new },
  { .id = FunctionCmp,         .fnc = (void_t) stacktrace_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _stacktrace_free },
  { .id = FunctionAllocString, .fnc = (void_t) _stacktrace_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

int Stackframe = -1;
int Stacktrace = -1;
int stacktrace_debug = 0;

/* ------------------------------------------------------------------------ */

void _stacktrace_init(void) {
  if (Stackframe < 1) {
    logging_register_module(stacktrace);
    typedescr_register(Stackframe, stackframe_t);
    typedescr_register(Stacktrace, stacktrace_t);
  }
}

/* ------------------------------------------------------------------------ */

stackframe_t * stackframe_create(data_t *data) {
  _stacktrace_init();
  return (stackframe_t *) data_create(Stackframe, data);
}

stackframe_t * _stackframe_new(stackframe_t *stackframe, va_list args) {
  vm_t         *vm = va_arg(args, vm_t *);

  stackframe -> bytecode = bytecode_copy(vm -> bytecode);
  // stackframe -> funcname = strdup(data_tostring(vm -> bytecode -> owner));
  // stackframe -> source = strdup(data_tostring(vm -> bytecode -> owner));
  stackframe -> funcname = NULL;
  stackframe -> source = NULL;
  stackframe -> line = 0;
  return stackframe;
}

void _stackframe_free(stackframe_t *stackframe) {
  if (stackframe) {
    bytecode_free(stackframe -> bytecode);
    free(stackframe -> funcname);
    free(stackframe -> source);
  }
}

int stackframe_cmp(stackframe_t *stackframe1, stackframe_t *stackframe2) {
  int ret = strcmp(stackframe1 -> funcname, stackframe2 -> funcname);

  if (!ret) {
    ret = stackframe1 -> line - stackframe2 -> line;
  }
  return ret;
}

char * _stackframe_allocstring(stackframe_t *stackframe) {
  char *buf;

  stackframe -> funcname = strdup(data_tostring(stackframe -> bytecode -> owner));
  stackframe -> source = strdup(data_tostring(stackframe -> bytecode -> owner));
  asprintf(&buf, "%-32.32s [%32s:%d]",
	   stackframe -> funcname, stackframe -> source, stackframe -> line);
  return buf;
}

/* ------------------------------------------------------------------------ */

stacktrace_t * stacktrace_create(void) {
  _stacktrace_init();
  return (stacktrace_t *) data_create(Stacktrace);
}

stacktrace_t * _stacktrace_new(stacktrace_t *stacktrace, va_list args) {
  thread_t     *thread = data_as_thread(data_current_thread());
  char         *stack_name;
  datastack_t  *stack;
  int           ix;
  stackframe_t *frame;

  asprintf(&stack_name, "Thread %s", thread_tostring(thread));
  stacktrace -> stack = datastack_create(stack_name);
  free(stack_name);
  stack = thread -> stack;
  for (ix = datastack_depth(thread -> stack) - 1; ix >= 0; ix--) {
    frame = stackframe_create(data_array_get(stack -> list, ix));
    datastack_push(stacktrace -> stack, (data_t *) stackframe_copy(frame));
  }
  return stacktrace;
}

void _stacktrace_free(stacktrace_t *stacktrace) {
  if (stacktrace) {
    datastack_free(stacktrace -> stack);
  }
}

int stacktrace_cmp(stacktrace_t *stacktrace1, stacktrace_t *stacktrace2) {
  return datastack_cmp(stacktrace1 -> stack, stacktrace2 -> stack);
}

char * _stacktrace_allocstring(stacktrace_t *stacktrace) {
  str_t   *str;
  char    *buf;

  str = array_join(stacktrace -> stack -> list, "\n");
  buf = strdup(str_chars(str));
  str_free(str);

  return buf;
}

stacktrace_t * stacktrace_push(stacktrace_t *trace, data_t * frame) {
  datastack_push(trace -> stack, frame);
  return trace;
}

/* ------------------------------------------------------------------------ */
