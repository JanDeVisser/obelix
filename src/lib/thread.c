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

#include <config.h>
#include <data.h>
#include <datastack.h>
#include <exception.h>
#include <dict.h>
#include <thread.h>

#define MAX_STACKDEPTH      200

typedef struct _thread_ctx {
  char            *name;
  threadproc_t     start_routine;
  void            *arg;
  thread_t        *creator;
  thread_t        *child;
  pthread_cond_t   condition;
} thread_ctx_t;

static void          _init_thread(void) __attribute__((constructor(120)));
static int           _pthread_cmp(pthread_t *, pthread_t *);
static unsigned int  _pthread_hash(pthread_t *);
static void *        _thread_start_routine_wrapper(thread_ctx_t *);

extern void          _thread_free(thread_t *);
extern char *        _thread_tostring(thread_t *);

static int           _data_cmp_thread(data_t *, data_t *);
static char *        _data_tostring_thread(data_t *);
static unsigned int  _data_hash_thread(data_t *);
static data_t *      _data_resolve_thread(data_t *, char *);

static data_t *      _thread_current_thread(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_interrupt(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_yield(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_stack(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_thread[] = {
  { .id = FunctionCmp,      .fnc = (void_t) thread_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _thread_free },
  { .id = FunctionToString, .fnc = (void_t) _thread_tostring },
  { .id = FunctionHash,     .fnc = (void_t) thread_hash },
  { .id = FunctionResolve,  .fnc = (void_t) thread_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methoddescr_thread[] = {
  { .type = Any, .name = "current_thread", .method = _thread_current_thread, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,  .name = "interrupt",      .method = _thread_interrupt,      .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,  .name = "yield",          .method = _thread_yield,          .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,  .name = "stack",          .method = _thread_stack,          .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,          .method = NULL,                   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

static pthread_key_t  self_obj;

int Thread = -1;
int thread_debug = 0;

/* ------------------------------------------------------------------------ */

void _init_thread(void) {
  thread_t *main;

  logging_register_category("thread", &thread_debug);
  pthread_key_create(&self_obj, /* (void (*)(void *)) _thread_free */ NULL);
  Thread = typedescr_create_and_register(Thread, "thread",
					 _vtable_thread,
					 _methoddescr_thread);
  main = thread_create(pthread_self(), "Main");
}

int _pthread_cmp(pthread_t *t1, pthread_t *t2) {
  return (pthread_equal(*t1, *t2)) ? 0 : 1;
}

unsigned int _pthread_hash(pthread_t *t) {
  return hash(t, sizeof(pthread_t));
}

void * _thread_start_routine_wrapper(thread_ctx_t *ctx) {
  threadproc_t  start_routine;
  void         *ret;
  thread_t     *thread = thread_self();
  char          buf[81];
  int           dummy;

  if (ctx -> name) {
    thread_setname(thread, ctx -> name);
  }
  ctx -> child = thread_copy(thread);
  pthread_setspecific(self_obj, thread);
  thread -> parent = thread_copy(ctx -> creator);
  errno = pthread_mutex_lock(&ctx -> creator -> mutex);
  if (!errno) {
    errno = pthread_cond_signal(&ctx -> condition);
    pthread_mutex_unlock(&ctx -> creator -> mutex);
  }
  
  if (!errno) {
    errno = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
  }
  if (!errno) {
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &dummy);
  }
  
  if (!errno) {
    pthread_cleanup_push((void (*)(void *)) _thread_free, thread);
    ret = ctx -> start_routine(ctx -> arg);
    pthread_cleanup_pop(1);
  } else {
    error("Error starting thread '%s': %s", ctx -> name, strerror_r(errno, buf, 80));
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

/* ------------------------------------------------------------------------ */

thread_t * thread_self(void) {
  thread_t  *thread = (thread_t *) pthread_getspecific(self_obj);

  if (!thread) {
    thread = thread_create(pthread_self(), NULL);
    pthread_setspecific(self_obj, thread);
  }
  return thread;
}

thread_t * thread_new(char *name, threadproc_t start_routine, void *arg) {
  thread_t           *self = thread_self();
  thread_ctx_t        ctx;
  pthread_t           thr_id;
  thread_t           *ret = NULL;

  ctx.name = name;
  ctx.start_routine = start_routine;
  ctx.arg = arg;
  ctx.creator = self;
  ctx.child = NULL;

  pthread_cond_init(&ctx.condition, NULL);
  errno = pthread_mutex_lock(&self -> mutex);
  if (!errno) {
    errno = pthread_create(&thr_id, NULL,
                           (threadproc_t) _thread_start_routine_wrapper,
			   &ctx);
  }
  if (!errno) {
    pthread_cond_wait(&ctx.condition, &self -> mutex);
    ret = ctx.child;
  }
  pthread_cond_destroy(&ctx.condition);

  assert(pthread_equal(ret -> thr_id, thr_id));
  ret -> _errno = pthread_detach(thr_id);
  return ret;
}

thread_t * thread_create(pthread_t thr_id, char *name) {
  thread_t            *ret = data_new(Thread, thread_t);
  pthread_mutexattr_t  attr;
  char                *buf = NULL;

  ret -> thr_id = thr_id;
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
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  if (errno = pthread_mutex_init(&ret -> mutex, &attr)) {
    free(ret -> name);
    free(ret);
    return NULL;
  }
  return ret;
}

unsigned int thread_hash(thread_t *thread) {
  return (thread) ? _pthread_hash(&thread -> thr_id) : 0;
}

int thread_cmp(thread_t *t1, thread_t *t2) {
  return (_pthread_cmp(&t1 -> thr_id, &t2 -> thr_id));
}

int thread_interrupt(thread_t *thread) {
  errno = pthread_cancel(thread -> thr_id);
  return errno;
}

int thread_yield(void) {
  errno = pthread_yield();
  return errno;
}

thread_t * thread_setname(thread_t *thread, char *name) {
  assert(name);
  free(thread -> name);
  thread -> name = strdup(name);
#ifdef HAVE_PTHREAD_SETNAME_NP
  pthread_setname_np(thread -> thr_id, thread -> name);
#endif
  return thread;
}

data_t * thread_resolve(thread_t *thread, char *name) {
  if (!strcmp(name, "name")) {
    return data_create(String, thread -> name);
  } else if (!strcmp(name, "id")) {
    return data_create(Int, (long) thread -> thr_id);
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

  thread_interrupt((thread_t *) self);
  return self;
}

data_t * _thread_yield(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  if (!data_cmp(self, (data_t *) thread_self())) {
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
