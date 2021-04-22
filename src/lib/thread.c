/*
 * /obelix/src/lib/thread.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <data.h>
#include <datastack.h>
#include <mutex.h>
#include <thread.h>

#define MAX_STACKDEPTH      200

typedef struct _thread_ctx {
  char         *name;
  threadproc_t  start_routine;
  void         *arg;
  thread_t     *creator;
  thread_t     *child;
  condition_t  *condition;
} thread_ctx_t;

static void *        _thread_start_routine_wrapper(thread_ctx_t *);

static void *        _thread_reduce_children(thread_t *, reduce_t, void *);
static pthread_t     _thread_self(void);
static void          _thread_set_selfobj(thread_t *);
static thread_t *    _thread_get_selfobj();

static data_t *      _thread_interrupt(data_t *, char *, arguments_t *);
static data_t *      _thread_yield(data_t *, char *, arguments_t *);
static data_t *      _thread_stack(data_t *, char *, arguments_t *);

extern data_t *      _thread_current_thread(char *, arguments_t *);

static vtable_t _vtable_Thread[] = {
  { .id = FunctionCmp,      .fnc = (void_t) thread_cmp },
  { .id = FunctionHash,     .fnc = (void_t) thread_hash },
  { .id = FunctionResolve,  .fnc = (void_t) thread_resolve },
  { .id = FunctionReduce,   .fnc = (void_t) _thread_reduce_children },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methods_Thread[] = {
  { .type = -1,  .name = "interrupt",      .method = _thread_interrupt,      .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,  .name = "yield",          .method = _thread_yield,          .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,  .name = "stack",          .method = _thread_stack,          .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,          .method = NULL,                   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

#ifdef HAVE_PTHREAD_H
static pthread_key_t  self_obj;
#elif HAVE_CREATETHREAD
static DWORD          self_ix;
#endif /* HAVE_PTHREAD_H */

int thread_debug = 0;

/* ------------------------------------------------------------------------ */

void thread_init(void) {
  thread_t *mainthread;

  logging_register_category("thread", &thread_debug);
  pthread_key_create(&self_obj, /* (void (*)(void *)) _thread_free */ NULL);
  builtin_typedescr_register(Thread, "thread", thread_t);
  mainthread = thread_self();
  if (mainthread) {
    thread_setname(mainthread, "main");
  }
}

void * _thread_start_routine_wrapper(thread_ctx_t *ctx) {
  void         *ret = NULL;
  thread_t     *thread = thread_self();
  int           retval = -1;
  int           dummy = 0;

  if (ctx -> name) {
    thread_setname(thread, ctx -> name);
  }
  ctx -> child = thread_copy(thread);
  thread -> parent = thread_copy(ctx -> creator);
  retval = condition_acquire(ctx -> condition);
  if (!retval) {
    retval = condition_wakeup(ctx -> condition);
  }
  if (!retval) {
    condition_free(ctx -> condition);
  }

  if (!retval) {
    errno = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
    retval = (errno) ? -1 : 0;
  }
  if (!retval) {
    errno = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &dummy);
    retval = (errno) ? -1 : 0;
  }

  if (!retval) {
    ret = ctx -> start_routine(ctx -> arg);
    free(ctx);
  } else {
    error("Error starting thread '%s': %s", ctx -> name, strerror(errno));
  }
  return ret;
}

void * _thread_reduce_children(thread_t *thread, reduce_t reducer, void *ctx) {
  ctx = reducer(thread->exit_code, ctx);
  ctx = reducer(thread->kernel, ctx);
  ctx = reducer(thread->stack, ctx);
  ctx = reducer(thread->parent, ctx);
  ctx = reducer(thread->mutex, ctx);
  return ctx;
}

pthread_t _thread_self(void) {
  return pthread_self();
}

void _thread_set_selfobj(thread_t *thread) {
  pthread_setspecific(self_obj, thread);
}

