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

#include <stdio.h>

#include <core.h>
#include <data-typedefs.h>
#include <array.h>
#include <dict.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef NDEBUG
/* Sentinel value used to identify legitimate data_t structures: */
#define MAGIC_COOKIE               ((unsigned short int) 0xDEADBEEF)
#endif /* !NDEBUG */

OBLCORE_IMPEXP void                data_init(void);
OBLCORE_IMPEXP data_t *            data_create_noinit(int);
OBLCORE_IMPEXP data_t *            data_create(int, ...);
OBLCORE_IMPEXP data_t *            data_settype(data_t *, int);
OBLCORE_IMPEXP data_t *            data_cast(data_t *, int);
OBLCORE_IMPEXP data_t *            data_promote(data_t *);
OBLCORE_IMPEXP data_t *            data_parse(int, char *);
OBLCORE_IMPEXP data_t *            data_decode(char *);
OBLCORE_IMPEXP data_t *            data_deserialize(data_t *);

OBLCORE_IMPEXP char *              data_encode(data_t *);
OBLCORE_IMPEXP data_t *            data_serialize(data_t *);
OBLCORE_IMPEXP void                data_free(data_t *);
OBLCORE_IMPEXP unsigned int        data_hash(data_t *);
OBLCORE_IMPEXP data_t *            data_len(data_t *);
OBLCORE_IMPEXP char *              _data_tostring(data_t *);
OBLCORE_IMPEXP double              _data_floatval(data_t *);
OBLCORE_IMPEXP int                 _data_intval(data_t *);
OBLCORE_IMPEXP int                 data_cmp(data_t *, data_t *);
OBLCORE_IMPEXP data_t *            data_call(data_t *, arguments_t *);
OBLCORE_IMPEXP int                 data_hasmethod(data_t *, char *);
OBLCORE_IMPEXP data_t *            data_method(data_t *, char *);
OBLCORE_IMPEXP data_t *            data_execute(data_t *, char *, arguments_t *);
OBLCORE_IMPEXP data_t *            data_resolve(data_t *, name_t *);
OBLCORE_IMPEXP data_t *            data_invoke(data_t *, name_t *, arguments_t *);
OBLCORE_IMPEXP int                 data_has(data_t *, name_t *);
OBLCORE_IMPEXP int                 data_has_callable(data_t *, name_t *);
OBLCORE_IMPEXP data_t *            data_get(data_t *, name_t *);
OBLCORE_IMPEXP data_t *            data_get_attribute(data_t *, char *);
OBLCORE_IMPEXP data_t *            data_set(data_t *, name_t *, data_t *);
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
OBLCORE_IMPEXP data_t *            data_interpolate(data_t *, arguments_t *);
OBLCORE_IMPEXP data_t *            data_query(data_t *, data_t *);

/* ------------------------------------------------------------------------ */

OBLCORE_IMPEXP array_t *       data_add_all_reducer(data_t *, array_t *);
OBLCORE_IMPEXP array_t *       data_add_all_as_data_reducer(char *, array_t *);
OBLCORE_IMPEXP array_t *       data_add_strings_reducer(data_t *, array_t *);
OBLCORE_IMPEXP dict_t *        data_put_all_reducer(entry_t *, dict_t *);

/* ------------------------------------------------------------------------ */

OBLCORE_IMPEXP void            typedescr_init(void);
OBLCORE_IMPEXP kind_t *        kind_get(int);
OBLCORE_IMPEXP kind_t *        kind_get_byname(char *);

OBLCORE_IMPEXP void            kind_register_method(kind_t *, methoddescr_t *);
OBLCORE_IMPEXP methoddescr_t * kind_get_method(kind_t *, char *);

/* ------------------------------------------------------------------------ */

OBLCORE_IMPEXP int                       _interface_register(int, char *, int, ...);
OBLCORE_IMPEXP interface_t *             interface_get(int);
OBLCORE_IMPEXP interface_t *             interface_get_byname(char *);
#define interface_register_method(i, m)  kind_register_method((kind_t *) (i), (m))
#define interface_get_method(i, m)       kind_get_method((kind_t *) (i), (m))

/* ------------------------------------------------------------------------ */

OBLCORE_IMPEXP vtable_t *      vtable_build(vtable_t[]);
OBLCORE_IMPEXP void            vtable_dump(vtable_t *);
OBLCORE_IMPEXP void_t          vtable_get(vtable_t *, int);
OBLCORE_IMPEXP int             vtable_implements(vtable_t *, int);

/* ------------------------------------------------------------------------ */

OBLCORE_IMPEXP int             _typedescr_register(int, char *, vtable_t *, methoddescr_t *);
OBLCORE_IMPEXP typedescr_t *   typedescr_assign_inheritance(int, int);
OBLCORE_IMPEXP typedescr_t *   typedescr_register_function(typedescr_t *, int, void_t);
OBLCORE_IMPEXP typedescr_t *   typedescr_register_accessors(int, accessor_t *);
OBLCORE_IMPEXP accessor_t *    typedescr_get_accessor(typedescr_t *, char *);
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


