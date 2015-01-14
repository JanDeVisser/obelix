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

#ifndef NDEBUG
extern int data_count;
#endif

typedef enum _datatype {
  NoType = -1,
  Error,
  Pointer,
  String,
  Int,
  Float,
  Bool,
  List,
  Function,
  Object,
  Script,
  Closure,
  Module,
  Any = 100,
  Numeric
} datatype_t;

typedef struct _data {
  int          type;
  union {
    void      *ptrval;
    long       intval;
    double     dblval;
  };
  int          size;
  int          refs;
  char        *str;
  char        *debugstr;
} data_t;

typedef data_t * (*cast_t)(data_t *, int);
typedef data_t * (*method_t)(data_t *, char *, array_t *, dict_t *);

typedef struct _typedescr {
  int           type;
  char         *typecode;
  char         *typename;
  new_t         new;
  copydata_t    copy;
  cmp_t         cmp;
  free_t        free;
  tostring_t    tostring;
  parse_t       parse;
  cast_t        cast;
  hash_t        hash;
  dict_t       *methods;
  int           promote_to;
  method_t      fallback;
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

extern typedescr_t typedescr_int;
extern typedescr_t typedescr_bool;
extern typedescr_t typedescr_float;
extern typedescr_t typedescr_str;
extern typedescr_t typedescr_ptr;
extern typedescr_t typedescr_fnc;
extern typedescr_t typedescr_list;
extern typedescr_t typedescr_error;

extern methoddescr_t methoddescr_int[];
extern methoddescr_t methoddescr_bool[];
extern methoddescr_t methoddescr_float[];
extern methoddescr_t methoddescr_str[];
extern methoddescr_t methoddescr_ptr[];
extern methoddescr_t methoddescr_list[];
extern methoddescr_t methoddescr_fnc[];
/* extern methoddescr_t methoddescr_error[]; */


extern int             typedescr_register(typedescr_t);
extern typedescr_t *   typedescr_get(int);
extern void            typedescr_register_methods(methoddescr_t methods[]);
extern void            typedescr_register_method(typedescr_t *, methoddescr_t *method);
extern methoddescr_t * typedescr_get_method(typedescr_t *, char *);
extern char *          typedescr_tostring(typedescr_t *);

extern data_t *        data_create(int, ...);
extern data_t *        data_cast(data_t *, int);
extern data_t *        data_promote(data_t *);
extern data_t *        data_parse(int, char *);
extern void            data_free(data_t *);
extern int             data_type(data_t *);
extern int             data_hastype(data_t *, int);
extern typedescr_t *   data_typedescr(data_t *);
extern int             data_is_numeric(data_t *);
extern int             data_is_error(data_t *t);
extern data_t *        data_copy(data_t *);
extern unsigned int    data_hash(data_t *);
extern char *          data_tostring(data_t *);
extern int             data_cmp(data_t *, data_t *);
extern methoddescr_t * data_method(data_t *, char *);
extern data_t *        data_execute_method(data_t *, methoddescr_t *, array_t *, dict_t *);
extern data_t *        data_execute(data_t *, char *, array_t *, dict_t *);
extern char *          data_debugstr(data_t *);

#define data_dblval(d)   ((data_type((d)) == Float) ? (double)((d) -> intval) : (d) -> dblval)
#define data_longval(d)  ((data_type((d)) == Int) ? (d) -> intval : (long)((d) -> dblval))
#define data_charval(d)  ((char *) (d) -> ptrval)
#define data_arrayval(d) ((array_t *) (d) -> ptrval)
#define data_errorval(d) ((error_t *) ((data_is_error((d)) ? (d) -> ptrval : NULL)))

extern array_t *       data_add_all_reducer(data_t *, array_t *);
extern array_t *       data_add_strings_reducer(data_t *, array_t *);
extern dict_t *        data_put_all_reducer(entry_t *, dict_t *);

extern data_t *        data_create_pointer(int, void *);
extern data_t *        data_null(void);
extern data_t *        data_error(int, char *, ...);
extern data_t *        data_create_list(array_t *);
extern array_t *       data_list_copy(data_t *);
extern array_t *       data_list_to_str_array(data_t *);


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
