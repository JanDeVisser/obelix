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

typedef enum _errorcode {
  ErrorSyntax,
  ErrorArgCount,
  ErrorType,
  ErrorName,
  ErrorNotCallable,
  ErrorRange,
  ErrorIOError,
  ErrorSysError,
  ErrorNotIterable,
  ErrorNotIterator,
  ErrorExhausted
} errorcode_t;

typedef struct _error {
  errorcode_t  code;
  char        *msg;
  char        *str;
} error_t;

extern int            error_register(char *str);
extern error_t *      error_create(int , ...);
extern error_t *      error_vcreate(int, va_list);
extern error_t *      error_copy(error_t *);
extern void           error_free(error_t *);
extern unsigned int   error_hash(error_t *);
extern int            error_cmp(error_t *, error_t *);
extern char *         error_tostring(error_t *);
extern void           error_report(error_t *);

#endif /* __EXCEPTION_H__ */
