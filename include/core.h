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

#include <oblconfig.h>
#include <assert.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <logging.h>

#ifndef OBLCORE_IMPEXP
  #define OBLCORE_IMPEXP       __DLL_IMPORT__
#endif /* OBLCORE_IMPEXP */

#ifndef HAVE_INTPTR_T
typedef long long intptr_t; /* FIXME: 32bits! */
#endif /* HAVE_INTPTR_T */

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TRUE                 1
#define FALSE                0
#define NEW(t)               ( (t *) _new( sizeof(t) ) )
#define NEWARR(n, t)         ( (t *) new_array((n), sizeof(t)))
#define NEWDYNARR(t, n, s)   ( (t *) _new( sizeof(t) + (n) * sizeof(s)))

typedef struct _code_label {
  int   code;
  char *label;
} code_label_t;

#define code_label(c)       { .code = c, .label = #c }

typedef void         (*void_t)(void);
typedef void         (*voidptr_t)(void *);
typedef void *       (*voidptrvoidptr_t)(void *);
typedef voidptr_t    visit_t;
typedef voidptr_t    free_t;

typedef void *       (*create_t)(void);
typedef void *       (*vcreate_t)(va_list);
typedef int          (*cmp_t)(const void *, const void *);
typedef unsigned int (*hash_t)(const void *);
typedef void *       (*copy_t)(const void *);
typedef char *       (*tostring_t)(const void *);
typedef void *       (*parse_t)(const char *);
typedef void *       (*copydata_t)(void *, const void *);
typedef void *       (*new_t)(const void *, va_list);
typedef void *       (*reduce_t)(void *, void *);
typedef int          (*read_t)(void *, char *, int);
typedef int          (*write_t)(void *, char *, int);
typedef void         (*obj_visit_t)(void *, visit_t);
typedef void *       (*obj_reduce_t)(void *, reduce_t, void *);

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

OBLCORE_IMPEXP void *          _new(size_t);
OBLCORE_IMPEXP void *          new_array(size_t, size_t);
OBLCORE_IMPEXP void *          new_ptrarray(size_t);
OBLCORE_IMPEXP void *          resize_block(void *, size_t, size_t);
OBLCORE_IMPEXP void *          resize_ptrarray(void *, size_t, size_t);

OBLCORE_IMPEXP int             oblcore_asprintf(char **, const char *, ...);
OBLCORE_IMPEXP int             oblcore_vasprintf(char **, const char *, va_list);
OBLCORE_IMPEXP unsigned int    hash(const void *, size_t);
OBLCORE_IMPEXP unsigned int    hashptr(const void *);
OBLCORE_IMPEXP unsigned int    hashlong(long);
OBLCORE_IMPEXP unsigned int    hashdouble(double);
OBLCORE_IMPEXP unsigned int    hashblend(unsigned int, unsigned int);

OBLCORE_IMPEXP void            initialize_random(void);
OBLCORE_IMPEXP char *          strrand(char *, size_t);
OBLCORE_IMPEXP unsigned int    strhash(const char *);
OBLCORE_IMPEXP char *          strltrim(char *);
OBLCORE_IMPEXP char *          strrtrim(char *);
OBLCORE_IMPEXP char *          strtrim(char *);
OBLCORE_IMPEXP char *          chars(const void *);
OBLCORE_IMPEXP int             atob(const char *);
OBLCORE_IMPEXP char *          btoa(long);
OBLCORE_IMPEXP int             strtoint(const char *, long *);
OBLCORE_IMPEXP char *          oblcore_itoa(long);
OBLCORE_IMPEXP char *          oblcore_dtoa(double);

OBLCORE_IMPEXP int             oblcore_strcasecmp(const char *, const char *);
OBLCORE_IMPEXP int             oblcore_strncasecmp(const char *, const char *, size_t);

OBLCORE_IMPEXP char *          escape(char *, const char *, char);
OBLCORE_IMPEXP char *          unescape(char *, char);
#define c_escape(s)            (escape((s), "\"\\", '\\'))
#define c_unescape(s)          (unescape((s), '\\'))

OBLCORE_IMPEXP char *          label_for_code(code_label_t *, int);
OBLCORE_IMPEXP char *          labels_for_bitmap(code_label_t *, int, char *, size_t);
OBLCORE_IMPEXP int             code_for_label(code_label_t *, char *);

OBLCORE_IMPEXP reduce_ctx *    reduce_ctx_create(void *, void *, void_t);
OBLCORE_IMPEXP reduce_ctx *    reduce_ctx_initialize(reduce_ctx *, void *, void *, void_t);
OBLCORE_IMPEXP reduce_ctx *    collection_hash_reducer(void *, reduce_ctx *);
OBLCORE_IMPEXP reduce_ctx *    collection_add_all_reducer(void *, reduce_ctx *);
OBLCORE_IMPEXP visit_t         collection_visitor(void *, visit_t);

OBLCORE_IMPEXP type_t           type_str;
OBLCORE_IMPEXP type_t           type_int;

#define new(i)                    (_new((i)))
#define stralloc(n)               ((char *) _new((n) + 1))
#ifndef _MSC_VER
#define asprintf(p, fmt, args...) oblcore_asprintf(p, fmt, ##args)
#else
#define asprintf(p, fmt, ...) oblcore_asprintf(p, fmt, __VA_ARGS__)
#endif
#define vasprintf(p, fmt, args)   oblcore_vasprintf(p, fmt, args)

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

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __CORE_H__ */