thread_t * _thread_get_selfobj(void) {
  return (thread_t *) pthread_getspecific(self_obj);
}

/* ------------------------------------------------------------------------ */

thread_t * thread_self(void) {
  thread_t *thread;

  thread = _thread_get_selfobj();
  if (!thread) {
    thread = thread_create(_thread_self(), NULL);
    _thread_set_selfobj(thread);
  }
  return thread;
}

thread_t * thread_new(char *name, threadproc_t start_routine, void *arg) {
  thread_t     *self = thread_self();
  thread_ctx_t *ctx;
  pthread_t        thr_id;
  thread_t     *ret = NULL;
  int           retval;

  ctx = NEW(thread_ctx_t);
  ctx -> name = name;
  ctx -> start_routine = start_routine;
  ctx -> arg = arg;
  ctx -> creator = self;
  ctx -> child = NULL;

  ctx -> condition = condition_create();
  retval = condition_acquire(ctx -> condition);
  if (!retval) {
    errno = pthread_create(&thr_id, NULL,
                           (threadproc_t) _thread_start_routine_wrapper,
                           ctx);
    retval = (errno) ? -1 : 0;
  }
  if (!retval) {
    retval = condition_sleep(ctx -> condition);
    ret = ctx -> child;
    errno = pthread_detach(thr_id);
    ret -> _errno = get_last_error();
  }
  return ret;
}

thread_t * thread_create(pthread_t thr_id, char *name) {
  thread_t *ret;
  char     *buf = NULL;

  ret = data_new(Thread, thread_t);
  ret -> thread = thr_id;
  ret -> exit_code = NULL;
  ret -> kernel = NULL;
  ret -> stack = NULL;
  if (!name) {
    asprintf(&buf, "Thread %ld", (long) thr_id);
    name = buf;
  }
  thread_setname(ret, name);
  free(buf);
  ret -> mutex = mutex_create();
  return ret;
}

unsigned int thread_hash(thread_t *thread) {
  return (thread) ? hash(&thread -> thread, sizeof(pthread_t)) : 0;
}

int thread_cmp(thread_t *t1, thread_t *t2) {
  return memcmp(&t1 -> thread, &t2 -> thread, sizeof(pthread_t));
}

int thread_interrupt(thread_t *thread) {
  errno = pthread_cancel(thread -> thread);
  return (errno) ? -1 : 0;
}

int thread_yield(void) {
  errno = 0;
#if defined(HAVE_PTHREAD_H) && defined(HAVE_PTHREAD_YIELD)
  errno = pthread_yield();
#endif /* HAVE_PTHREAD_H */
  return (errno) ? -1 : 0;
}

thread_t * thread_setname(thread_t *thread, char *name) {
  assert(name);
  free(thread->_d.str);
  thread -> _d.str = strdup(name);
  data_set_string_semantics(thread, StrSemanticsStatic);
#if defined(HAVE_PTHREAD_H) && defined(HAVE_PTHREAD_SETNAME_NP)
#ifndef __APPLE__
  pthread_setname_np(thread -> thread, thread -> name);
#else
  pthread_setname_np(thread -> name);
#endif
#endif
  return thread;
}

data_t * thread_resolve(thread_t *thread, char *name) {
  data_t *ret = NULL;

  if (!strcmp(name, "name")) {
    return str_to_data(thread->_d.str);
  } else if (!strcmp(name, "id")) {
    return int_to_data((long) thread -> thread);
  } else if (!strcmp(name, "exit_code")) {
    while (!ret && thread) {
      ret = data_copy(thread -> exit_code);
      thread = thread -> parent;
    }
    return ret;
  }
  return NULL;
}

int thread_set_status(thread_t *thread, thread_status_flag_t status) {
  debug(thread, "  Setting flag %d on thread %s", status, thread->_d.str);
  thread -> status |= status;
  return thread -> status;
}

