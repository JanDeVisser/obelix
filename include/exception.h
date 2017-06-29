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

#include <oblconfig.h>
#include <stdarg.h>
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

#include <data.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum _errorcode {
  ErrorNoError                 = 0,
  ErrorSyntax                  = 1,
  ErrorArgCount,              /* 2 */
  ErrorMaxStackDepthExceeded, /* 3 */
  ErrorInternalError,         /* 4 */
  ErrorType,                  /* 5 */
  ErrorName,                  /* 6 */
  ErrorNotCallable,           /* 7 */
  ErrorRange,                 /* 8 */
  ErrorIOError,               /* 9 */
  ErrorSysError,              /* 10 */
  ErrorFunctionUndefined,     /* 11 */
  ErrorParameterValue,        /* 12 */
  ErrorOverflow,              /* 13 */
  ErrorNotIterable,           /* 14 */
  ErrorNotIterator,           /* 15 */
  ErrorExhausted,             /* 16 */
  ErrorThrowable,             /* 17 */
  ErrorLeave,                 /* 18 */
  ErrorReturn,                /* 19 */
  ErrorExit,                  /* 20 */
  ErrorYield,                 /* 21 */
  ErrorQuit                   /* 22 */
} errorcode_t;

typedef struct _exception_t {
  data_t        _d;
  errorcode_t   code;
  char         *msg;
  int           handled;
  struct _data *throwable;
  struct _data *trace;
} exception_t;

OBLCORE_IMPEXP int            _exception_register(char *str);
OBLCORE_IMPEXP exception_t *  exception_create(int, char *, ...);
OBLCORE_IMPEXP exception_t *  exception_vcreate(int, char *, va_list);
OBLCORE_IMPEXP exception_t *  exception_from_my_errno(int);
OBLCORE_IMPEXP exception_t *  exception_from_errno(void);
OBLCORE_IMPEXP unsigned int   exception_hash(exception_t *);
OBLCORE_IMPEXP int            exception_cmp(exception_t *, exception_t *);
OBLCORE_IMPEXP void           exception_report(exception_t *);
OBLCORE_IMPEXP char *         _exception_getcodestr(exception_t *);

OBLCORE_IMPEXP data_t *       data_exception(int, char *, ...);
OBLCORE_IMPEXP data_t *       data_exception_from_my_errno(int);
OBLCORE_IMPEXP data_t *       data_exception_from_errno(void);
OBLCORE_IMPEXP data_t *       data_throwable(data_t *);

#define exception_register(e)            (e = _exception_register( #e ))

type_skel(exception, Exception, exception_t);

static inline int data_is_unhandled_exception(void *ex) {
  return data_is_exception(ex) && !(data_as_exception(ex) -> handled);
}

static inline int data_is_exception_with_code(void *ex, int code) {
  return data_is_exception(ex) && ((data_as_exception(ex) -> code) == code);
}

static inline char * exception_getmessage(void *ex) {
  return data_is_exception(ex)
    ? data_as_exception(ex) -> msg : data_tostring(ex);
}

static inline int exception_getcode(void *ex) {
  return data_is_exception(ex)
    ? data_as_exception(ex) -> code : -1;
}

static inline char * exception_getcodestr(void *ex) {
  return data_is_exception(ex)
    ? _exception_getcodestr(data_as_exception(ex)) : NULL;
}

static inline int exception_handled(void *ex) {
  return data_is_exception(ex)
    ? data_as_exception(ex) -> handled : -1;
}

static inline data_t * exception_throwable(void *ex) {
  return data_is_exception(ex)
    ? data_as_exception(ex) -> throwable : NULL;
}

static inline data_t * exception_trace(void *ex) {
  return data_is_exception(ex)
    ? data_as_exception(ex) -> trace : NULL;
}

#ifdef __WIN32__
#define get_last_error()               (GetLastError())
#else
#define get_last_error()               (errno)
#endif

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __EXCEPTION_H__ */
