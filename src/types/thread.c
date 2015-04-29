/*
 * /obelix/src/types/thread.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <string.h>

#include <data.h>
#include <datastack.h>
#include <exception.h>
#include <thread.h>

#define MAX_STACKDEPTH      200

static void          _data_init_thread(void) __attribute__((constructor));
static data_t *      _data_new_thread(data_t *, va_list);
static int           _data_cmp_thread(data_t *, data_t *);
static char *        _data_tostring_thread(data_t *);
static unsigned int  _data_hash_thread(data_t *);
static data_t *      _data_resolve_thread(data_t *, char *);

static data_t *      _thread_current_thread(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_interrupt(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_yield(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_stack(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_thread[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_thread },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_thread },
  { .id = FunctionFree,     .fnc = (void_t) thread_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_thread },
  { .id = FunctionHash,     .fnc = (void_t) _data_hash_thread },
  { .id = FunctionResolve,  .fnc = (void_t) _data_resolve_thread },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t typedescr_thread = {
  .type       = Thread,
  .type_name  = "thread",
  .vtable     = _vtable_thread
};

static methoddescr_t _methoddescr_thread[] = {
  { .type = Any,    .name = "current_thread", .method = _thread_current_thread, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Thread, .name = "interrupt",      .method = _thread_interrupt,      .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Thread, .name = "yield",          .method = _thread_yield,          .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Thread, .name = "stack",          .method = _thread_stack,          .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,             .method = NULL,                   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

#define data_is_thread(d) ((d) && (data_type((d)) == Thread))
#define data_threadval(d) ((thread_t *) ((data_is_thread((d)) ? (d) -> ptrval : NULL)))

/* ------------------------------------------------------------------------ */

void _data_init_thread(void) {
  typedescr_register(&typedescr_thread);
  typedescr_register_methods(_methoddescr_thread);
}

data_t * _data_new_thread(data_t *data, va_list args) {
  char         *name = va_arg(args, char *);
  threadproc_t  handler = va_arg(args, threadproc_t);
  void         *context = va_arg(args, void *);
  thread_t     *thread = thread_new(name, handler, context);
  
  if (thread) {
    data -> ptrval = thread;
    return data;
  } else {
    return data_exception_from_errno();
  }
}

int _data_cmp_thread(data_t *d1, data_t *d2) {
  return thread_cmp(data_threadval(d1), data_threadval(d2));
}

char * _data_tostring_thread(data_t *data) {
  return thread_tostring(data_threadval(data));
}

unsigned int _data_hash_thread(data_t *data) {
  return thread_hash(data_threadval(data));
}

data_t * _data_resolve_thread(data_t *self, char *name) {
  if (!strcmp(name, "name")) {
    return data_create(String, data_threadval(self) -> name);
  }
  return NULL;
}

/* ------------------------------------------------------------------------ */

data_t * _thread_current_thread(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) self;
  (void) name;
  (void) args;
  (void) kwargs;
  
  return data_current_thread();
}

data_t * _thread_interrupt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;
  
  thread_interrupt(data_threadval(self));
  return self;
}

data_t * _thread_yield(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;
  
  if (!thread_cmp(data_threadval(self), thread_self())) {
    thread_yield();
    return self;
  } else {
    return data_exception(ErrorType, "Can only call yield on current thread");
  }
}

data_t * _thread_stack(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) self;
  (void) name;
  (void) args;
  (void) kwargs;
  
  return data_thread_stacktrace(self);
}

/* ------------------------------------------------------------------------ */

data_t * data_current_thread(void) {
  thread_t *current;
  data_t   *data;
  
  current = thread_self();
  if (!current -> stack) {
    current -> stack = datastack_create(current -> name);
    current -> onfree = (free_t) datastack_free;
  }
  data = data_create_noinit(Thread);
  data -> ptrval = thread_copy(current);
  return data;
}

data_t * data_thread_frame_element(data_t *element) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_threadval(data);
  datastack_t *stack = thread -> stack;
  data_t      *ret;
  
  if (datastack_depth(stack) > MAX_STACKDEPTH) {
    ret =  data_exception(ErrorMaxStackDepthExceeded, 
                      "Maximum stack depth (%d) exceeded, most likely due to infinite recursion",
                      MAX_STACKDEPTH);
  } else {
    datastack_push((datastack_t *) thread -> stack, element);
    data_free(data);
    ret = element;
  }
  return ret;
}

data_t * data_thread_stacktrace(data_t *thread) {
  data_t      *data = NULL;
  thread_t    *thr;
  datastack_t *stack;
  data_t      *ret;
  
  if (!thread) {
    data = data_current_thread();
    thread = data;
  }
  thr = data_threadval(data);
  stack = (datastack_t *) thr -> stack;
  ret =  data_create_list(stack -> list);
  data_free(data);
  return ret;
}
