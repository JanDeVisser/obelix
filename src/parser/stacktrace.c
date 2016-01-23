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

#include <config.h>
#include <stdio.h>

#include <closure.h>
#include <data.h>
#include <datastack.h>
#include <logging.h>
#include <namespace.h>
#include <stacktrace.h>
#include <str.h>
#include <thread.h>
#include <vm.h>

static inline void _stacktrace_init(void) __attribute__((constructor));

static void        _stackframe_free(stackframe_t *);
static char *      _stackframe_allocstring(stackframe_t *);
static void        _stacktrace_free(stacktrace_t *);
static char *      _stacktrace_allocstring(stacktrace_t *);

static vtable_t _vtable_stackframe[] = {
  { .id = FunctionCmp,         .fnc = (void_t) stackframe_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _stackframe_free },
  { .id = FunctionAllocString, .fnc = (void_t) _stackframe_allocstring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_stacktrace[] = {
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
  if (Stackframe < 0) {
    logging_register_category("stacktrace", &stacktrace_debug);
    Stackframe = typedescr_create_and_register(Stackframe, "stackframe",
                                               _vtable_stackframe, NULL);
    Stacktrace = typedescr_create_and_register(Stacktrace, "stacktrace",
                                               _vtable_stacktrace, NULL);
  }
}

/* ------------------------------------------------------------------------ */

stackframe_t * stackframe_create(data_t *data) {
  stackframe_t *ret;
  vm_t         *vm = data_as_vm(data);
  
  _stacktrace_init();
  ret = data_new(Stackframe, stackframe_t);
  ret -> funcname = strdup(data_tostring(vm -> bytecode -> owner));
  // FIXME
  ret -> source = strdup(data_tostring(vm -> bytecode -> owner));
  ret -> line = 0;
  return ret;
}

void _stackframe_free(stackframe_t *stackframe) {
  if (stackframe) {
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
  
  asprintf(&buf, "%-32.32s [%32s:%d]",
	   stackframe -> funcname, stackframe -> source, stackframe -> line);
  return buf;
}

/* ------------------------------------------------------------------------ */

stacktrace_t * stacktrace_create(void) {
  stacktrace_t *ret;
  thread_t     *thread = data_as_thread(data_current_thread());
  char         *stack_name;
  datastack_t  *stack;
  int           ix;
  stackframe_t *frame;
  
  _stacktrace_init();
  ret = data_new(Stacktrace, stacktrace_t);
  asprintf(&stack_name, "Thread %s", thread_tostring(thread));
  ret -> stack = datastack_create(stack_name);
  free(stack_name);
  stack = thread -> stack;
  for (ix = datastack_depth(thread -> stack) - 1; ix >= 0; ix--) {
    frame = stackframe_create(data_array_get(stack -> list, ix));
    datastack_push(ret -> stack, (data_t *) stackframe_copy(frame));
  }
  return ret;
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
  array_t *stack;
  int      ix;
  data_t  *frame;
  char    *buf;
  
  str = str_create(74 * datastack_depth(stacktrace -> stack));
  stack = stacktrace -> stack -> list;
  for (ix = 0; ix < array_size(stack); ix++) {
    if (ix > 0) {
      str_append_chars(str, "\n");
    }
    frame = (data_t *) array_get(stack, ix);
    str_append_chars(str, data_tostring(frame));
  }
  buf = strdup(str_chars(str));
  str_free(str);

  return buf;
}

stacktrace_t * stacktrace_push(stacktrace_t *trace, data_t * frame) {
  datastack_push(trace -> stack, frame);
  return trace;
}

/* ------------------------------------------------------------------------ */
