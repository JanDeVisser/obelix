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

#include <config.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <data.h>
#include <datastack.h>
#include <dict.h>
#include <exception.h>
#include <str.h>
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

static inline void   _thread_init(void);
static void *        _thread_start_routine_wrapper(thread_ctx_t *);

extern void          _thread_free(thread_t *);
extern char *        _thread_tostring(thread_t *);
extern _thr_t        _thread_self(void);
static void          _thread_set_selfobj(thread_t *);
static thread_t *    _thread_get_selfobj();

static int           _data_cmp_thread(data_t *, data_t *);
static char *        _data_tostring_thread(data_t *);
static unsigned int  _data_hash_thread(data_t *);
static data_t *      _data_resolve_thread(data_t *, char *);

static data_t *      _thread_interrupt(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_yield(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_stack(data_t *, char *, array_t *, dict_t *);

extern data_t *      _thread_current_thread(char *, array_t *, dict_t *);

static vtable_t _vtable_thread[] = {
  { .id = FunctionCmp,      .fnc = (void_t) thread_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _thread_free },
  { .id = FunctionToString, .fnc = (void_t) _thread_tostring },
  { .id = FunctionHash,     .fnc = (void_t) thread_hash },
  { .id = FunctionResolve,  .fnc = (void_t) thread_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methoddescr_thread[] = {
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

int Thread = -1;
int thread_debug = 0;

/* ------------------------------------------------------------------------ */

void _thread_init(void) {
  thread_t *main;

  if (Thread < 0) {
    logging_register_category("thread", &thread_debug);
#ifdef HAVE_PTHREAD_H
    pthread_key_create(&self_obj, /* (void (*)(void *)) _thread_free */ NULL);
#elif defined(HAVE_CREATETHREAD)
    self_ix = TlsAlloc();
#endif /* HAVE_PTHREAD_H */
    Thread = typedescr_create_and_register(Thread, "thread",
                                           _vtable_thread,
                                           _methoddescr_thread);
    main = thread_self();
    if (main) {
      thread_setname(main, "main");
    }
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
  
#ifdef HAVE_PTHREAD_H
  if (!retval) {
    errno = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
    retval = (errno) ? -1 : 0;
  }
  if (!retval) {
    errno = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &dummy);
    retval = (errno) ? -1 : 0;
  }
#endif /* HAVE_PTHREAD_H */
  
  if (!retval) {
#ifdef HAVE_PTHREAD_H
    pthread_cleanup_push((void (*)(void *)) _thread_free, thread);
#endif /* HAVE_PTHREAD_H */
    ret = ctx -> start_routine(ctx -> arg);
    free(ctx);
#ifdef HAVE_PTHREAD_H
    pthread_cleanup_pop(1);
#endif /* HAVE_PTHREAD_H */
  } else {
    error("Error starting thread '%s': %s", ctx -> name, strerror(errno));
  }
  return ret;
}

void _thread_free(thread_t *thread) {
  if (thread) {
    thread_free(thread -> parent);
    data_free(thread -> kernel);
    data_free(thread -> exit_code);
    if (thread -> onfree) {
      thread -> onfree(thread -> stack);
    }
    free(thread -> name);
  }
}

char * _thread_tostring(thread_t *thread) {
  return thread -> name;
}

_thr_t _thread_self(void) {
#ifdef HAVE_PTHREAD_H
  return pthread_self();
#elif HAVE_CREATETHREAD
  return GetCurrentThread();
#endif
}

void _thread_set_selfobj(thread_t *thread) {
#ifdef HAVE_PTHREAD_H
  pthread_setspecific(self_obj, thread);
#elif HAVE_CREATETHREAD
  TlsSetValue(self_ix, thread);
#endif
}

thread_t * _thread_get_selfobj(void) {
#ifdef HAVE_PTHREAD_H
  return (thread_t *) pthread_getspecific(self_obj);
#elif HAVE_CREATETHREAD
  return (thread_t *) TlsGetValue(self_ix);
#endif
}

/* ------------------------------------------------------------------------ */

thread_t * thread_self(void) {
  thread_t *thread;
  
  _thread_init();
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
  _thr_t        thr_id;
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
#ifdef HAVE_PTHREAD_H
    errno = pthread_create(&thr_id, NULL,
                           (threadproc_t) _thread_start_routine_wrapper,
                           ctx);
    retval = (errno) ? -1 : 0;
#elif defined(HAVE_CREATETHREAD)
    thr_id = CreateThread(
    	NULL,                                         /* default security attributes   */
        0,                                            /* use default stack size        */
        (LPTHREAD_START_ROUTINE) _thread_start_routine_wrapper, /* thread function name          */
        ctx,                                          /* argument to thread function   */
        0,                                            /* use default creation flags    */
        NULL);                                        /* returns the thread identifier */
    if (!thr_id) {
      retval = -1;
      errno = GetLastError();
    }
#endif /* HAVE_PTHREAD_H */
  }
  if (!retval) {
    retval = condition_sleep(ctx -> condition);
  }
  if (!retval) {
    ret = ctx -> child;
  }
#ifdef HAVE_PTHREAD_H
  if (!retval) {
    errno = pthread_detach(thr_id);
  }
#endif /* HAVE_PTHREAD_H */
  if (!retval) {
    ret -> _errno = get_last_error();
  }
  return ret;
}

thread_t * thread_create(_thr_t thr_id, char *name) {
  thread_t *ret;
  char     *buf = NULL;

  _thread_init();
  ret = data_new(Thread, thread_t);
  ret -> thread = thr_id;
  ret -> exit_code = NULL;
  ret -> kernel = NULL;
  ret -> stack = NULL;
  ret -> onfree = NULL;
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
  return (thread) ? hash(&thread -> thread, sizeof(_thr_t)) : 0;
}

int thread_cmp(thread_t *t1, thread_t *t2) {
  return memcmp(&t1 -> thread, &t2 -> thread, sizeof(_thr_t));
}

int thread_interrupt(thread_t *thread) {
#ifdef HAVE_PTHREAD_H
  errno = pthread_cancel(thread -> thread);
#elif defined(HAVE_CREATETHREAD)
  if (TerminateThread(thread -> thread, 0)) {
  	errno = GetLastError();
  }
#endif /* HAVE_PTHREAD_H */
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
  free(thread -> name);
  thread -> name = strdup(name);
#if defined(HAVE_PTHREAD_H) && defined(HAVE_PTHREAD_SETNAME_NP)
  pthread_setname_np(thread -> thread, thread -> name);
#endif
  return thread;
}

data_t * thread_resolve(thread_t *thread, char *name) {
  data_t *ret = NULL;
  
  if (!strcmp(name, "name")) {
    return str_to_data(thread -> name);
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
  if (thread_debug) {
    debug("  Setting flag %d on thread %s", status, thread -> name);
  }
  thread -> status |= status;
  return thread -> status;
}

int thread_unset_status(thread_t *thread, thread_status_flag_t status) {
  if (thread_debug) {
    debug("  Clearing flag %d on thread %s", status, thread -> name);
  }
  thread -> status &= ~status;
  return thread -> status;
}

int thread_has_status (thread_t *thread, thread_status_flag_t status) {
  if (thread_debug) {
    debug("  Thread %s %s %d", 
          thread -> name, 
          thread -> status & status ? "has" : "doesn't have", 
          status);
  }
  return thread -> status & status;
}

int thread_status(thread_t *thread) {
  return thread -> status;
}

/* ------------------------------------------------------------------------ */

data_t * _thread_current_thread(char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  return data_current_thread();
}

data_t * _thread_interrupt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  if (thread_interrupt((thread_t *) self)) {
    return data_exception_from_errno();
  } else {
    return self;
  }
}

data_t * _thread_yield(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  if (thread_yield()) {
    return data_exception_from_errno();
  } else {
    return self;
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

  current = thread_self();
  if (!current -> stack) {
    current -> stack = datastack_create(current -> name);
    current -> onfree = (free_t) datastack_free;
  }
  return data_copy((data_t *) current);
}

data_t * data_thread_push_stackframe(data_t *frame) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);
  datastack_t *stack = thread -> stack;
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
    data = data_current_thread();
    thread = data;
  }
  thr = data_as_thread(data);
  stack = (datastack_t *) thr -> stack;
  ret =  data_create_list(stack -> list);
  data_free(data);
  return ret;
}

data_t * data_thread_set_kernel(data_t *kernel) {
  data_t      *data = data_current_thread();
  thread_t    *thread = data_as_thread(data);

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
