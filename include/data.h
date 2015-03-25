/*
 * /obelix/include/data.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __DATA_H__
#define __DATA_H__

#include <core.h>
#include <array.h>
#include <dict.h>
#include <name.h>

typedef enum _datatype {
  NoType         = 0,
  Error,        /*  1 */
  Pointer,      /*  2 */
  String,       /*  3 */
  Number,       /*  4 */
  Int,          /*  5 */
  Float,        /*  6 */
  Bool,         /*  7 */
  List,         /*  8 */
  Function,     /*  9 */
  Method,       /* 10 */
  Object,       /* 11 */
  Script,       /* 12 */
  Closure,      /* 13 */
  Module,       /* 14 */
  Name,         /* 15 */
  Native,       /* 16 */
  NVP,          /* 17 */
  Range,        /* 18 */
  Any           = 100,
  UserTypeRange = 999
} datatype_t;

typedef enum _vtable_id {
  FunctionNone        = 0,
  FunctionNew,       /* 1  */
  FunctionCopy,      /* 2  */
  FunctionCmp,       /* 3  */
  FunctionFree,      /* 4  */
  FunctionToString,  /* 5  */
  FunctionFltValue,  /* 6  */
  FunctionIntValue,  /* 7  */
  FunctionParse,     /* 8  */
  FunctionCast,      /* 9  */
  FunctionHash,      /* 10 */
  FunctionLen,       /* 11 */
  FunctionResolve,   /* 12 */
  FunctionCall,      /* 13 */
  FunctionSet,       /* 14 */
  FunctionRead,      /* 15 */
  FunctionWrite,     /* 16 */
  FunctionOpen,      /* 17 */
  FunctionIter,      /* 18 */
  FunctionHasNext,   /* 19 */
  FunctionNext,      /* 20 */
  FunctionDecr,      /* 21 */
  FunctionIncr,      /* 22 */
  FunctionEndOfListDummy
} vtable_id_t;

typedef struct _data {
  int          type;
  union {
    void      *ptrval;
    long       intval;
    double     dblval;
  };
  int          size;
  int          refs;
  dict_t      *methods;
  char        *str;
  char        *debugstr;
} data_t;

typedef data_t * (*cast_t)(data_t *, int);
typedef data_t * (*method_t)(data_t *, char *, array_t *, dict_t *);
typedef data_t * (*resolve_name_t)(data_t *, char *);
typedef data_t * (*call_t)(data_t *, array_t *, dict_t *);
typedef data_t * (*setvalue_t)(data_t *, char *, data_t *);
typedef data_t * (*data_fnc_t)(data_t *);

typedef struct _vtable {
  vtable_id_t id;
  void_t      fnc;
} vtable_t;

typedef struct _typedescr {
  int           type;
  char         *type_name;
  vtable_t     *vtable;
  dict_t       *methods;
  int           promote_to;
  int           inherits_size;
  int          *inherits;
  unsigned int  hash;
  char         *str;
  int           count;
} typedescr_t;

#define MAX_METHOD_PARAMS      3

typedef struct _methoddescr {
  int       type;
  char     *name;
  method_t  method;
  int       argtypes[MAX_METHOD_PARAMS];
  int       minargs;
  int       varargs;
} methoddescr_t;

extern int             typedescr_register(typedescr_t *);
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

