/*
 * core.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CORE_H__
#define __CORE_H__

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#define TRUE   1
#define FALSE  0
#define NEW(t) ( (t *) new( sizeof(t) ) )

typedef enum _log_level {
  LogLevelDebug,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelFatal
} log_level_t;

typedef void    (*void_t)(void);
typedef void *  (*voidptr_t)(void *);
typedef int     (*cmp_t)(void *, void *);
typedef int     (*hash_t)(void *);
typedef char *  (*tostring_t)(void *);
typedef void *  (*reduce_t)(void *, void *);
typedef void    (*visit_t)(void *);
typedef void *  (*obj_reduce_t)(void *, reduce_t, void *);
typedef void    (*obj_visit_t)(void *, visit_t);
typedef visit_t free_t;
typedef int     (*read_t)(void *, char *, int);
typedef int     (*write_t)(void *, char *, int);
typedef void *  (*parse_t)(char *);

typedef enum _reduce_type {
  RTObjects = 1,
  RTChars = 2,
  RTStrs = 4
} reduce_type_t;

typedef enum _enum_function_type {
  Void, Visitor, Reducer, Stringifier, Destructor, Evaluator
} function_type_t; 

typedef struct _function_union {
  void_t     void_fnc;
  visit_t    visitor;
  reduce_t   reducer;
  tostring_t stringfier;
  free_t     destructor;
} function_ptr_t;

typedef struct _function {
  function_type_t type;
  function_ptr_t  fnc;
} function_t;

extern function_ptr_t no_func_ptr;
extern function_t no_func;

#define NOFUNCPTR no_func_ptr
#define NOFUNC no_func

typedef enum _abstract_function_type {
  AFTFree,
  AFTToString,
  AFTCmp,
  AFTHash,
  AFTReduce,
  AFTVisit,
  AFTRead,
  AFTWrite
} abstract_function_type_t;

typedef union _abstract_function_ptr {
  free_t        free;
  tostring_t    tostring;
  cmp_t         cmp;
  hash_t        hash;
  obj_reduce_t  reduce;
  obj_visit_t   visit;
  read_t        read;
  write_t       write;
} abstract_function_ptr_t;

typedef struct _abstract_function {
  abstract_function_type_t type;
  abstract_function_ptr_t  fnc;
} abstract_function_t;

typedef struct _type {
  char                *name;
  abstract_function_t  functions[];
} type_t;

typedef struct _reduce_ctx {
  void           *obj;
  void           *user;
  void           *data;
  function_ptr_t fnc;
} reduce_ctx;

typedef struct _reader {
  read_t read_fnc;
} reader_t;

extern void *       new(int);
extern void *       new_ptrarray(int);
extern void *       resize_block(void *, int);
extern void *       resize_ptrarray(void *, int);

extern unsigned int hash(void *, size_t);

extern char *       _log_level_str(log_level_t);
extern void         _logmsg(log_level_t, char *, int, char *, ...);

extern void         initialize_random(void);
extern char *       strrand(char *, size_t);
extern unsigned int strhash(char *);
extern char *       chars(void *);
extern int          atob(char *);
extern char *       btoa(long);
extern char *       itoa(long);
extern char *       dtoa(double);

extern reduce_ctx * reduce_ctx_create(void *, void *, function_ptr_t);

#define reader_read(reader, buf, n)  (((reader_t *) reader) -> read_fnc(reader, buf, n))
#ifndef NDEBUG
#define debug(fmt, args...)          _logmsg(LogLevelDebug, __FILE__, __LINE__, fmt, ## args)
#else /* NDEBUG */
#define debug(fmt, args...)
#endif /* NDEBUG */
#define info(fmt, args...)           _logmsg(LogLevelInfo, __FILE__, __LINE__, fmt, ## args)
#define warning(fmt, args...)        _logmsg(LogLevelWarning, __FILE__, __LINE__, fmt, ## args)
#define error(fmt, args...)          _logmsg(LogLevelError, __FILE__, __LINE__, fmt, ## args)

#endif /* __CORE_H__ */
