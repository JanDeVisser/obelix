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

#include <data.h>
#include <datastack.h>
#include <logging.h>
#include <namespace.h>
#include <stacktrace.h>
#include <str.h>
#include <thread.h>
#include <typedescr.h>
#include <wrapper.h>

static void     _stacktrace_init(void) __attribute__((constructor));

static vtable_t _wrapper_vtable_stackframe[] = {
  { .id = FunctionCopy,     .fnc = (void_t) stackframe_copy },
  { .id = FunctionCmp,      .fnc = (void_t) stackframe_cmp },
  { .id = FunctionFree,     .fnc = (void_t) stackframe_free },
  { .id = FunctionToString, .fnc = (void_t) stackframe_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

static vtable_t _wrapper_vtable_stacktrace[] = {
  { .id = FunctionCopy,     .fnc = (void_t) stacktrace_copy },
  { .id = FunctionCmp,      .fnc = (void_t) stacktrace_cmp },
  { .id = FunctionFree,     .fnc = (void_t) stacktrace_free },
  { .id = FunctionToString, .fnc = (void_t) stacktrace_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};

int Stackframe = -1;
int Stacktrace = -1;
int stacktrace_debug = 0;

/* ------------------------------------------------------------------------ */

void _stacktrace_init(void) {
  logging_register_category("stacktrace", &stacktrace_debug);
  Stackframe = wrapper_register(Stackframe, "stackframe", _wrapper_vtable_stackframe);
  Stacktrace = wrapper_register(Stacktrace, "stacktrace", _wrapper_vtable_stacktrace);
}

/* ------------------------------------------------------------------------ */

stackframe_t * stackframe_create(data_t *data) {
  stackframe_t *ret = NEW(stackframe_t);
  closure_t    *closure = data_closureval(data);
  
  ret -> funcname = strdup(closure_tostring(closure));
  ret -> source = strdup(data_tostring(closure -> script -> mod -> source));
  ret -> line = closure -> line;
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

void stackframe_free(stackframe_t *stackframe) {
  if (stackframe && (--stackframe -> refs <= 0)) {
    free(stackframe -> funcname);
    free(stackframe -> source);
    free(stackframe);
  }
}

stackframe_t * stackframe_copy(stackframe_t *stackframe) {
  if (stackframe) {
    stackframe -> refs++;
  }
  return stackframe;
}

int stackframe_cmp(stackframe_t *stackframe1, stackframe_t *stackframe2) {
  int ret = strcmp(stackframe1 -> funcname, stackframe2 -> funcname);
  
  if (!ret) {
    ret = stackframe1 -> line - stackframe2 -> line;
  }
  return ret;
}

char * stackframe_tostring(stackframe_t *stackframe) {
  if (!stackframe -> str) {
    asprintf(&stackframe -> str, "%-32.32s [%32s:%d]",
             stackframe -> funcname, stackframe -> source, stackframe -> line);
  }
  return stackframe -> str;
}

/* ------------------------------------------------------------------------ */

stacktrace_t * stacktrace_create(void) {
  stacktrace_t *ret = NEW(stacktrace_t);
  thread_t     *thread = data_threadval(data_current_thread());
  char         *stack_name;
  datastack_t  *stack;
  int           ix;
  stackframe_t *frame;
  
  asprintf(&stack_name, "Thread %s", thread_tostring(thread));
  ret -> stack = datastack_create(stack_name);
  free(stack_name);
  stack = thread -> stack;
  for (ix = datastack_depth(thread -> stack) - 1; ix >= 0; ix--) {
    frame = stackframe_create(data_array_get(stack -> list, ix));
    datastack_push(ret -> stack, data_create(Stackframe, frame));
  }
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

void stacktrace_free(stacktrace_t *stacktrace) {
  if (stacktrace && (--stacktrace -> refs <= 0)) {
    datastack_free(stacktrace -> stack);
    free(stacktrace -> str);
    free(stacktrace);
  }
}

stacktrace_t * stacktrace_copy(stacktrace_t *stacktrace) {
  if (stacktrace) {
    stacktrace -> refs++;
  }
  return stacktrace;
}

int stacktrace_cmp(stacktrace_t *stacktrace1, stacktrace_t *stacktrace2) {
  return datastack_cmp(stacktrace1 -> stack, stacktrace2 -> stack);
}

char * stacktrace_tostring(stacktrace_t *stacktrace) {
  str_t   *str;
  array_t *stack;
  int      ix;
  data_t  *frame;
  
  if (!stacktrace -> str) {
    str = str_create(74 * datastack_depth(stacktrace -> stack));
    stack = stacktrace -> stack -> list;
    for (ix = 0; ix < array_size(stack); ix++) {
      frame = (data_t *) array_get(stack, ix);
      str_append_chars(str, data_tostring(frame));
      str_append_chars(str, "\n");
    }
    stacktrace -> str = strdup(str_chars(str));
    str_free(str);
  }
  return stacktrace -> str;
}

stacktrace_t * stacktrace_push(stacktrace_t *trace, data_t * frame) {
  datastack_push(trace -> stack, frame);
  return trace;
}

/* ------------------------------------------------------------------------ */