int thread_unset_status(thread_t *thread, thread_status_flag_t status) {
  debug(thread, "  Clearing flag %d on thread %s", status, thread->_d.str);
  thread -> status &= ~status;
  return thread -> status;
}

int thread_has_status (thread_t *thread, thread_status_flag_t status) {
  debug(thread, "  Thread %s %s %d",
        thread->_d.str,
        (thread -> status & status) ? "has" : "doesn't have",
        status);
  return thread -> status & status;
}

int thread_status(thread_t *thread) {
  return thread -> status;
}

/* ------------------------------------------------------------------------ */

data_t * _thread_current_thread(_unused_ char *name, _unused_ arguments_t *args) {
  return data_current_thread();
}

data_t * _thread_interrupt(data_t *self, _unused_ char *name, _unused_ arguments_t *args) {
  if (thread_interrupt((thread_t *) self)) {
    return data_exception_from_errno();
  } else {
    return self;
  }
}

data_t * _thread_yield(data_t *self, _unused_ char *name, arguments_t *args) {
  if (thread_yield()) {
    return data_exception_from_errno();
  } else {
    return self;
  }
}

data_t * _thread_stack(_unused_ data_t *self, _unused_ char *name, _unused_ arguments_t *args) {
  return data_thread_stacktrace(self);
}

/* ------------------------------------------------------------------------ */

data_t * data_current_thread(void) {
  thread_t *current;

  current = thread_self();
  if (!current -> stack) {
    current -> stack = datastack_create(current->_d.str);
  }
  return data_copy((data_t *) current);
}

data_t * data_thread_push_stackframe(data_t *frame) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);
  datastack_t *stack = thread->stack;
  data_t      *ret;

  if (datastack_depth(stack) > MAX_STACKDEPTH) {
    ret =  data_exception(ErrorMaxStackDepthExceeded,
                          "Maximum stack depth (%d) exceeded, most likely due to infinite recursion",
                          MAX_STACKDEPTH);
  } else {
    datastack_push((datastack_t *) thread -> stack, frame);
    ret = frame;
  }
  data_free(data);
  return ret;
}

data_t * data_thread_pop_stackframe(void) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);
  datastack_t *stack = thread -> stack;
  data_t      *ret;

  if (!datastack_depth(stack)) {
    ret =  data_exception(ErrorInternalError,
                          "Call stack empty?");
  } else {
    ret = datastack_pop((datastack_t *) thread -> stack);
  }
  data_free(data);
  return ret;
}

data_t * data_thread_stacktrace(data_t *thread) {
  data_t      *data = NULL;
  thread_t    *thr;
  datastack_t *stack;
  data_t      *ret;

  if (!thread) {
    thread = data_uncopy(data_current_thread());
  }
  thr = data_as_thread(thread);
  stack = (datastack_t *) thr -> stack;
  ret = (data_t *) datalist_create(stack -> list);
  return ret;
}

data_t * data_thread_set_kernel(data_t *kernel) {
  thread_t    *thread = data_as_thread(data_current_thread());

  data_free(thread -> kernel);
  thread -> kernel = data_copy(kernel);
  return kernel;
}

data_t * data_thread_kernel(void) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);
  data_t      *ret = NULL;

  while (!ret && thread) {
    ret = data_copy(thread -> kernel);
    thread = thread -> parent;
  }
  return ret;
}

data_t * data_thread_set_exit_code(data_t *code) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);

  while (thread) {
    thread -> exit_code = data_copy(code);
    thread = thread -> parent;
  }
  return code;
}

data_t *data_thread_exit_code(void) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);
  data_t      *ret = NULL;

  while (!ret && thread) {
    ret = data_copy(thread -> exit_code);
    thread = thread -> parent;
  }
  return ret;
}

void data_thread_clear_exit_code(void) {
  thread_t    *thread = data_as_thread(data_current_thread());

  while (thread) {
    data_free(thread -> exit_code);
    thread -> exit_code = NULL;;
    thread = thread -> parent;
  }
}
