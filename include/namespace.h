/*
 * /obelix/include/namespace.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <array.h>
#include <data.h>
#include <dict.h>
#include <object.h>
#include <script.h>

typedef data_t * (*import_t)(void *, array_t *);

extern int ns_debug;

typedef struct _module {
  object_t  *obj;
  char      *name;
  int        refs;
} module_t;

typedef struct _namespace {
  void                *import_ctx;
  union {
    struct _namespace *up;
    import_t           import_fnc;
  };
  dict_t              *contents;
} namespace_t;

#define data_is_module(d)  ((d) && (data_type((d)) == Module))
#define data_moduleval(d)  ((module_t *) (data_is_module((d)) ? ((d) -> ptrval) : NULL))

extern data_t *      data_create_module(module_t *);

extern module_t *    mod_create(script_t *, array_t *);
extern void          mod_free(module_t *);
extern module_t *    mod_copy(module_t *);

extern namespace_t * ns_create(namespace_t *);
extern namespace_t * ns_create_root(void *, import_t);
extern void          ns_free(namespace_t *);
extern data_t *      ns_import(namespace_t *, array_t *);
extern data_t *      ns_get(namespace_t *, array_t *);
extern data_t *      ns_gets(namespace_t *, char *);
extern int           ns_has(namespace_t *, char *);
extern data_t *      ns_resolve(namespace_t *, array_t *);

#endif /* __NAMESPACE_H__ */
