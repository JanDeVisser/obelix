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
  Dynamic         = 12,
  FirstInterface  = 1000, /* Marker */
  Number,        /* 1001 */
  InputStream,   /* 1002 */
  OutputStream,  /* 1003 */
  Iterable,      /* 1004 */
  Iterator,      /* 1005 */
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
  FunctionEncode,       /* 12 */
  FunctionParse,        /* 13 */
  FunctionSerialize,    /* 14 */
  FunctionDeserialize,  /* 15 */
  FunctionCast,         /* 16 */
  FunctionHash,         /* 17 */
  FunctionLen,          /* 18 */
  FunctionResolve,      /* 19 */
  FunctionCall,         /* 20 */
  FunctionSet,          /* 21 */
  FunctionRead,         /* 22 */
  FunctionWrite,        /* 23 */
  FunctionOpen,         /* 24 */
  FunctionIter,         /* 25 */
  FunctionHasNext,      /* 26 */
  FunctionNext,         /* 27 */
  FunctionDecr,         /* 28 */
  FunctionIncr,         /* 29 */
  FunctionVisit,        /* 30 */
  FunctionReduce,       /* 31 */
  FunctionIs,           /* 32 */
  FunctionQuery,        /* 33 */
  FunctionEnter,        /* 34 */
  FunctionLeave,        /* 35 */
  FunctionPush,         /* 36 */
  FunctionPop,          /* 37 */
  FunctionConstructor,  /* 38 */
  FunctionInterpolate,  /* 39 */
  FunctionUsr1,         /* 40 */
  FunctionUsr2,         /* 41 */
  FunctionUsr3,         /* 42 */
  FunctionUsr4,         /* 43 */
  FunctionUsr5,         /* 44 */
  FunctionUsr6,         /* 45 */
  FunctionUsr7,         /* 46 */
  FunctionUsr8,         /* 47 */
  FunctionUsr9,         /* 48 */
  FunctionUsr10,        /* 49 */
  FunctionEndOfListDummy
} vtable_id_t;

typedef struct _data * (*method_t)(struct _data *, char *, array_t *, dict_t *);

typedef struct _kind {
  data_t  _d;
  int     type;
  char   *name;
  dict_t *methods;
} kind_t;

typedef struct _interface {
  kind_t _d;
  int    fncs[];
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
  int       minargs;
  int       maxargs;
  int       varargs;
  int       argtypes[MAX_METHOD_PARAMS];
} methoddescr_t;

typedef struct _typedescr {
  kind_t    _d;
  int       size;
  int       debug;
  vtable_t *vtable;
  vtable_t *inherited_vtable;
  void_t   *constructors;
  void     *ptr;
  int       promote_to;
  int      *ancestors;
  int      *implements;
  int       implements_sz;
  int       count;
  int      *inherits;
} typedescr_t;

OBLCORE_IMPEXP void            typedescr_init(void);
OBLCORE_IMPEXP kind_t *        kind_get(int);
OBLCORE_IMPEXP kind_t *        kind_get_byname(char *);

OBLCORE_IMPEXP void            kind_register_method(kind_t *, methoddescr_t *);
OBLCORE_IMPEXP methoddescr_t * kind_get_method(kind_t *, char *);

OBLCORE_IMPEXP int                       _interface_register(int, char *, int, ...);
OBLCORE_IMPEXP interface_t *             interface_get(int);
OBLCORE_IMPEXP interface_t *             interface_get_byname(char *);
#define interface_register_method(i, m)  kind_register_method((kind_t *) (i), (m))
#define interface_get_method(i, m)       kind_get_method((kind_t *) (i), (m))

OBLCORE_IMPEXP vtable_t *      vtable_build(vtable_t[]);
OBLCORE_IMPEXP void            vtable_dump(vtable_t *);
OBLCORE_IMPEXP void_t          vtable_get(vtable_t *, int);
OBLCORE_IMPEXP int             vtable_implements(vtable_t *, int);

