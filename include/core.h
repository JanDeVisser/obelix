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

#include <config.h>

#define TRUE          1
#define FALSE         0
#define NEW(t)        ( (t *) new( sizeof(t) ) )
#define NEWARR(n, t)  ( (t *) new_array((n), sizeof(t)))

typedef enum _log_level {
  LogLevelDebug = 0,
  LogLevelInfo,
  LogLevelWarning,
  LogLevelError,
  LogLevelFatal
} log_level_t;

extern log_level_t log_level;

typedef struct _code_label {
  int   code;
  char *label;
} code_label_t;

typedef void      (*void_t)(void);
typedef void      (*voidptr_t)(void *);
typedef voidptr_t visit_t;
typedef voidptr_t free_t;

typedef int       (*cmp_t)(void *, void *);
typedef int       (*hash_t)(void *);
typedef char *    (*tostring_t)(void *);
typedef void *    (*parse_t)(char *);
typedef void *    (*copydata_t)(void *, void *);
typedef void *    (*new_t)(void *, va_list);
typedef void *    (*reduce_t)(void *, void *);
typedef int       (*read_t)(void *, char *, int);
typedef int       (*write_t)(void *, char *, int);
typedef void      (*obj_visit_t)(void *, visit_t);
typedef void *    (*obj_reduce_t)(void *, reduce_t, void *);

typedef enum _reduce_type {
  RTObjects = 1,
  RTChars = 2,
  RTStrs = 4
} reduce_type_t;

typedef struct _function {
  char      *name;
  voidptr_t  fnc;
  int        min_params;
  int        max_params;
  char      *str;
  int        refs;
} function_t;

typedef struct _reduce_ctx {
  void           *obj;
  void           *user;
  void           *data;
  long            longdata;
  void_t          fnc;
} reduce_ctx;

typedef struct _reader {
  read_t read_fnc;
  free_t free;
} reader_t;

extern void *          new(int);
extern void *          new_array(int, int);
extern void *          new_ptrarray(int);
extern void *          resize_block(void *, int, int);
extern void *          resize_ptrarray(void *, int, int);

#ifndef HAVE_ASPRINTF
extern int             asprintf(char **, const char *, ...);
#endif
#ifndef HAVE_VASPRINTF
extern int             vasprintf(char **, const char *, va_list);
#endif
extern unsigned int    hash(void *, size_t);
extern unsigned int    hashptr(void *);
extern unsigned int    hashlong(long);
extern unsigned int    hashdouble(double);
extern unsigned int    hashblend(unsigned int, unsigned int);

extern char *          _log_level_str(log_level_t);
extern void            _logmsg(log_level_t, char *, int, char *, ...);

extern void            initialize_random(void);
extern char *          strrand(char *, size_t);
extern unsigned int    strhash(char *);
extern char *          chars(void *);
extern int             atob(char *);
extern char *          btoa(long);
extern int             strtoint(char *, long *);
extern char *          itoa(long);
extern char *          dtoa(double);

extern char *          label_for_code(code_label_t *, int);
extern int             code_for_label(code_label_t *, char *);

extern function_t *    function_create(char *, voidptr_t);
extern function_t *    function_create_noresolve(char *);
extern function_t *    function_copy(function_t *);
extern void            function_free(function_t *);
extern char *          function_tostring(function_t *);
extern unsigned int    function_hash(function_t *);
extern int             function_cmp(function_t *, function_t *);
extern function_t *    function_resolve(function_t *fnc);

extern reduce_ctx *    reduce_ctx_create(void *, void *, void_t);
extern reduce_ctx *    collection_hash_reducer(void *, reduce_ctx *);
extern reduce_ctx *    collection_add_all_reducer(void *, reduce_ctx *);
extern visit_t         collection_visitor(void *, visit_t);


#define reader_read(reader, buf, n)  (((reader_t *) reader) -> read_fnc(reader, buf, n))
#define reader_free(rdr)             if (rdr) (((reader_t *) (rdr)) -> free((rdr)))
#ifndef NDEBUG
#define debug(fmt, args...)          _logmsg(LogLevelDebug, __FILE__, __LINE__, fmt, ## args)
#else /* NDEBUG */
#define debug(fmt, args...)
#endif /* NDEBUG */
#define info(fmt, args...)           _logmsg(LogLevelInfo, __FILE__, __LINE__, fmt, ## args)
#define warning(fmt, args...)        _logmsg(LogLevelWarning, __FILE__, __LINE__, fmt, ## args)
#define error(fmt, args...)          _logmsg(LogLevelError, __FILE__, __LINE__, fmt, ## args)

#endif /* __CORE_H__ */