OBLCORE_IMPEXP type_t          type_data;

/* ------------------------------------------------------------------------ */

#define typename(t)                        ((t) ? (((kind_t *) (t)) -> name) : "")
#define typetype(t)                        ((t) ? ((kind_t *) (t)) -> type : -1)
#define typedescr_get_local_function(t, f) (((t) && (t) -> vtable) ? (t) -> vtable[(f)].fnc : NULL)
#define typedescr_get_function(t, f)       (((t) && (t) -> inherited_vtable) ? (t) -> inherited_vtable[(f)].fnc : NULL)
#define typedescr_register_method(t, m)    kind_register_method((kind_t *) (t), (m))
#define typedescr_constructors(t)          ((t) -> constructors)
#define typedescr_set_size(d, t)           (typedescr_get((d)) -> size = sizeof(t))
  
/* ------------------------------------------------------------------------ */

#define data_new(dt,st)                    ((st *) data_settype((data_t *) _new(sizeof(st)), dt))

static inline int data_is_data(void *data) {
#ifndef NDEBUG
  return !data || (((data_t *) data) -> cookie == MAGIC_COOKIE);
#else /* NDEBUG */
  return TRUE;
#endif
}

static inline data_t * data_as_data(void *data) {
#ifndef NDEBUG
  if (!data) {
    return NULL;
  } else if (data_is_data(data)) {
    return (data_t *) data;
  } else {
    fprintf(stderr, "data_as_data(%p): cookie = %x\n",
        data, ((data_t *) data) -> cookie);
    abort();
  }
#else
  return (data_t *) data;
#endif
}

static inline int data_type(void *data) {
  return (data) ? data_as_data(data) -> type : -1;
}

static inline data_t * data_copy(void *src) {
  data_t *s = data_as_data(src);
  if (s) {
    s -> refs++;
  }
  return s;
}

static inline data_t * data_uncopy(void *src) {
  data_t *s = data_as_data(src);
  if (s) {
    s -> refs--;
  }
  return s;
}

static inline char * data_tostring(void *data) {
  return _data_tostring(data_as_data(data));
}

static inline int data_hastype(void *data, int type) {
  data_t *d = data_as_data(data);

  return (d)
    ? (d -> type == type) || typedescr_is(typedescr_get(d -> type), type)
    : FALSE;
}

/* -- T Y P E D E S C R  T Y P E ----------------------------------------- */

type_skel(typedescr, Type, typedescr_t)
type_skel(interface, Interface, interface_t)
type_skel(method, Method, methoddescr_t)

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