extern data_t *        data_create(int, ...);
extern data_t *        data_cast(data_t *, int);
extern data_t *        data_promote(data_t *);
extern data_t *        data_parse(int, char *);
extern data_t *        data_decode(char *);
extern void            data_free(data_t *);
extern int             data_type(data_t *);
extern int             data_hastype(data_t *, int);
extern typedescr_t *   data_typedescr(data_t *);
extern void_t          data_get_function(data_t *, int);
extern int             data_is_numeric(data_t *);
extern int             data_is_error(data_t *t);
extern int             data_is_callable(data_t *);
extern int             data_is_iterable(data_t *);
extern int             data_is_iterator(data_t *);
extern data_t *        data_copy(data_t *);
extern unsigned int    data_hash(data_t *);
extern char *          data_tostring(data_t *);
extern int             data_cmp(data_t *, data_t *);
extern data_t *        data_call(data_t *, array_t *, dict_t *);
extern data_t *        data_method(data_t *, char *);
extern data_t *        data_execute(data_t *, char *, array_t *, dict_t *);
extern data_t *        data_resolve(data_t *, name_t *);
extern data_t *        data_invoke(data_t *, name_t *, array_t *, dict_t *);
extern int             data_has(data_t *, name_t *);
extern int             data_has_callable(data_t *, name_t *);
extern data_t *        data_get(data_t *, name_t *);
extern data_t *        data_set(data_t *, name_t *, data_t *);
extern data_t *        data_iter(data_t *);
extern data_t *        data_has_next(data_t *);
extern data_t *        data_next(data_t *);
extern int             data_count(void);

extern double          data_floatval(data_t *);
extern int             data_intval(data_t *);

#define data_debugstr(d) (data_tostring((d)))
#define data_charval(d)  ((char *) (d) -> ptrval)
#define data_arrayval(d) ((array_t *) (d) -> ptrval)
#define data_is_error(d) ((d) && (data_type((d)) == Error))
#define data_errorval(d) ((error_t *) ((data_is_error((d)) ? (d) -> ptrval : NULL)))

extern array_t *       data_add_all_reducer(data_t *, array_t *);
extern array_t *       data_add_all_as_data_reducer(char *, array_t *);
extern array_t *       data_add_strings_reducer(data_t *, array_t *);
extern dict_t *        data_put_all_reducer(entry_t *, dict_t *);

extern data_t *        data_create_pointer(int, void *);
extern data_t *        data_null(void);
extern data_t *        data_error(int, char *, ...);
extern data_t *        data_exception(data_t *);
extern data_t *        data_create_list(array_t *);
extern array_t *       data_list_copy(data_t *);
extern array_t *       data_list_to_str_array(data_t *);
extern data_t *        data_str_array_to_list(array_t *);

/* FIXME Needs a better home */
extern str_t *         format(char *, array_t *, dict_t *);

#define strdata_dict_create()   dict_set_tostring_data( \
                                  dict_set_tostring_key( \
                                    dict_set_free_data( \
                                      dict_set_free_key( \
                                        dict_set_hash( \
                                          dict_create((cmp_t) strcmp),\
                                          (hash_t) strhash), \
                                        (free_t) free), \
                                      (free_t) data_free), \
                                    (tostring_t) chars), \
                                  (tostring_t) data_tostring)
#define datadata_dict_create()   dict_set_tostring_data( \
                                  dict_set_tostring_key( \
                                    dict_set_free_data( \
                                      dict_set_free_key( \
                                        dict_set_hash( \
                                          dict_create((cmp_t) strcmp),\
                                          (hash_t) data_hash), \
                                        (free_t) data_free), \
                                      (free_t) data_free), \
                                    (tostring_t) data_tostring), \
                                  (tostring_t) data_tostring)
#define data_dict_get(d, k)     ((data_t *) dict_get((d), (k)))

#define data_array_create(i)    array_set_tostring( \
                                  array_set_free( \
                                    array_create((i)), \
                                    (free_t) data_free), \
                                  (tostring_t) data_tostring)
#define data_array_get(a, i)    ((data_t *) array_get((a), (i)))

#define data_list_create()     _list_set_tostring( \
                                 _list_set_hash( \
                                   _list_set_cmp( \
                                     _list_set_free( \
                                       list_create(), \
                                       (free_t) data_free), \
                                     (cmp_t) data_cmp), \
                                   (hash_t) data_hash), \
                                 (tostring_t) data_tostring)

#endif /* __DATA_H__ */
