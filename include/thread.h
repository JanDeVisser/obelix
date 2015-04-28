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

#include <file.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _thread {
  pthread_t  thr_id;
  void      *stack;
  free_t     onfree;
  char      *name;
  int        refs;
} thread_t;

typedef void * (*threadproc_t)(void *);

extern thread_t *    thread_create(pthread_t, char *);
extern thread_t *    thread_new(char *, threadproc_t, void *);
extern thread_t *    thread_self(void);
extern void          thread_free(thread_t *);
extern thread_t *    thread_copy(thread_t *);
extern unsigned int  thread_hash(thread_t *);
extern int           thread_cmp(thread_t *, thread_t *);
extern char *        thread_tostring(thread_t *);
extern int           thread_interruptable(thread_t *);
extern int           thread_interrupt(thread_t *);
extern int           thread_yield(void);
extern thread_t *    thread_setname(thread_t *, char *);

#ifdef	__cplusplus
}
#endif

#endif /* __THREAD_H__ */
