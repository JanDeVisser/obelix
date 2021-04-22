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

#include <pthread.h>

#include <data.h>
#include <mutex.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum _thread_status_flag {
  TSFNone  = 0x0000,
  TSFLeave = 0x0001
} thread_status_flag_t;

typedef struct _thread {
  data_t           _d;
  pthread_t        thread;
  mutex_t         *mutex;
  struct _thread  *parent;
  data_t          *kernel;
  void            *stack;
  int              status;
  data_t          *exit_code;
  char            *name;
  int              _errno;
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
extern int           thread_set_status(thread_t *, thread_status_flag_t);
extern int           thread_unset_status(thread_t *, thread_status_flag_t);
extern int           thread_has_status(thread_t *, thread_status_flag_t);
extern int           thread_status(thread_t *);

extern data_t *      data_current_thread(void);
extern data_t *      data_thread_stacktrace(data_t *);
extern data_t *      data_thread_push_stackframe(data_t *);
extern data_t *      data_thread_pop_stackframe(void);
extern data_t *      data_thread_set_kernel(data_t *);
extern data_t *      data_thread_kernel(void);
extern data_t *      data_thread_set_exit_code(data_t *);
extern data_t *      data_thread_exit_code(void);
extern void          data_thread_clear_exit_code(void);

extern int thread_debug;

type_skel(thread, Thread, thread_t);

#endif /* __MUTEX_H__ */
