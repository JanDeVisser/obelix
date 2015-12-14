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
#define __INCLUDING_TYPEDESCR_H__

#include <array.h>
#include <data.h>
#include <dict.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_METHOD_PARAMS      3
#define MAX_INHERITS           3

typedef enum _metatype {
  NoType          = 0,
  Dynamic         = 100,
  FirstInterface  = 1000, /* Marker */
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
  FunctionNone           = 0,
  FunctionFactory,      /* 1  */
  FunctionNew,          /* 2  */
  FunctionCopy,         /* 3  */
  FunctionCmp,          /* 4  */
  FunctionFreeData,     /* 5  */
  FunctionFree,         /* 6  */
  FunctionToString,     /* 7  */
  FunctionStaticString, /* 8  */
  FunctionAllocString,  /* 9  */
  FunctionFltValue,     /* 10  */
  FunctionIntValue,     /* 11 */
  FunctionParse,        /* 12 */
  FunctionCast,         /* 13 */
  FunctionHash,         /* 14 */
  FunctionLen,          /* 15 */
  FunctionResolve,      /* 16 */
  FunctionCall,         /* 17 */
  FunctionSet,          /* 18 */
  FunctionRead,         /* 19 */
  FunctionWrite,        /* 20 */
  FunctionOpen,         /* 21 */
  FunctionIter,         /* 22 */
  FunctionHasNext,      /* 23 */
  FunctionNext,         /* 24 */
  FunctionDecr,         /* 25 */
  FunctionIncr,         /* 26 */
  FunctionVisit,        /* 27 */
  FunctionReduce,       /* 28 */
  FunctionIs,           /* 29 */
  FunctionQuery,        /* 30 */
  FunctionEnter,        /* 31 */
  FunctionLeave,        /* 32 */
  FunctionPush,         /* 33 */
  FunctionPop,          /* 34 */
  FunctionConstructor,  /* 35 */
  FunctionInterpolate,  /* 36 */
  FunctionEndOfListDummy
} vtable_id_t;

typedef struct _data * (*method_t)(struct _data *, char *, array_t *, dict_t *);

typedef struct _interface {
  data_t  _d;
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
  data_t    _d;
  int       type;
  char     *name;
  method_t  method;
  int       argtypes[MAX_METHOD_PARAMS];
  int       minargs;
  int       maxargs;
  int       varargs;
} methoddescr_t;

typedef struct _typedescr {
  data_t         _d;
  int            type;
  char          *type_name;
  vtable_t      *vtable;
  dict_t        *methods;
  methoddescr_t *constructor;
  void          *ptr;
  int            promote_to;
  int            inherits[MAX_INHERITS];
  unsigned int   hash;
  int            count;
} typedescr_t;

OBLCORE_IMPEXP data_t *        type_get(int);
OBLCORE_IMPEXP data_t *        type_get_byname(char *);

OBLCORE_IMPEXP int             interface_register(int, char *, int, ...);
OBLCORE_IMPEXP interface_t *   interface_get(int);
OBLCORE_IMPEXP interface_t *   interface_get_byname(char *);
OBLCORE_IMPEXP void            interface_register_method(interface_t *,
							 methoddescr_t *);
OBLCORE_IMPEXP methoddescr_t * interface_get_method(interface_t *, char *);

OBLCORE_IMPEXP vtable_t *      vtable_build(vtable_t[]);
OBLCORE_IMPEXP void            vtable_dump(vtable_t *);
OBLCORE_IMPEXP void_t          vtable_get(vtable_t *, int);
OBLCORE_IMPEXP int             vtable_implements(vtable_t *, int);

OBLCORE_IMPEXP int             typedescr_register(typedescr_t *);
OBLCORE_IMPEXP int             typedescr_register_type(typedescr_t *,
						       methoddescr_t *);
OBLCORE_IMPEXP void            typedescr_register_types(typedescr_t *);
OBLCORE_IMPEXP int             typedescr_create_and_register(int, char *,
							     vtable_t *,
							     methoddescr_t *);
OBLCORE_IMPEXP typedescr_t *   typedescr_register_functions(typedescr_t *, vtable_t[]);
OBLCORE_IMPEXP typedescr_t *   typedescr_register_function(typedescr_t *, int, void_t);
OBLCORE_IMPEXP typedescr_t *   typedescr_assign_inheritance(typedescr_t *, int);
OBLCORE_IMPEXP typedescr_t *   typedescr_get(int);
OBLCORE_IMPEXP typedescr_t *   typedescr_get_byname(char *);
OBLCORE_IMPEXP void            typedescr_count(void);
OBLCORE_IMPEXP unsigned int    typedescr_hash(typedescr_t *);
OBLCORE_IMPEXP void            typedescr_register_methods(methoddescr_t[]);
OBLCORE_IMPEXP void            typedescr_register_method(typedescr_t *, methoddescr_t *);
OBLCORE_IMPEXP methoddescr_t * typedescr_get_method(typedescr_t *, char *);
OBLCORE_IMPEXP void_t          typedescr_get_function(typedescr_t *, int);
OBLCORE_IMPEXP int             typedescr_is(typedescr_t *, int);
OBLCORE_IMPEXP void            typedescr_dump_vtable(typedescr_t *);

#define data_is_type(d)       ((d) && (data_hastype((d), Type)))
#define data_as_type(d)       ((typedescr_t *) (data_is_type((d)) ? ((typedescr_t *) (d)) : NULL))
#define type_tostring(s)      (data_tostring((data_t *) (s)))
#define type_copy(s)          ((typedescr_t *) data_copy((data_t *) (s)))
  
#define data_is_interface(d)  ((d) && (data_hastype((d), Interface)))
#define data_as_interface(d)  ((interface_t *) (data_is_interface((d)) ? ((interface_t *) (d)) : NULL))
#define interface_tostring(s) (data_tostring((data_t *) (s)))
#define interface_copy(s)     ((interface_t *) data_copy((data_t *) (s)))

#define data_is_method(d)     ((d) && (data_hastype((d), Method)))
#define data_as_method(d)     ((methoddescr_t *) (data_is_method((d)) ? ((methoddescr_t *) (d)) : NULL))
#define method_tostring(s)    (data_tostring((data_t *) (s)))
#define method_copy(s)        ((methoddescr_t *) data_copy((data_t *) (s)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#undef __INCLUDING_TYPEDESCR_H__

#endif /* __TYPEDESCR_H__ */
