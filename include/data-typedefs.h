/*
 * /obelix/include/data-typedefs.h - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __DATA_TYPEDEFS_H__
#define __DATA_TYPEDEFS_H__

#include <core.h>
#include <array.h>
#include <dict.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_METHOD_PARAMS      3
#define MAX_INHERITS           3

/* -- E N U M E R A T I O N S --------------------------------------------- */

typedef enum _datatype {
  Exception    = 1,
  Kind,       /* 2 */
  Type,       /* 3 */
  Interface,  /* 4 */
  Method,     /* 5 */
  Pointer,    /* 6 */
  String,     /* 7 */
  Int,        /* 8 */
  Float,      /* 9 */
  Bool,       /* 10 */
  List,       /* 11 */
  Mutex,      /* 12 */
  Condition,  /* 13 */
  Thread,     /* 14 */
} datatype_t;

typedef enum _free_semantics {
  Normal,
  DontFreeData,
  Constant
} free_semantics_t;

typedef enum _metatype {
  NoType          = 0,
  Dynamic         = 15,
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

/* -- S T R U C T S ------------------------------------------------------- */

typedef struct _data {
#ifndef NDEBUG
  unsigned short int  cookie;
#endif /* !NDEBUG */
  int                 type;
  unsigned int        hash;
  free_semantics_t    free_me;
  free_semantics_t    free_str;
  int                 refs;
  char               *str;
} data_t;

typedef struct _pointer {
  data_t  _d;
  void   *ptr;
  size_t  size;
} pointer_t;

typedef pointer_t datalist_t;

typedef struct _dictionary {
  data_t  _d;
  dict_t *attributes;
} dictionary_t;

typedef struct _arguments {
  data_t        _d;
  datalist_t   *args;
  dictionary_t *kwargs;
} arguments_t;

/* ------------------------------------------------------------------------ */

typedef data_t * (*factory_t)(int, va_list);
typedef data_t * (*cast_t)(data_t *, int);
typedef data_t * (*resolve_name_t)(void *, char *);
typedef data_t * (*call_t)(void *, arguments_t *);
typedef data_t * (*setvalue_t)(void *, char *, data_t *);
typedef data_t * (*data_fnc_t)(data_t *);
typedef data_t * (*data2_fnc_t)(data_t *, data_t *);
typedef data_t * (*data_valist_t)(data_t *, va_list);
typedef data_t * (*method_t)(data_t *, char *, arguments_t *);

/* ------------------------------------------------------------------------ */

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

typedef struct _accessor {
  char           *name;
  setvalue_t      setter;
  resolve_name_t  resolver;
} accessor_t;

typedef struct _typedescr {
  kind_t    _d;
  size_t    size;
  int       debug;
  vtable_t *vtable;
  vtable_t *inherited_vtable;
  dict_t   *accessors;
  void_t   *constructors;
  void     *ptr;
  int       promote_to;
  int      *ancestors;
  int      *implements;
  size_t    implements_sz;
  int       count;
  int      *inherits;
} typedescr_t;

typedef struct _flt {
  data_t _d;
  double dbl;
} flt_t;

typedef struct _int {
  data_t _d;
  long   i;
} int_t;

typedef struct _name {
  data_t   _d;
  array_t *name;
  char    *sep;
} name_t;


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

/* ------------------------------------------------------------------------ */

#define type_skel(id, code, type)                                            \
  static inline int data_is_ ## id(void *d) {                                \
    return data_hastype(d, code);                                            \
  }                                                                          \
  static inline type * data_as_ ## id(void *d) {                             \
    return (data_is_ ## id(d)) ? (type *) d : NULL;                          \
  }                                                                          \
  static inline void id ## _free(type *d) {                                  \
    data_free((data_t *) d);                                                 \
  }                                                                          \
  static inline char * id ## _tostring(type *d) {                            \
    return data_tostring((data_t *) d);                                      \
  }                                                                          \
  static inline type * id ## _copy(type *d) {                                \
    return (type *) data_copy((data_t *) d);                                 \
  }

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __DATA_TYPEDEFS_H__ */
