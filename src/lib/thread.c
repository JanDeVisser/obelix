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
#include <pthread.h>
#include <string.h>
#include <time.h>

#include <dict.h>
#include <thread.h>

static void          _init_thread(void) __attribute__((constructor));
static int           _pthread_cmp(pthread_t *, pthread_t *);
static unsigned int  _pthread_hash(pthread_t *);
static void *        _thread_start_routine_wrapper(void **);

static dict_t       *_threads;

/* ------------------------------------------------------------------------ */

void _init_thread(void) {
  thread_t *main;
  
  _threads = dict_create((cmp_t) _pthread_cmp);
  dict_set_hash(_threads, (hash_t) _pthread_hash);
  dict_set_free_key(_threads, (free_t) free);
  dict_set_free_data(_threads, (free_t) thread_free);
  main = thread_create(pthread_self(), "Main");
}

int _pthread_cmp(pthread_t *t1, pthread_t *t2) {
  return (pthread_equal(*t1, *t2)) ? 0 : 1;
}

unsigned int _pthread_hash(pthread_t *t) {
  return hash(t, sizeof(pthread_t));
}

void * _thread_start_routine_wrapper(void **arg) {
  threadproc_t  start_routine;
  void         *ret; 
  thread_t     *thread = thread_self();
  pthread_t    *key;
  
  if (arg[0]) {
    thread_setname(thread, arg[0]);
  }
  key = NEW(pthread_t);
  memcpy(key, &thread -> thr_id, sizeof(pthread_t));
  dict_put(_threads, key, thread_copy(thread));
  arg[3] = thread;

  pthread_setcancelstate(thread -> thr_id, PTHREAD_CANCEL_ENABLE);
  pthread_setcanceltype(thread -> thr_id, PTHREAD_CANCEL_DEFERRED);
  pthread_cleanup_push(thread_free, thread);
  start_routine = (threadproc_t) arg[1];
  ret = start_routine(arg[2]);
  pthread_cleanup_pop(1);
  return ret;
}

/* ------------------------------------------------------------------------ */

thread_t * thread_self(void) {
  pthread_t  thr_id = pthread_self();
  thread_t  *thread = (thread_t *) dict_get(_threads, &thr_id);
  
  if (!thread) {
    thread = thread_create(thr_id, NULL);
  }
  return thread;
}

thread_t * thread_new(char *name, threadproc_t start_routine, void *arg) {
  void            *wrapper_args[4];
  pthread_t        thr_id;
  thread_t        *ret;
  struct timespec  ts;
  
  wrapper_args[0] = name;
  wrapper_args[1] = (void *) start_routine;
  wrapper_args[2] = arg;
  wrapper_args[3] = NULL;
  
  if (errno = pthread_create(&thr_id, NULL,
			     (threadproc_t) _thread_start_routine_wrapper, 
			     wrapper_args)) {
    return NULL;
  }
  while (!wrapper_args[3]) {
    ts.tv_sec = 0;
    ts.tv_nsec = (long) 1000000;
    nanosleep(&ts, &ts);
  }
  ret = (thread_t *) wrapper_args[3];
  assert(pthread_equal(ret -> thr_id, thr_id));
  if (errno = pthread_detach(thr_id)) {
    pthread_cancel(thr_id);
    return NULL;
  }
  return ret;
}

thread_t * thread_create(pthread_t thr_id, char *name) {
  thread_t  *ret = NEW(thread_t);

  ret -> thr_id = thr_id;
  if (name) {
    ret -> name = strdup(name);
  } else {
    asprintf(&ret -> name, "Thread %d", dict_size(_threads));    
  }
  ret -> refs = 1;
  return ret;
}

void thread_free(thread_t *thread) {
  if (thread && (--thread -> refs <= 0)) {
    dict_pop(_threads, &thread -> thr_id);
    free(thread -> name);
    free(thread);
  }
}

thread_t * thread_copy(thread_t *thread) {
  if (thread) {
    thread -> refs++;
  }
  return thread;
}

unsigned int thread_hash(thread_t *thread) {
  return (thread) ? _pthread_hash(&thread -> thr_id) : 0;
}

int thread_cmp(thread_t *t1, thread_t *t2) {
  return (_pthread_cmp(&t1 -> thr_id, &t2 -> thr_id));
}

char * thread_tostring(thread_t *thread) {
  return thread -> name;
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
#ifdef _GNU_SOURCE
  pthread_setname_np(thread -> thr_id, thread -> name);
#endif
  return thread;
}