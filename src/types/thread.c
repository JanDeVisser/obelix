/*
 * /obelix/src/types/thread.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
#include <pthread.h>
#include <data.h>

typedef struct _thread {
  pthread_t  thr_id;
  char      *name;
  int        refs;
} thread_t;

static thread_t *    thread_create(pthread_t, char *);
static void          thread_free(thread_t *);
static thread_t *    threat_copy(thread_t *);
static unsigned int  thread_hash(thread_t *);
static int           thread_cmp(thread_t *, thread_t *);
static char *        thread_tostring(thread_t *);

static void          _data_init_thread(void) __attribute__((constructor));
static data_t *      _data_new_thread(data_t *, va_list);
static data_t *      _data_copy_thread(data_t *, data_t *);
static int           _data_cmp_thread(data_t *, data_t *);
static char *        _data_tostring_thread(data_t *);
static unsigned int  _data_hash_thread(data_t *);
static data_t *      _data_resolve_thread(data_t *, char *);

static data_t *      _thread_interrupt(data_t *, char *, array_t *, dict_t *);
static data_t *      _thread_yield(data_t *, char *, array_t *, dict_t *);

vtable_t _vtable_thread[] = {
  { .id = FunctionNew,      .fnc = (void_t) _data_new_thread },
  { .id = FunctionCopy,     .fnc = (void_t) _data_copy_thread },
  { .id = FunctionCmp,      .fnc = (void_t) _data_cmp_thread },
  { .id = FunctionFree,     .fnc = (void_t) _thread_free },
  { .id = FunctionToString, .fnc = (void_t) _data_tostring_thread },
  { .id = FunctionHash,     .fnc = (void_t) _data_hash_thread },
  { .id = FunctionResolve,  .fnc = (void_t) _data_resolve_thread },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t typedescr_thread = {
  .type       = Thread,
  .type_name  = "thread",
  .vtable     = _vtable_thread
};

static methoddescr_t _methoddescr_thread[] = {
  { .type = Thread, .name = "interrupt", .method = _thread_interrupt, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = Thread, .name = "yield",     .method = _thread_yield,     .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,        .method = NULL,              .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};
