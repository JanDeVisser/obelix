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

#include <data-typedefs.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void            typedescr_init(void);
extern kind_t *        kind_get(int);
extern kind_t *        kind_get_byname(const char *);

extern void            kind_register_method(kind_t *, methoddescr_t *);
extern methoddescr_t * kind_get_method(kind_t *, const char *);

extern int                       _interface_register(int, char *, int, ...);
extern interface_t *             interface_get(int);
extern interface_t *             interface_get_byname(const char *);
#define interface_register_method(i, m)  kind_register_method((kind_t *) (i), (m))
#define interface_get_method(i, m)       kind_get_method((kind_t *) (i), (m))

extern vtable_t *      vtable_build(vtable_t[]);
extern void            vtable_dump(vtable_t *);
extern void_t          vtable_get(vtable_t *, int);
extern int             vtable_implements(vtable_t *, int);

extern int             _typedescr_register(int, const char *, vtable_t *, methoddescr_t *);
extern typedescr_t *   typedescr_assign_inheritance(int, int);
extern typedescr_t *   typedescr_register_function(typedescr_t *, int, void_t);
extern typedescr_t *   typedescr_register_accessors(int, accessor_t *);
extern accessor_t *    typedescr_get_accessor(typedescr_t *, const char *);
extern typedescr_t *   typedescr_get(int);
extern typedescr_t *   typedescr_get_byname(const char *);
extern void            typedescr_count(void);
extern unsigned int    typedescr_hash(typedescr_t *);
extern void            typedescr_register_methods(int, methoddescr_t[]);
extern int             typedescr_implements(typedescr_t *, int);
extern int             typedescr_inherits(typedescr_t *, int);
extern int             typedescr_is(typedescr_t *, int);
extern void            typedescr_dump_vtable(typedescr_t *);
extern methoddescr_t * typedescr_get_method(typedescr_t *, const char *);

#define type_name(t)                        ((t) ? (((kind_t *) (t)) -> name) : "")
#define typetype(t)                        ((t) ? ((kind_t *) (t)) -> type : -1)
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
  assert(t >= 0);                                                             \
  _typedescr_register(t, name , _vtable_ ## t, _methods_ ## t);              \
  typedescr_set_size(t, type);                                               \

#define builtin_typedescr_register_nomethods(t, name, type)                  \
  assert(t >= 0);                                                             \
  _typedescr_register(t, name , _vtable_ ## t, NULL);                        \
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

#endif /* __TYPEDESCR_H__ */
