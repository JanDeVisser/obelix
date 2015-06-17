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
  data_t  _d;
  char   *funcname;
  char   *source;
  int     line;
} stackframe_t;

extern stackframe_t *  stackframe_create(data_t *);
extern int             stackframe_cmp(stackframe_t *, stackframe_t *);

typedef struct _stacktrace {
  datastack_t *stack;
} stacktrace_t;

extern stacktrace_t * stacktrace_create(void);
extern int            stacktrace_cmp(stacktrace_t *, stacktrace_t *);
extern stacktrace_t * stacktrace_push(stacktrace_t *, data_t *);

extern int Stackframe;
extern int Stacktrace;

#define data_is_stackframe(d)  ((d) && data_hastype((data_t *) (d), Stackframe))
#define data_as_stackframe(d)  (data_is_stackframe((d)) ? ((stackframe_t *) (d)) : NULL)
#define stackframe_copy(o)     ((stackframe_t *) data_copy((data_t *) (o)))
#define stackframe_free(o)     (data_free((data_t *) (o)))
#define stackframe_tostring(o) (data_tostring((data_t *) (o)))

#define data_is_stacktrace(d)  ((d) && data_hastype((data_t *) (d), Stacktrace))
#define data_as_stacktrace(d)  (data_is_stacktrace((d)) ? ((stacktrace_t *) (d)) : NULL)
#define stacktrace_copy(o)     ((stacktrace_t *) data_copy((data_t *) (o)))
#define stacktrace_free(o)     (data_free((data_t *) (o)))
#define stacktrace_tostring(o) (data_tostring((data_t *) (o)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __STACKTRACE_H__ */
