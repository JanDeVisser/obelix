/*
 * /obelix/include/typedescr.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __TYPEDESCR_H__
#define __TYPEDESCR_H__

#include <array.h>
#include <dict.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_METHOD_PARAMS      3
#define MAX_INHERITS           3

typedef enum _metatype {
  NoType          = 0,
  Dynamic         = 100,
  Interface       = 1000, /* Marker */
  Number,        /* 1001 */
  InputStream,   /* 1002 */
  OutputStream,  /* 1003 */
  Iterable,      /* 1004 */
  Iterator,      /* 1005 */
  Scope,         /* 1006 */
  Callable,      /* 1007 */
  Connector,     /* 1008 */
  CtxHandler,    /* 1009 */
  Incrementable, /* 1010 */
  Any,           /* 1011 */
  NextInterface  /* 1012 - Marker */
} metatype_t;

typedef enum _vtable_id {
  FunctionNone          = 0,
  FunctionFactory,     /* 1  */
  FunctionNew,         /* 2  */
  FunctionCopy,        /* 3  */
  FunctionCmp,         /* 4  */
  FunctionFreeData,    /* 5  */
  FunctionFree,        /* 6  */
  FunctionToString,    /* 7  */
  FunctionFltValue,    /* 8  */
  FunctionIntValue,    /* 9  */
  FunctionParse,       /* 10 */
  FunctionCast,        /* 11 */
  FunctionHash,        /* 12 */
  FunctionLen,         /* 13 */
  FunctionResolve,     /* 14 */
  FunctionCall,        /* 15 */
  FunctionSet,         /* 16 */
  FunctionRead,        /* 17 */
  FunctionWrite,       /* 18 */
  FunctionOpen,        /* 19 */
  FunctionIter,        /* 20 */
  FunctionHasNext,     /* 21 */
  FunctionNext,        /* 22 */
  FunctionDecr,        /* 23 */
  FunctionIncr,        /* 24 */
  FunctionVisit,       /* 25 */
  FunctionReduce,      /* 26 */
  FunctionIs,          /* 27 */
  FunctionQuery,       /* 28 */
  FunctionEnter,       /* 29 */
  FunctionLeave,       /* 30 */
  FunctionPush,        /* 31 */
  FunctionPop,         /* 32 */
  FunctionConstructor, /* 33 */
  FunctionEndOfListDummy
} vtable_id_t;

typedef struct _data * (*method_t)(struct _data *, char *, array_t *, dict_t *);

typedef struct _interface {
  int     type;
  char   *name;
  dict_t *methods;
  int    *fncs;
} interface_t;
  
typedef struct _vtable {
  vtable_id_t id;
  void_t      fnc;
} vtable_t;

typedef struct _methoddescr {
  int       type;
  char     *name;
  method_t  method;
  int       argtypes[MAX_METHOD_PARAMS];
  int       minargs;
  int       maxargs;
  int       varargs;
} methoddescr_t;

typedef struct _typedescr {
  int            type;
  char          *type_name;
  vtable_t      *vtable;
  dict_t        *methods;
  methoddescr_t *constructor;
  void          *ptr;
  int            promote_to;
  int            inherits[MAX_INHERITS];
  unsigned int   hash;
  char          *str;
  int            count;
} typedescr_t;

extern int             interface_register(int, char *, int, ...);
extern interface_t *   interface_get(int);
extern void            interface_register_method(interface_t *, methoddescr_t *);
extern methoddescr_t * interface_get_method(interface_t *, char *);

extern vtable_t *      vtable_build(vtable_t[]);
extern void            vtable_dump(vtable_t *);
extern void_t          vtable_get(vtable_t *, int);
extern int             vtable_implements(vtable_t *, int);

extern int             typedescr_register(typedescr_t *);
extern int             typedescr_register_type(typedescr_t *, methoddescr_t *);
extern void            typedescr_register_types(typedescr_t *);
extern int             typedescr_create_and_register(int, char *, vtable_t *, methoddescr_t *);
extern typedescr_t *   typedescr_register_functions(typedescr_t *, vtable_t[]);
extern typedescr_t *   typedescr_register_function(typedescr_t *, int, void_t);
extern typedescr_t *   typedescr_get(int);
extern typedescr_t *   typedescr_get_byname(char *);
extern void            typedescr_count(void);
extern unsigned int    typedescr_hash(typedescr_t *);
extern void            typedescr_register_methods(methoddescr_t[]);
extern void            typedescr_register_method(typedescr_t *, methoddescr_t *);
extern methoddescr_t * typedescr_get_method(typedescr_t *, char *);
extern void_t          typedescr_get_function(typedescr_t *, int);
extern char *          typedescr_tostring(typedescr_t *);
extern int             typedescr_is(typedescr_t *, int);
extern void            typedescr_dump_vtable(typedescr_t *);
  
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __TYPEDESCR_H__ */
