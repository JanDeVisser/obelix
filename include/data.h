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
#include <typedescr.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum _datatype {
  Exception       = 1,
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
  BoundMethod,  /* 13 */
  Closure,      /* 14 */
  Module,       /* 15 */
  Name,         /* 16 */
  Native,       /* 17 */
  NVP,          /* 18 */
  Range,        /* 19 */
  Thread,       /* 20 */
  Mutex         /* 21 */
  /* 
   * Any must be the last type. Therefore, after adding types here Any and 
   * Dynamic in typedescr must be updated as well.
   * 
   * FIXME this link between this enum and the constants in typedescr should 
   * be fixed.
   */
} datatype_t;

typedef struct _data {
  int          type;
  int          free_me;
  union {
    void      *ptrval;
    long       intval;
    double     dblval;
  };
  int          refs;
  char        *str;
} data_t;

typedef data_t * (*factory_t)(int, va_list);
typedef data_t * (*cast_t)(data_t *, int);
typedef data_t * (*resolve_name_t)(void *, char *);
typedef data_t * (*call_t)(void *, array_t *, dict_t *);
typedef data_t * (*setvalue_t)(void *, char *, data_t *);
typedef data_t * (*data_fnc_t)(data_t *);

extern data_t *        data_create_noinit(int);
extern data_t *        data_create(int, ...);
extern data_t *        data_settype(data_t *, int);
extern data_t *        data_cast(data_t *, int);
extern data_t *        data_promote(data_t *);
extern data_t *        data_parse(int, char *);
extern data_t *        data_decode(char *);
extern void            data_free(data_t *);
extern int             data_hastype(data_t *, int);
extern typedescr_t *   data_typedescr(data_t *);
extern void_t          data_get_function(data_t *, int);
extern int             data_implements(data_t *, int);
extern int             data_is_numeric(data_t *);
extern int             data_is_exception(data_t *t);
extern int             data_is_callable(data_t *);
extern int             data_is_iterable(data_t *);
extern int             data_is_iterator(data_t *);
extern data_t *        data_copy(data_t *);
extern unsigned int    data_hash(data_t *);
extern data_t *        data_len(data_t *);
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
extern data_t *        data_visit(data_t *, data_t *);
extern data_t *        data_reduce(data_t *, data_t *, data_t *);
extern int             data_count(void);

extern double          data_floatval(data_t *);
extern int             data_intval(data_t *);

#define data_type(d)   ((d) -> type)

#define data_charval(d)  ((char *) (d) -> ptrval)
#define data_arrayval(d) ((array_t *) (d) -> ptrval)

extern array_t *       data_add_all_reducer(data_t *, array_t *);
extern array_t *       data_add_all_as_data_reducer(char *, array_t *);
extern array_t *       data_add_strings_reducer(data_t *, array_t *);
extern dict_t *        data_put_all_reducer(entry_t *, dict_t *);

extern data_t *        data_null(void);
extern data_t *        data_true(void);
extern data_t *        data_false(void);
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

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __DATA_H__ */
