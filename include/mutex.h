/*
 * /obelix/include/thread.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __MUTEX_H__
#define	__MUTEX_H__

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */
#ifdef HAVE_INITIALIZECRITICALSECTION
  #ifdef HAVE_WINDOWS_H
    #include <windows.h>
  #endif /* HAVE_WINDOWS_H */
#endif /* HAVE_INITIALIZECRITICALSECTION */

#include <data.h>

#ifdef	__cplusplus
extern "C" {
#endif
  
typedef struct _mutex {
  data_t             _d;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t    mutex;
#elif defined(HAVE_INITIALIZECRITICALSECTION)
  CRITICAL_SECTION   cs;
#endif /* HAVE_PTHREAD_H */
} mutex_t;

typedef struct _condition {
  data_t             _d;
  mutex_t           *mutex;
#ifdef HAVE_PTHREAD_H
  pthread_cond_t     condition;
#elif defined(HAVE_CREATEMUTEX)
  CONDITION_VARIABLE condition;
#endif
} condition_t;

OBLCORE_IMPEXP mutex_t *     mutex_create(void);
OBLCORE_IMPEXP unsigned int  mutex_hash(mutex_t *);
OBLCORE_IMPEXP int           mutex_cmp(mutex_t *, mutex_t *);
OBLCORE_IMPEXP int           mutex_lock(mutex_t *);
OBLCORE_IMPEXP int           mutex_trylock(mutex_t *);
OBLCORE_IMPEXP int           mutex_unlock(mutex_t *);

OBLCORE_IMPEXP condition_t * condition_create();
OBLCORE_IMPEXP unsigned int  condition_hash(condition_t *);
OBLCORE_IMPEXP int           condition_cmp(condition_t *, condition_t *);
OBLCORE_IMPEXP int           condition_acquire(condition_t *);
OBLCORE_IMPEXP int           condition_tryacquire(condition_t *);
OBLCORE_IMPEXP int           condition_wakeup(condition_t *);
OBLCORE_IMPEXP int           condition_sleep(condition_t *);

OBLCORE_IMPEXP int Mutex;
OBLCORE_IMPEXP int Condition;

#define data_is_mutex(d)      ((d) && (data_hastype((d), Mutex)))
#define data_as_mutex(d)      ((mutex_t *) (data_is_mutex((d)) ? (d) : NULL))
#define mutex_free(o)         (data_free((data_t *) (o)))
#define mutex_tostring(o)     (data_tostring((data_t *) (o)))
#define mutex_copy(o)         ((mutex_t *) data_copy((data_t *) (o)))

#define data_is_condition(d)  ((d) && (data_hastype((d), Condition)))
#define data_as_condition(d)  ((condition_t *) (data_is_condition((d)) ? (d) : NULL))
#define condition_free(o)     (data_free((data_t *) (o)))
#define condition_tostring(o) (data_tostring((data_t *) (o)))
#define condition_copy(o)     ((condition_t *) data_copy((data_t *) (o)))

#ifdef	__cplusplus
}
#endif

#endif /* __MUTEX_H__ */
