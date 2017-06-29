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

#define typename(t)                        ((t) ? (((kind_t *) (t)) -> name) : "")
#define typetype(t)                        ((t) ? ((kind_t *) (t)) -> type : -1)
#define typedescr_get_local_function(t, f) (((t) && (t) -> vtable) ? (t) -> vtable[(f)].fnc : NULL)
#define typedescr_get_function(t, f)       (((t) && (t) -> inherited_vtable) ? (t) -> inherited_vtable[(f)].fnc : NULL)
#define typedescr_register_method(t, m)    kind_register_method((kind_t *) (t), (m))
#define typedescr_constructors(t)          ((t) -> constructors)
#define typedescr_set_size(d, t)           (typedescr_get((d)) -> size = sizeof(t))

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

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __TYPEDESCR_H__ */
