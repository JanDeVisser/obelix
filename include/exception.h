/*
 * /obelix/include/error.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <stdarg.h>

#include <data.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */
  
typedef enum _errorcode {
  ErrorSyntax,
  ErrorArgCount,
  ErrorMaxStackDepthExceeded,
  ErrorInternalError,
  ErrorType,
  ErrorName,
  ErrorNotCallable,
  ErrorRange,
  ErrorIOError,
  ErrorSysError,
  ErrorNotIterable,
  ErrorNotIterator,
  ErrorExhausted,
  ErrorThrowable,
  ErrorLeave,
  ErrorExit
} errorcode_t;

typedef struct _exception {
  errorcode_t   code;
  char         *msg;
  char         *str;
  int           handled;
  struct _data *throwable;
  struct _data *trace;
  int           refs;
} exception_t;

extern int            exception_register(char *str);
extern exception_t *  exception_create(int, char *, ...);
extern exception_t *  exception_vcreate(int, char *, va_list);
extern exception_t *  exception_from_errno(void);
extern exception_t *  exception_copy(exception_t *);
extern void           exception_free(exception_t *);
extern unsigned int   exception_hash(exception_t *);
extern int            exception_cmp(exception_t *, exception_t *);
extern char *         exception_tostring(exception_t *);
extern void           exception_report(exception_t *);

extern data_t *       data_exception(int, char *, ...);
extern data_t *       data_exception_from_errno(void);
extern data_t *       data_throwable(data_t *);

#define data_is_exception(d) ((d) && (data_type((d)) == Exception))
#define data_is_unhandled_exception(d) ((d) && (data_type((d)) == Exception) && !((exception_t *) (d) -> ptrval) -> handled)
#define data_exceptionval(d) ((exception_t *) ((data_is_exception((d)) ? (d) -> ptrval : NULL)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __EXCEPTION_H__ */
