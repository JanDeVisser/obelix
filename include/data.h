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

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum _datatype {
  Exception = 1,
  Type,
  Interface,
  Method,
  Pointer,
  String,
  Int,
  Float,
  Bool,
  List
} datatype_t;

typedef enum _free_semantics {
  Normal,
  DontFreeData,
  Constant
} free_semantics_t;

typedef struct _data {
  int               type;
  free_semantics_t  free_me;
  free_semantics_t  free_str;
  int               refs;
  char             *str;
} data_t;

typedef struct _pointer {
  data_t  _d;
  void   *ptr;
  int     size;
} pointer_t;

typedef struct _flt {
  data_t _d;
  double dbl;
} flt_t;

typedef struct _int {
  data_t _d;
  long   i;
} int_t;

struct _name;

typedef data_t * (*factory_t)(int, va_list);
typedef data_t * (*cast_t)(data_t *, int);
typedef data_t * (*resolve_name_t)(void *, char *);
typedef data_t * (*call_t)(void *, array_t *, dict_t *);
typedef data_t * (*setvalue_t)(void *, char *, data_t *);
typedef data_t * (*data_fnc_t)(data_t *);
typedef data_t * (*data2_fnc_t)(data_t *, data_t *);

OBLCORE_IMPEXP data_t *            data_create_noinit(int);
OBLCORE_IMPEXP data_t *            data_create(int, ...);
OBLCORE_IMPEXP data_t *            data_settype(data_t *, int);
OBLCORE_IMPEXP data_t *            data_cast(data_t *, int);
OBLCORE_IMPEXP data_t *            data_promote(data_t *);
OBLCORE_IMPEXP data_t *            data_parse(int, char *);
OBLCORE_IMPEXP data_t *            data_decode(char *);
OBLCORE_IMPEXP char *              data_encode(data_t *);
OBLCORE_IMPEXP void                data_free(data_t *);
OBLCORE_IMPEXP int                 data_hastype(data_t *, int);
OBLCORE_IMPEXP struct _typedescr * data_typedescr(data_t *);
OBLCORE_IMPEXP void_t              data_get_function(data_t *, int);
OBLCORE_IMPEXP int                 data_implements(data_t *, int);
OBLCORE_IMPEXP int                 data_is_numeric(data_t *);
OBLCORE_IMPEXP int                 data_is_exception(data_t *t);
OBLCORE_IMPEXP int                 data_is_callable(data_t *);
OBLCORE_IMPEXP int                 data_is_iterable(data_t *);
OBLCORE_IMPEXP int                 data_is_iterator(data_t *);
OBLCORE_IMPEXP data_t *            data_copy(data_t *);
OBLCORE_IMPEXP data_t *            data_uncopy(data_t *);
OBLCORE_IMPEXP unsigned int        data_hash(data_t *);
OBLCORE_IMPEXP data_t *            data_len(data_t *);
OBLCORE_IMPEXP char *              data_tostring(data_t *);
OBLCORE_IMPEXP int                 data_cmp(data_t *, data_t *);
OBLCORE_IMPEXP data_t *            data_call(data_t *, array_t *, dict_t *);
OBLCORE_IMPEXP int                 data_hasmethod(data_t *, char *);
OBLCORE_IMPEXP data_t *            data_method(data_t *, char *);
OBLCORE_IMPEXP data_t *            data_execute(data_t *, char *,
                                                array_t *, dict_t *);
OBLCORE_IMPEXP data_t *            data_resolve(data_t *, struct _name *);
OBLCORE_IMPEXP data_t *            data_invoke(data_t *, struct _name *,
                                               array_t *, dict_t *);
OBLCORE_IMPEXP int                 data_has(data_t *, struct _name *);
OBLCORE_IMPEXP int                 data_has_callable(data_t *, struct _name *);
OBLCORE_IMPEXP data_t *            data_get(data_t *, struct _name *);
OBLCORE_IMPEXP data_t *            data_get_attribute(data_t *, char *);
OBLCORE_IMPEXP data_t *            data_set(data_t *, struct _name *, data_t *);
OBLCORE_IMPEXP data_t *            data_set_attribute(data_t *, char *, data_t *);
OBLCORE_IMPEXP data_t *            data_iter(data_t *);
OBLCORE_IMPEXP data_t *            data_has_next(data_t *);
OBLCORE_IMPEXP data_t *            data_next(data_t *);
OBLCORE_IMPEXP data_t *            data_visit(data_t *, data_t *);
OBLCORE_IMPEXP data_t *            data_reduce(data_t *, data_t *, data_t *);
OBLCORE_IMPEXP data_t *            data_reduce_with_fnc(data_t *, reduce_t, data_t *);
OBLCORE_IMPEXP data_t *            data_read(data_t *, char *, int);
OBLCORE_IMPEXP data_t *            data_write(data_t *, char *, int);
OBLCORE_IMPEXP data_t *            data_push(data_t *, data_t *);
OBLCORE_IMPEXP data_t *            data_pop(data_t *);
OBLCORE_IMPEXP int                 data_count(void);
OBLCORE_IMPEXP data_t *            data_interpolate(data_t *, array_t *, dict_t *);
OBLCORE_IMPEXP data_t *            data_query(data_t *, data_t *);

