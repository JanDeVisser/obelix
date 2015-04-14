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

#include <data.h>
#include <dict.h>
#include <name.h>
#include <object.h>
#include <script.h>

typedef data_t * (*import_t)(void *, name_t *);

extern int ns_debug;

typedef enum _modstate {
  ModStateUninitialized,
  ModStateLoading,
  ModStateActive
} modstate_t;

typedef struct _namespace {
  struct _namespace *up;
  int                level;
  void              *import_ctx;
  import_t           import_fnc;
  data_t            *exit_code;
  data_t            *root;
  char              *str;
} namespace_t;

typedef struct _module {
  modstate_t    state;
  object_t     *obj;
  namespace_t  *ns;
  name_t       *name;
  dict_t       *contents;
  set_t        *imports;
  int           refs;
  char         *str;
  unsigned int  hash;
} module_t;

#define data_is_module(d)  ((d) && (data_type((d)) == Module))
#define data_moduleval(d)  ((module_t *) (data_is_module((d)) ? ((d) -> ptrval) : NULL))

extern data_t *      data_create_module(module_t *);

extern module_t *    mod_create(name_t *);
extern void          mod_free(module_t *);
extern module_t *    mod_copy(module_t *);
extern unsigned int  mod_hash(module_t *);
extern int           mod_cmp(module_t *, module_t *);
extern int           mod_cmp_name(module_t *, name_t *);
extern char *        mod_tostring(module_t *);
extern int           mod_has_module(module_t *, char *);
extern data_t *      mod_get_module(module_t *, char *);
extern module_t *    mod_add_module(module_t *, name_t *, module_t *);
extern object_t *    mod_get(module_t *);
extern data_t *      mod_set(module_t *, script_t *, array_t *, dict_t *);
extern data_t *      mod_resolve(module_t *, char *);
extern name_t *      mod_name(module_t *);

extern namespace_t * ns_create(namespace_t *);
extern namespace_t * ns_create_root(void *, import_t);
extern void          ns_free(namespace_t *);
extern data_t *      ns_import(namespace_t *, name_t *);
extern data_t *      ns_execute(namespace_t *, name_t *, array_t *, dict_t *);
extern data_t *      ns_get(namespace_t *, name_t *);
extern data_t *      ns_resolve(namespace_t *, char *);
extern char *        ns_tostring(namespace_t *);
extern namespace_t * ns_exit(namespace_t *, data_t *);
extern data_t *      ns_exit_code(namespace_t *);

#endif /* __NAMESPACE_H__ */
