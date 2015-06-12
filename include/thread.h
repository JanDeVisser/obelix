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

#ifndef __THREAD_H__
#define	__THREAD_H__

#include <data.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _thread {
  data_t           _d;
  pthread_t        thr_id;
  struct _thread  *parent;
  data_t          *kernel;
  void            *stack;
  free_t           onfree;
  pthread_mutex_t  mutex;
  data_t          *exit_code;
  char            *name;
} thread_t;

typedef void * (*threadproc_t)(void *);

extern thread_t *    thread_create(pthread_t, char *);
extern thread_t *    thread_new(char *, threadproc_t, void *);
extern thread_t *    thread_self(void);
extern unsigned int  thread_hash(thread_t *);
extern int           thread_cmp(thread_t *, thread_t *);
extern int           thread_interruptable(thread_t *);
extern int           thread_interrupt(thread_t *);
extern int           thread_yield(void);
extern thread_t *    thread_setname(thread_t *, char *);
extern data_t *      thread_resolve(thread_t *, char *);

extern data_t *      data_current_thread(void);
extern data_t *      data_thread_stacktrace(data_t *);
extern data_t *      data_thread_push_stackframe(data_t *);
extern data_t *      data_thread_pop_stackframe(void);
extern data_t *      data_thread_set_kernel(data_t *);
extern data_t *      data_thread_kernel(void);
extern data_t *      data_thread_set_exit_code(data_t *);
extern data_t *      data_thread_exit_code(void);

extern int Thread;
extern int thread_debug;

#define data_is_thread(d)  ((d) && (data_hastype((d), Thread)))
#define data_as_thread(d)  ((thread_t *) (data_is_thread((d)) ? (d) : NULL))
#define thread_free(o)     (data_free((data_t *) (o)))
#define thread_tostring(o) (data_tostring((data_t *) (o)))
#define thread_copy(o)     ((thread_t *) data_copy((data_t *) (o)))

#ifdef	__cplusplus
}
#endif

#endif /* __THREAD_H__ */
