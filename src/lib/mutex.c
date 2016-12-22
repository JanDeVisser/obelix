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

#include "libcore.h"
#include <mutex.h>
#include <exception.h>

static void          _mutex_free(mutex_t *);
static data_t *      _mutex_new(int, va_list);
static char *        _mutex_tostring(mutex_t *);
static data_t *      _mutex_enter(mutex_t *);
static data_t *      _mutex_leave(mutex_t *, data_t *);

extern data_t *      _mutex_create(data_t *, char *, array_t *, dict_t *);

static data_t *      _mutex_lock(mutex_t *, char *, array_t *, dict_t *);
static data_t *      _mutex_unlock(mutex_t *, char *, array_t *, dict_t *);

static int Mutex = -1;

static vtable_t _vtable_Mutex[] = {
  { .id = FunctionFactory,  .fnc = (void_t) _mutex_new },
  { .id = FunctionCmp,      .fnc = (void_t) mutex_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _mutex_free },
  { .id = FunctionToString, .fnc = (void_t) _mutex_tostring },
  { .id = FunctionHash,     .fnc = (void_t) mutex_hash },
  { .id = FunctionEnter,    .fnc = (void_t) _mutex_enter },
  { .id = FunctionLeave,    .fnc = (void_t) _mutex_leave },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methods_Mutex[] = {
  { .type = -1,     .name = "lock",    .method = (method_t) _mutex_lock,    .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "unlock",  .method = (method_t) _mutex_unlock,  .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,      .method = NULL,                      .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

static int Condition = -1;

static void          _condition_free(condition_t *);
static data_t *      _condition_new(condition_t *, va_list);
static char *        _condition_tostring(condition_t *);
static data_t *      _condition_enter(condition_t *);
static data_t *      _condition_leave(condition_t *, data_t *);

extern data_t *      _condition_create(char *, array_t *, dict_t *);

static data_t *      _condition_acquire(condition_t *, char *, array_t *, dict_t *);
static data_t *      _condition_release(condition_t *, char *, array_t *, dict_t *);
static data_t *      _condition_wakeup(condition_t *, char *, array_t *, dict_t *);
static data_t *      _condition_sleep(condition_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_Condition[] = {
  { .id = FunctionNew,      .fnc = (void_t) _condition_new },
  { .id = FunctionCmp,      .fnc = (void_t) condition_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _condition_free },
  { .id = FunctionToString, .fnc = (void_t) _condition_tostring },
  { .id = FunctionHash,     .fnc = (void_t) condition_hash },
  { .id = FunctionEnter,    .fnc = (void_t) _condition_enter },
  { .id = FunctionLeave,    .fnc = (void_t) _condition_leave },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methods_Condition[] = {
  { .type = -1,     .name = "acquire",   .method = (method_t) _condition_acquire, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "release",   .method = (method_t) _condition_release, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "wakeup",    .method = (method_t) _condition_wakeup,  .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "sleep",     .method = (method_t) _condition_sleep,   .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,        .method = NULL,                          .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int mutex_debug = -1;

/* ------------------------------------------------------------------------ */

void mutex_init(void) {
  if (Mutex < 1) {
    logging_register_category("mutex", &mutex_debug);
    typedescr_register_with_methods(Mutex, mutex_t);
    typedescr_register_with_methods(Condition, condition_t);
  }
}

/* ------------------------------------------------------------------------ */

data_t * _mutex_new(int type, va_list args) {
  data_t *ret;

  ret = (data_t *) mutex_create();
  if (!ret) {
    ret = data_exception_from_errno();
  }
  return ret;
}

void _mutex_free(mutex_t *mutex) {
  if (mutex) {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&mutex -> mutex);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
    DeleteCriticalSection(&(mutex -> cs));
#endif /* HAVE_PTHREAD_H */
  }
}

char * _mutex_tostring(mutex_t *data) {
  return "mutex";
}

data_t * _mutex_enter(mutex_t *mutex) {
  return mutex_lock(mutex) ? data_exception_from_errno() :  data_true();
}

data_t * _mutex_leave(mutex_t *mutex, data_t *param) {
  return mutex_unlock(mutex) ? data_exception_from_errno() :  param;
}

/* ------------------------------------------------------------------------ */

mutex_t * mutex_create() {
#ifdef HAVE_PTHREAD_H
  pthread_mutexattr_t  attr;
#endif /* HAVE_PTHREAD_H */
  mutex_t             *mutex;

  mutex_init();
  mutex = data_new(Mutex, mutex_t);
#ifdef HAVE_PTHREAD_H
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  if ((errno = pthread_mutex_init(&mutex -> mutex, &attr))) {
    free(mutex);
    mutex = NULL;
  }
  pthread_mutexattr_destroy(&attr);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  InitializeCriticalSection(&(mutex -> cs));
#endif /* HAVE_PTHREAD_H */
  mdebug(mutex, "Mutex created");
  return mutex;
}

int mutex_cmp(mutex_t *m1, mutex_t *m2) {
  return ((char *) m1) - ((char *) m2);
}

unsigned int mutex_hash(mutex_t *mutex) {
#ifdef HAVE_PTHREAD_H
  return hash(&mutex -> mutex, sizeof(pthread_mutex_t));
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  return hash(&mutex -> cs, sizeof(CRITICAL_SECTION));
#endif /* HAVE_PTHREAD_H */
}

int mutex_lock(mutex_t *mutex) {
  int retval = 0;

  mdebug(mutex, "Locking mutex");
#ifdef HAVE_PTHREAD_H
  errno = pthread_mutex_lock(&mutex -> mutex);
  if (errno) {
    retval = -1;
  }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  EnterCriticalSection(&(mutex -> cs));
#endif /* HAVE_PTHREAD_H */
  if (retval) {
    error("Error locking mutex: %d", errno);
  } else {
    mdebug(mutex, "Mutex locked");
  }
  return retval;
}

/**
 * @return 0 if the mutex was successfully locked
 *         1 if the mutex was owned by another thread
 *         -1 If an error occurred.
 */
int mutex_trylock(mutex_t *mutex) {
  int retval = 0;

  mdebug(mutex, "Trying to lock mutex");
#ifdef HAVE_PTHREAD_H
  errno = pthread_mutex_trylock(&mutex -> mutex);
  switch (errno) {
    case 0:
      retval = 0;
      break;
    case EBUSY:
      retval = 1;
      break;
    default:
      retval = -1;
      break;
  }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  retval = (!TryEnterCriticalSection(&mutex -> cs)) ? 0 : 1;
#endif /* HAVE_PTHREAD_H */
  mdebug(mutex, "Trylock mutex: %s", (retval) ? "Fail" : "Success");
  return retval;
}

int mutex_unlock(mutex_t *mutex) {
  int retval = 0;

  mdebug(mutex, "Unlocking mutex");
#ifdef HAVE_PTHREAD_H
  errno = pthread_mutex_unlock(&mutex -> mutex);
  if (errno) {
    retval = -1;
  }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  EnterCriticalSection(&mutex -> cs);
#endif /* HAVE_PTHREAD_H */
  if (retval) {
    error("Error unlocking mutex: %d", errno);
  } else {
    mdebug(mutex, "Mutex unlocked");
  }
  return retval;
}

/* ------------------------------------------------------------------------ */


data_t * _mutex_create(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) self;
  (void) name;
  (void) args;
  (void) kwargs;

  mutex_init();
  return data_create(Mutex);
}

data_t * _mutex_lock(mutex_t *mutex, char *name, array_t *args, dict_t *kwargs) {
	int wait = TRUE;

  (void) name;
  (void) kwargs;

  if (args && array_size(args)) {
    wait = data_intval(data_array_get(args, 0));
  }
  if (wait) {
    return _mutex_enter(mutex);
  } else {
    switch (mutex_trylock(mutex)) {
      case 1:
        return data_false();
      case -1:
        return data_exception_from_errno();
      default:
        return data_true();
    }
  }
}

data_t * _mutex_unlock(mutex_t *mutex, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  return _mutex_leave(mutex, data_true());
}

/* ------------------------------------------------------------------------ */
/* -- C O N D I T I O N _ T ----------------------------------------------- */
/* ------------------------------------------------------------------------ */

data_t * _condition_new(condition_t *condition, va_list args) {
  mutex_t *mutex = va_arg(args, mutex_t *);

  condition -> mutex = (mutex) ? mutex_copy(mutex) : mutex_create();
  condition -> borrowed_mutex = !mutex;
#ifdef HAVE_PTHREAD_H
  pthread_cond_init(&condition -> condition, NULL);
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  InitializeConditionVariable(&condition -> condition);
#endif /* HAVE_PTHREAD_H */
  debug(mutex, "Condition created");
  return (data_t *) condition;
}

void _condition_free(condition_t *condition) {
  if (condition) {
#ifdef HAVE_PTHREAD_H
    pthread_cond_destroy(&condition -> condition);
#endif /* HAVE_PTHREAD_H */
    mutex_free(condition -> mutex);
  }
}

char * _condition_tostring(condition_t *data) {
  return "condition";
}

data_t * _condition_enter(condition_t *condition) {
  return condition_acquire(condition) ? data_exception_from_errno() : data_true();
}

data_t * _condition_leave(condition_t *condition, data_t *param) {
  return condition_wakeup(condition) ? data_exception_from_errno() :  param;
}

/* ------------------------------------------------------------------------ */

condition_t * condition_create() {
  mutex_init();
  return (condition_t *) data_create(Condition, NULL);
}

int condition_cmp(condition_t *c1, condition_t *c2) {
  return ((char *) c1) - ((char *) c2);
}

unsigned int condition_hash(condition_t *condition) {
#ifdef HAVE_PTHREAD_H
  return hash(&condition -> condition, sizeof(pthread_cond_t));
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  return hash(&condition -> condition, sizeof(CONDITION_VARIABLE));
#endif /* HAVE_PTHREAD_H */
}

int condition_acquire(condition_t *condition) {
  mdebug(mutex, "Acquiring condition");
  return mutex_lock(condition -> mutex);
}

int condition_release(condition_t *condition) {
  mdebug(mutex, "Releasing condition");
  return mutex_unlock(condition -> mutex);
}

/**
 * @return 0 if the condition was successfully locked
 *         1 if the condition was owned by another thread
 *         -1 If an error occurred.
 */
int condition_tryacquire(condition_t *condition) {
  mdebug(mutex, "Trying to acquire condition");
  return mutex_trylock(condition -> mutex);
}

int condition_wakeup(condition_t *condition) {
  int retval = 0;

  mdebug(mutex, "Waking up condition");
#ifdef HAVE_PTHREAD_H
  errno = pthread_cond_signal(&condition -> condition);
  if (errno) {
    retval = -1;
  }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  WakeConditionVariable (&condition -> condition);
#endif /* HAVE_PTHREAD_H */
  mutex_unlock(condition -> mutex);
  if (retval) {
    error("Error waking condition: %d", errno);
  } else {
    mdebug(mutex, "Condition woken up");
  }
  return retval;
}

int condition_sleep(condition_t *condition) {
  int retval = 0;

  mdebug(mutex, "Going to sleep on condition");
#ifdef HAVE_PTHREAD_H
  errno = pthread_cond_wait(&condition -> condition, &condition -> mutex -> mutex);
  if (errno) {
    retval = -1;
  }
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  SleepConditionVariableCS(&condition -> condition, &condition -> mutex -> cs, INFINITE);
#endif /* HAVE_PTHREAD_H */
if (retval) {
  error("Error sleeping on condition: %d", errno);
} else {
  mdebug(mutex, "Woke up from condition");
}
  return retval;
}

/* ------------------------------------------------------------------------ */

data_t * _condition_create(char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  mutex_init();
  return data_create(Condition);
}

data_t * _condition_acquire(condition_t *condition, char *name, array_t *args, dict_t *kwargs) {
	int wait = TRUE;

  (void) name;
  (void) kwargs;

  if (args && array_size(args)) {
    wait = data_intval(data_array_get(args, 0));
  }
  if (wait) {
    return _condition_enter(condition);
  } else {
    switch (condition_tryacquire(condition)) {
    	case 1:
        return data_false();
    	case -1:
        return data_exception_from_errno();
    	default:
        return data_true();
    }
  }
}

data_t * _condition_release(condition_t *condition, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  if (condition_release(condition) < 0) {
    return data_exception_from_errno();
  } else {
    return data_true();
  }
}

data_t * _condition_wakeup(condition_t *condition, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  return _condition_leave(condition, data_true());
}

data_t * _condition_sleep(condition_t *condition, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  return condition_sleep(condition) ? data_exception_from_errno() : data_true();
}