OBLCORE_IMPEXP int             _typedescr_register(int, char *, vtable_t *, methoddescr_t *);
OBLCORE_IMPEXP typedescr_t *   typedescr_assign_inheritance(int, int);
OBLCORE_IMPEXP typedescr_t *   typedescr_register_function(typedescr_t *, int, void_t);
OBLCORE_IMPEXP typedescr_t *   typedescr_get(int);
OBLCORE_IMPEXP typedescr_t *   typedescr_get_byname(char *);
OBLCORE_IMPEXP void            typedescr_count(void);
OBLCORE_IMPEXP unsigned int    typedescr_hash(typedescr_t *);
OBLCORE_IMPEXP void            typedescr_register_methods(int, methoddescr_t[]);
OBLCORE_IMPEXP int             typedescr_implements(typedescr_t *, int);
OBLCORE_IMPEXP int             typedescr_inherits(typedescr_t *, int);
OBLCORE_IMPEXP int             typedescr_is(typedescr_t *, int);
OBLCORE_IMPEXP void            typedescr_dump_vtable(typedescr_t *);
OBLCORE_IMPEXP methoddescr_t * typedescr_get_method(typedescr_t *, char *);

#define typename(t)                        ((t) ? ((t) -> _d.name) : "")
#define typetype(t)                        ((t) ? (t) -> _d.type : -1)
#define typedescr_get_local_function(t, f) (((t) && (t) -> vtable) ? (t) -> vtable[(f)].fnc : NULL)
#define typedescr_get_function(t, f)       (((t) && (t) -> inherited_vtable) ? (t) -> inherited_vtable[(f)].fnc : NULL)
#define typedescr_register_method(t, m)    kind_register_method((kind_t *) (t), (m))
#define typedescr_constructors(t)          ((t) -> constructors)

#define data_is_typedescr(d)   ((d) && (data_hastype((d), Type)))
#define data_as_typedescr(d)   ((typedescr_t *) (data_is_typedescr((d)) ? ((typedescr_t *) (d)) : NULL))
#define typedescr_tostring(s)  (data_tostring((data_t *) (s)))
#define typedescr_copy(s)      ((typedescr_t *) data_copy((data_t *) (s)))

#define data_is_interface(d)   ((d) && (data_hastype((d), Interface)))
#define data_as_interface(d)   ((interface_t *) (data_is_interface((d)) ? ((interface_t *) (d)) : NULL))
#define interface_tostring(s)  (data_tostring((data_t *) (s)))
#define interface_copy(s)      ((interface_t *) data_copy((data_t *) (s)))

#define data_is_method(d)      ((d) && (data_hastype((d), Method)))
#define data_as_method(d)      ((methoddescr_t *) (data_is_method((d)) ? ((methoddescr_t *) (d)) : NULL))
#define method_tostring(s)     (data_tostring((data_t *) (s)))
#define method_copy(s)         ((methoddescr_t *) data_copy((data_t *) (s)))

#define typedescr_set_size(d, t)    (typedescr_get((d)) -> size = sizeof(t))

#define typedescr_register(t, type)                                          \
    if (t < 1) {                                                             \
      t = _typedescr_register(t, #t , _vtable_ ## t, NULL);                  \
      typedescr_set_size(t, type);                                           \
    }

#define typedescr_register_with_name(t, name, type)                          \
    if (t < 1) {                                                             \
      t = _typedescr_register(t, name , _vtable_ ## t, NULL);                \
      typedescr_set_size(t, type);                                           \
    }

#define typedescr_register_with_methods(t, type)                             \
  if (t < 1) {                                                               \
    t = _typedescr_register(t, #t , _vtable_ ## t, _methods_ ## t);          \
    typedescr_set_size(t, type);                                             \
  }

#define typedescr_register_with_name_and_methods(t, name, type)              \
  if (t < 1) {                                                               \
    t = _typedescr_register(t, name , _vtable_ ## t, _methods_ ## t);        \
    typedescr_set_size(t, type);                                             \
  }

#define builtin_typedescr_register(t, name, type)                            \
  assert(t > 0);                                                             \
  _typedescr_register(t, name , _vtable_ ## t, _methods_ ## t);              \
  typedescr_set_size(t, type);                                               \

#define builtin_interface_register(i, num, fncs...)                          \
  _interface_register(i, #i , num, ## fncs);

#define interface_register(i, num, fncs...)                                  \
  if (i < 1) {                                                               \
    i = _interface_register(i, #i , num, ## fncs);                           \
  }


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#undef __INCLUDING_TYPEDESCR_H__

#endif /* __TYPEDESCR_H__ */
