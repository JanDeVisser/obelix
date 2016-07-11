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

#include <config.h>
#include <core-setup.h>
#include <stdarg.h>
#include <logging.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TRUE          1
#define FALSE         0
#define NEW(t)        ( (t *) _new( sizeof(t) ) )
#define NEWARR(n, t)  ( (t *) new_array((n), sizeof(t)))
  
typedef struct _code_label {
  int   code;
  char *label;
} code_label_t;

typedef void      (*void_t)(void);
typedef void      (*voidptr_t)(void *);
typedef void *    (*voidptrvoidptr_t)(void *);
typedef voidptr_t visit_t;
typedef voidptr_t free_t;

typedef void *    (*create_t)(void);
typedef void *    (*vcreate_t)(va_list);
typedef int       (*cmp_t)(void *, void *);
typedef int       (*hash_t)(void *);
typedef void *    (*copy_t)(void *);
typedef char *    (*tostring_t)(void *);
typedef void *    (*parse_t)(char *);
typedef void *    (*copydata_t)(void *, void *);
typedef void *    (*new_t)(void *, va_list);
typedef void *    (*reduce_t)(void *, void *);
typedef int       (*read_t)(void *, char *, int);
typedef int       (*write_t)(void *, char *, int);
typedef void      (*obj_visit_t)(void *, visit_t);
typedef void *    (*obj_reduce_t)(void *, reduce_t, void *);

typedef struct _type {
  hash_t     hash;
  tostring_t tostring;
  copy_t     copy;
  free_t     free;
  cmp_t      cmp;
} type_t;

typedef enum _reduce_type {
  RTObjects = 1,
  RTChars = 2,
  RTStrs = 4
} reduce_type_t;

typedef struct _reduce_ctx {
  void           *obj;
  void           *user;
  void           *data;
  long            longdata;
  void_t          fnc;
} reduce_ctx;

OBLCORE_IMPEXP void *          _new(int);
OBLCORE_IMPEXP void *          new_array(int, int);
OBLCORE_IMPEXP void *          new_ptrarray(int);
OBLCORE_IMPEXP void *          resize_block(void *, int, int);
OBLCORE_IMPEXP void *          resize_ptrarray(void *, int, int);

#ifndef HAVE_ASPRINTF
OBLCORE_IMPEXP int             asprintf(char **, const char *, ...);
#endif
#ifndef HAVE_VASPRINTF
OBLCORE_IMPEXP int             vasprintf(char **, const char *, va_list);
#endif
OBLCORE_IMPEXP unsigned int    hash(void *, size_t);
OBLCORE_IMPEXP unsigned int    hashptr(void *);
OBLCORE_IMPEXP unsigned int    hashlong(long);
OBLCORE_IMPEXP unsigned int    hashdouble(double);
OBLCORE_IMPEXP unsigned int    hashblend(unsigned int, unsigned int);

OBLCORE_IMPEXP void            initialize_random(void);
OBLCORE_IMPEXP char *          strrand(char *, size_t);
OBLCORE_IMPEXP unsigned int    strhash(char *);
OBLCORE_IMPEXP char *          chars(void *);
OBLCORE_IMPEXP int             atob(char *);
OBLCORE_IMPEXP char *          btoa(long);
OBLCORE_IMPEXP int             strtoint(char *, long *);
OBLCORE_IMPEXP char *          oblcore_itoa(long);
OBLCORE_IMPEXP char *          oblcore_dtoa(double);

OBLCORE_IMPEXP int             oblcore_strcasecmp(char *, char *);
OBLCORE_IMPEXP int             oblcore_strncasecmp(char *, char *, size_t);

OBLCORE_IMPEXP char *          label_for_code(code_label_t *, int);
OBLCORE_IMPEXP int             code_for_label(code_label_t *, char *);

OBLCORE_IMPEXP reduce_ctx *    reduce_ctx_create(void *, void *, void_t);
OBLCORE_IMPEXP reduce_ctx *    collection_hash_reducer(void *, reduce_ctx *);
OBLCORE_IMPEXP reduce_ctx *    collection_add_all_reducer(void *, reduce_ctx *);
OBLCORE_IMPEXP visit_t         collection_visitor(void *, visit_t);

OBLCORE_IMPEXP type_t *        type_str;
OBLCORE_IMPEXP type_t *        type_int;

#define new(i)          (_new((i)))
#define stralloc(n)     ((char *) _new((n) + 1))

#define type_copy(d, s) (memcpy((d), (s), sizeof(type_t)))

#ifdef HAVE__STRDUP
#define strdup _strdup
#endif

#ifdef itoa
#undef itoa
#endif
#define itoa(i)        (oblcore_itoa((i)))

#ifdef HAVE__DTOA
#define dtoa _dtoa
#else
#ifndef HAVE_DTOA
#define dtoa(i)        (oblcore_dtoa((i)))
#endif /* !HAVE_DTOA */
#endif /* HAVE__DTOA */

#ifndef HAVE_STRCASECMP
#ifdef HAVE__STRICMP
#define strcasecmp _stricmp 
#else
#define strcasecmp oblcore_strcasecmp
#endif /* HAVE__STRICMP */
#endif /* HAVE_STRCASECMP */

#ifndef HAVE_STRNCASECMP
#ifdef HAVE__STRNICMP
#define strncasecmp _strnicmp 
#else
#define strncasecmp oblcore_strncasecmp
#endif /* HAVE__STRNICMP */
#endif /* HAVE_STRNCASECMP */

#ifndef HAVE__MAX_PATH
#define _MAX_PATH     512
#endif

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __CORE_H__ */
