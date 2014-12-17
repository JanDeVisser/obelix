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

typedef enum _datatype {
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
  Closure
} datatype_t;

typedef struct _data {
  datatype_t type;
  union {
    void      *ptrval;
    long       intval;
    double     dblval;
  };
} data_t;

typedef data_t * (*method_t)(data_t *, char *, array_t *, dict_t *);

typedef enum _errorcode {
  ErrorArgCount,
  ErrorType,
  ErrorName,
  ErrorNotCallable
} errorcode_t;

typedef struct _error {
  errorcode_t  code;
  char        *msg;
  char        *str;
} error_t;

typedef struct _typedescr {
  int           type;
  char         *typecode;
  new_t         new;
  copydata_t    copy;
  cmp_t         cmp;
  free_t        free;
  tostring_t    tostring;
  parse_t       parse;
  hash_t        hash;
  dict_t       *methods;
  method_t      fallback;
} typedescr_t;

extern int            error_register(char *str);
extern error_t *      error_create(int , ...);
extern error_t *      error_vcreate(int, va_list);
extern error_t *      error_copy(error_t *);
extern void           error_free(error_t *);
extern unsigned int   error_hash(error_t *);
extern int            error_cmp(error_t *, error_t *);
extern char *         error_tostring(error_t *);
extern void           error_report(error_t *);

extern int            typedescr_register(typedescr_t *);
extern typedescr_t *  typedescr_get(int);
extern typedescr_t *  typedescr_add_method(typedescr_t *, char *, method_t);
extern method_t       typedescr_get_method(typedescr_t *, char *);

extern data_t *       data_create(int, ...);
extern data_t *       data_create_pointer(void *);
extern data_t *       data_null(void);
extern data_t *       data_error(int, char *, ...);
extern data_t *       data_create_int(long);
extern data_t *       data_create_float(double);
extern data_t *       data_create_bool(long);
extern data_t *       data_create_string(char *);
extern data_t *       data_create_list(list_t *);
extern data_t *       data_create_list_fromarray(array_t *);
extern array_t *      data_list_toarray(data_t *);
extern data_t *       data_create_function(function_t *);
extern data_t *       data_parse(int, char *);
extern void           data_free(data_t *);
extern int            data_type(data_t *);
extern int            data_is_numeric(data_t *);
extern int            data_is_error(data_t *t);
extern data_t *       data_copy(data_t *);
extern unsigned int   data_hash(data_t *);
extern char *         data_tostring(data_t *);
extern int            data_cmp(data_t *, data_t *);
extern method_t       data_method(data_t *, char *);
extern data_t *       data_execute(data_t *, char *, array_t *, dict_t *);
extern char *         data_debugstr(data_t *);

extern dict_t *       data_add_all_reducer(entry_t *, dict_t *);

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

#define data_array_create(i)    array_set_tostring( \
                                  array_set_free( \
                                    array_create((i)), \
                                    (free_t) data_free), \
                                  (tostring_t) data_tostring)

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
