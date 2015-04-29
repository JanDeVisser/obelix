/*
 * /obelix/src/types/mutex.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <data.h>
#include <exception.h>

static void          _mutex_init(void) __attribute__((constructor));
static void          _mutex_free(pthread_mutex_t *);
static data_t *      _mutex_new(data_t *, va_list);
static int           _mutex_cmp(data_t *, data_t *);
static char *        _mutex_tostring(data_t *);
static unsigned int  _mutex_hash(data_t *);
static data_t *      _mutex_resolve(data_t *, char *);

static data_t *      _mutex_create(data_t *, char *, array_t *, dict_t *);
static data_t *      _mutex_lock(data_t *, char *, array_t *, dict_t *);
static data_t *      _mutex_trylock(data_t *, char *, array_t *, dict_t *);
static data_t *      _mutex_unlock(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_mutex[] = {
  { .id = FunctionNew,      .fnc = (void_t) _mutex_new },
  { .id = FunctionCmp,      .fnc = (void_t) _mutex_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _mutex_free },
  { .id = FunctionToString, .fnc = (void_t) _mutex_tostring },
  { .id = FunctionHash,     .fnc = (void_t) _mutex_hash },
  { .id = FunctionResolve,  .fnc = (void_t) _mutex_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t typedescr_mutex = {
  .type       = Mutex,
  .type_name  = "mutex",
  .vtable     = _vtable_mutex
};

static methoddescr_t _methoddescr_mutex[] = {
  { .type = Any,    .name = "mutex",   .method = _mutex_create,  .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Mutex,  .name = "lock",    .method = _mutex_lock,    .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Mutex,  .name = "trylock", .method = _mutex_trylock, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Mutex,  .name = "unlock",  .method = _mutex_unlock,  .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,           .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

#define data_is_mutex(d) ((d) && (data_type((d)) == Mutex))
#define data_mutexval(d) ((pthread_mutex_t *) ((data_is_mutex((d)) ? (d) -> ptrval : NULL)))

/* ------------------------------------------------------------------------ */

void _mutex_init(void) {
  typedescr_register(&typedescr_mutex);
  typedescr_register_methods(_methoddescr_mutex);
}

data_t * _mutex_new(data_t *data, va_list args) {
  pthread_mutexattr_t  attr;
  pthread_mutex_t     *mutex;
  data_t              *ret;
  
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  mutex = NEW(pthread_mutex_t);
  if (errno = pthread_mutex_init(mutex, &attr)) {
    ret = data_exception_from_errno();
  } else {
    data -> ptrval = mutex;
    ret = data;
  }
  pthread_mutexattr_destroy(&attr);
  return data;
}

void _mutex_free(pthread_mutex_t *mutex) {
  pthread_mutex_destroy(mutex);
  free(mutex);
}

int _mutex_cmp(data_t *d1, data_t *d2) {
  /*
   * Since the mutex is created in the initialization function, two mutexes
   * can only be identical if they live in the same data object. That case 
   * is already resolved in data_cmp, and therefore, if we get here, they must 
   * be different.
   */
  return 1;
}

char * _mutex_tostring(data_t *data) {
  return "mutex";
}

unsigned int _mutex_hash(data_t *data) {
  return hash(data_mutexval(data), sizeof(pthread_mutex_t));
}

data_t * _mutex_resolve(data_t *self, char *name) {
  return NULL;
}

/* ------------------------------------------------------------------------ */

data_t * _mutex_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Mutex);
}

data_t * _mutex_lock(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  pthread_mutex_t *mutex = data_mutexval(self);
  
  (void) name;
  (void) args;
  (void) kwargs;
  if (errno = pthread_mutex_lock(mutex)) {
    return data_exception_from_errno();
  } else {
    return data_create(Bool, TRUE);
  }
}

data_t * _mutex_trylock(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  pthread_mutex_t *mutex = data_mutexval(self);
  
  (void) name;
  (void) args;
  (void) kwargs;
  if (errno = pthread_mutex_lock(mutex)) {
    return (errno == EBUSY) ? data_create(Bool, FALSE) : data_exception_from_errno();
  } else {
    return data_create(Bool, TRUE);
  }
}

data_t * _mutex_unlock(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  pthread_mutex_t *mutex = data_mutexval(self);
  
  (void) name;
  (void) args;
  (void) kwargs;
  if (errno = pthread_mutex_unlock(mutex)) {
    return data_exception_from_errno();
  } else {
    return data_create(Bool, TRUE);
  }
}

