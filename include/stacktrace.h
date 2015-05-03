/*
 * /obelix/include/stacktrace.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __STACKTRACE_H__
#define __STACKTRACE_H__

#include <stdarg.h>

#include <data.h>
#include <datastack.h>
#include <script.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _stackframe {
  char *funcname;
  char *source;
  int   line;
  int   refs;
  char *str;
} stackframe_t;

extern stackframe_t *  stackframe_create(data_t *);
extern void            stackframe_free(stackframe_t *);
extern stackframe_t *  stackframe_copy(stackframe_t *);
extern int             stackframe_cmp(stackframe_t *, stackframe_t *);
extern char *          stackframe_tostring(stackframe_t *);

typedef struct _stacktrace {
  datastack_t *stack;
  int          refs;
  char        *str;
} stacktrace_t;

extern stacktrace_t * stacktrace_create(void);
extern void           stacktrace_free(stacktrace_t *);
extern stacktrace_t * stacktrace_copy(stacktrace_t *);
extern int            stacktrace_cmp(stacktrace_t *, stacktrace_t *);
extern char *         stacktrace_tostring(stacktrace_t *);
extern stacktrace_t * stacktrace_push(stacktrace_t *, data_t *);

extern int Stackframe;
extern int Stacktrace;

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __STACKTRACE_H__ */