OBLCORE_IMPEXP int_t *             int_create(intptr_t);
OBLCORE_IMPEXP int_t *             bool_get(long);
OBLCORE_IMPEXP flt_t *             flt_create(double);
OBLCORE_IMPEXP pointer_t *         ptr_create(int, void *);
OBLCORE_IMPEXP double              data_floatval(data_t *);
OBLCORE_IMPEXP int                 data_intval(data_t *);

#define data_new(dt,st)     ((st *) data_settype((data_t *) _new(sizeof(st)), dt))
#define data_type(d)        ((d) ? ((data_t *) (d)) -> type : 0)
#define data_typename(d)    ((d) ? (typedescr_get(data_type((data_t *) (d))) -> type_name) : "null")

#define data_is_int(d )     ((d) && (data_hastype((d), Int)))
#define data_is_float(d )   ((d) && (data_hastype((d), Float)))
#define data_is_string(d )  ((d) && (data_hastype((d), String)))
#define data_is_pointer(d)  ((d) && (data_hastype((d), Pointer)))
#define data_as_pointer(d)  (data_is_pointer((data_t *) (d)) ? (pointer_t *) (d) : NULL)
#define data_unwrap(d)      (data_is_pointer((data_t *) (d)) ? (data_as_pointer(d) -> ptr) : NULL)

#define data_is_list(d)     ((d) && (data_hastype((d), List)))
#define data_as_array(d)    ((array_t *) (((pointer_t *) (d)) -> ptr))

OBLCORE_IMPEXP array_t *       data_add_all_reducer(data_t *, array_t *);
OBLCORE_IMPEXP array_t *       data_add_all_as_data_reducer(char *, array_t *);
OBLCORE_IMPEXP array_t *       data_add_strings_reducer(data_t *, array_t *);
OBLCORE_IMPEXP dict_t *        data_put_all_reducer(entry_t *, dict_t *);

OBLCORE_IMPEXP data_t *        data_null(void);
OBLCORE_IMPEXP int             data_isnull(data_t *);
OBLCORE_IMPEXP int             data_notnull(data_t *);
OBLCORE_IMPEXP data_t *        data_create_list(array_t *);
OBLCORE_IMPEXP array_t *       data_list_copy(data_t *);
OBLCORE_IMPEXP array_t *       data_list_to_str_array(data_t *);
OBLCORE_IMPEXP data_t *        data_str_array_to_list(array_t *);
OBLCORE_IMPEXP data_t *        data_list_push(data_t *, data_t *);
OBLCORE_IMPEXP data_t *        data_list_get(data_t *, int);
OBLCORE_IMPEXP int             data_list_size(data_t *);

OBLCORE_IMPEXP int_t *         bool_true;
OBLCORE_IMPEXP int_t *         bool_false;

OBLCORE_IMPEXP type_t          type_data[1];

#define int_as_bool(i)         ((data_t *) bool_get((i)))
#define int_to_data(i)         ((data_t *) int_create((i)))
#define flt_to_data(f)         ((data_t *) flt_create((f)))
#define ptr_to_data(s, p)      ((data_t *) ptr_create((s), (p)))

#define data_true()    ((data_t *) bool_true)
#define data_false()   ((data_t *) bool_false)

/* -- Standard vtable functions ------------------------------------------ */

/* ----------------------------------------------------------------------- */

#define strdata_dict_create()  (dict_set_data_type( \
                                 dict_set_key_type( \
                                   dict_create(NULL), \
                                   type_str), \
                                 type_data))

#define intdata_dict_create()  (dict_set_data_type( \
                                 dict_set_key_type( \
                                   dict_create(NULL), \
                                   type_int), \
                                 type_data))

#define datadata_dict_create() (dict_set_data_type( \
                                 dict_set_key_type( \
                                   dict_create(NULL), \
                                   type_data), \
                                 type_data))

#define data_dict_get(d, k)    ((data_t *) dict_get((d), (k)))

#define data_array_create(i)   (array_set_type(array_create((i)), type_data))
#define data_array_get(a, i)   ((data_t *) array_get((a), (i)))

#define data_list_create()     (list_set_type(list_create(), type_data))
#define data_list_pop(l)       (data_t *) list_pop((l))
#define data_list_shift(l)     (data_t *) list_shift((l))

#define data_set_create()      (set_set_type(set_create(NULL), type_data))
\
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#ifndef __INCLUDING_TYPEDESCR_H__
#include <typedescr.h>
#endif /* __INCLUDING_TYPEDESCR_H__ */

#endif /* __DATA_H__ */