#ifndef _MSC_VER
#define builtin_interface_register(i, num, fncs...)                          \
  _interface_register(i, #i , num, ## fncs);

#define interface_register(i, num, fncs...)                                  \
  if (i < 1) {                                                               \
    i = _interface_register(i, #i , num, ## fncs);                           \
  }
#else /* _MSC_VER */
#define builtin_interface_register(i, num, ...)                          \
  _interface_register(i, #i , num, __VA_ARGS__);

#define interface_register(i, num, ...)                                  \
  if (i < 1) {                                                               \
    i = _interface_register(i, #i , num, __VA_ARGS__);                           \
  }
#endif /* _MSC_VER */

static inline typedescr_t * data_typedescr(void *data) {
  return (data)
    ? typedescr_get(data_as_data(data) -> type)
    : NULL;
}

static inline char * data_typename(void *data) {
  return (data)
    ? typename(typedescr_get(data_type(data_as_data(data))))
    : "null";
}

static inline void_t data_get_function(void  *data, vtable_id_t func) {
  return (data)
      ? typedescr_get_function(data_typedescr(data_as_data(data)), func)
      : NULL;
}

static inline int data_is_callable(void *d) {
  return data_hastype(d, Callable);
}

static inline int data_is_iterable(void *d) {
  return data_hastype(d, Iterable);
}

static inline int data_is_iterator(void *d) {
  return data_hastype(d, Iterator);
}

/* -- P O I N T E R  T Y P E ---------------------------------------------- */

OBLCORE_IMPEXP data_t * data_null(void);

type_skel(pointer, Pointer, pointer_t);

static inline pointer_t * ptr_create(size_t sz, void *ptr) {
  return (pointer_t *) data_create(Pointer, sz, ptr);
}

static inline data_t * data_wrap(void *p) {
  return (data_t *) ptr_create(0, p);
}

static inline void * data_unwrap(void *p) {
  return data_is_pointer(p) ? (data_as_pointer(p) -> ptr) : NULL;
}

static inline int data_isnull(data_t *data) {
  return !data || (data == data_null());
}

static inline int data_notnull(data_t *data) {
  return data && (data != data_null());
}

static inline data_t * ptr_to_data(size_t sz, void *p) {
  return (data_t *) ptr_create(sz, p);
}

/* -- D A T A L I S T  T Y P E -------------------------------------------- */

#define data_array_create(i)   (array_set_type(array_create((i)), &type_data))
#define data_array_get(a, i)   ((data_t *) array_get((a), (i)))

OBLCORE_IMPEXP datalist_t *    datalist_create(array_t *);
OBLCORE_IMPEXP array_t *       datalist_to_array(datalist_t *);
OBLCORE_IMPEXP array_t *       datalist_to_str_array(datalist_t *);
OBLCORE_IMPEXP datalist_t *    str_array_to_datalist(array_t *);
OBLCORE_IMPEXP datalist_t *    _datalist_set(datalist_t *, int, data_t *);
OBLCORE_IMPEXP datalist_t *    _datalist_push(datalist_t *, data_t *);

static inline datalist_t * data_as_list(void *data) {
  return (data_hastype(data_as_data(data), List)) ? (datalist_t *) data : NULL;
}

static inline array_t * data_as_array(void *data) {
  return (array_t *) (((pointer_t *) data_as_list(data)) -> ptr);
}

static inline void datalist_free(datalist_t *list) {
  data_free((data_t *) list);
}

static inline int datalist_size(datalist_t *list) {
  return array_size(data_as_array(list));
}

static inline datalist_t * datalist_push(datalist_t *list, void *data) {
  return _datalist_push(list, data_as_data(data));
}

static inline datalist_t * datalist_set(datalist_t *list, int ix, void *data) {
  return _datalist_set(list, ix, data_as_data(data));
}

static inline data_t * datalist_remove(datalist_t *list, int ix) {
  return (data_t *) array_remove(data_as_array(list), ix);
}

static inline data_t * datalist_shift(datalist_t *list) {
  return (datalist_size(list)) ? datalist_remove(list, 0) : NULL;
}

static inline data_t * datalist_get(datalist_t *datalist, int ix) {
  return data_copy(data_array_get(data_as_array(datalist), ix));
}

static inline data_t * datalist_pop(datalist_t *list) {
  return (data_t *) array_pop(data_as_array(list));
}

static char * datalist_tostring(datalist_t *list) {
  return data_tostring(list);
}

static int data_is_datalist(void *data) {
  return data_hastype(data, List);
}

#define data_is_list(d)        (data_is_datalist((d)))

/* -- N U M E R I C  T Y P E S -------------------------------------------- */

OBLCORE_IMPEXP int_t *         int_create(intptr_t);
OBLCORE_IMPEXP int_t *         int_parse(char *);
OBLCORE_IMPEXP flt_t *         float_parse(char *);

static inline int data_intval(void *d) {
  return _data_intval(data_as_data(d));
}

static flt_t * float_create(double d) {
  return (flt_t *) data_create(Float, d);
}

static inline double data_floatval(void *d) {
  return _data_floatval(data_as_data(d));
}

static inline int data_is_numeric(void *d) {
  return data_hastype(d, Number);
}

static inline int data_is_int(void *d) {
  return data_hastype(d, Int);
}

static inline int data_is_bool(void *d) {
  return data_hastype(d, Bool);
}

static inline int data_is_float(void *d) {
  return data_hastype(d, Float);
}

static inline data_t * int_to_data(intptr_t i) {
  return (data_t *) int_create(i);
}

static inline data_t * flt_to_data(double f) {
  return (data_t *) float_create(f);
}

/* -- B O O L  T Y P E ---------------------------------------------------- */

OBLCORE_IMPEXP int_t *         bool_true;
OBLCORE_IMPEXP int_t *         bool_false;
OBLCORE_IMPEXP int_t *         bool_get(long);

#define int_as_bool(i)         ((data_t *) bool_get((i)))
#define data_true()            ((data_t *) bool_true)
#define data_false()           ((data_t *) bool_false)

/* ------------------------------------------------------------------------ */ 

#define strdata_dict_create()  (dict_set_data_type( \
                                 dict_set_key_type( \
                                   dict_create(NULL), \
                                   &type_str), \
                                 &type_data))

#define intdata_dict_create()  (dict_set_data_type( \
                                 dict_set_key_type( \
                                   dict_create(NULL), \
                                   &type_int), \
                                 &type_data))

#define datadata_dict_create() (dict_set_data_type( \
                                 dict_set_key_type( \
                                   dict_create(NULL), \
                                   &type_data), \
                                 &type_data))

#define data_dict_get(d, k)    ((data_t *) dict_get((d), (k)))

#define data_list_create()     (list_set_type(list_create(), &type_data))
#define data_list_pop(l)       (data_t *) list_pop((l))
#define data_list_shift(l)     (data_t *) list_shift((l))

#define data_set_create()      (set_set_type(set_create(NULL), &type_data))

#include <arguments.h>
#include <dictionary.h>
#include <exception.h>
#include <name.h>
#include <str.h>

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __DATA_H__ */
