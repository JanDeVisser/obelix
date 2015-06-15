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
#include <set.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef data_t * (*import_t)(void *, module_t *);

typedef enum _modstate {
  ModStateUninitialized,
  ModStateLoading,
  ModStateActive
} modstate_t;

typedef struct _namespace {
  data_t    _d;
  char     *name;
  void     *import_ctx;
  import_t  import_fnc;
  data_t   *exit_code;
  dict_t   *modules;
} namespace_t;

typedef struct _module {
  data_t       _d;
  name_t      *name;
  data_t      *source;
  namespace_t *ns;
  modstate_t   state;
  object_t    *obj;
  set_t       *imports;
} module_t;

extern module_t *    mod_create(namespace_t *, name_t *);
extern unsigned int  mod_hash(module_t *);
extern int           mod_cmp(module_t *, module_t *);
extern int           mod_cmp_name(module_t *, name_t *);
extern object_t *    mod_get(module_t *);
extern data_t *      mod_set(module_t *, script_t *, array_t *, dict_t *);
extern data_t *      mod_resolve(module_t *, char *);
extern data_t *      mod_import(module_t *, name_t *);
extern module_t *    mod_exit(module_t *, data_t *);
extern data_t *      mod_exit_code(module_t *);

extern namespace_t * ns_create(char *, void *, import_t);
extern data_t *      ns_import(namespace_t *, name_t *);
extern data_t *      ns_execute(namespace_t *, name_t *, array_t *, dict_t *);
extern data_t *      ns_get(namespace_t *, name_t *);
extern namespace_t * ns_exit(namespace_t *, data_t *);
extern data_t *      ns_exit_code(namespace_t *);

extern int Namespace;
extern int Module;
extern int ns_debug;

#define data_is_namespace(d)   ((d) && (data_hastype((data_t *) (d), Namespace)))
#define data_as_namespace(d)   ((namespace_t *) (data_is_namespace((data_t *) (d)) ? (d) : NULL))
#define ns_free(o)             (data_free((data_t *) (o)))
#define ns_tostring(o)         (data_tostring((data_t *) (o)))
#define ns_copy(o)             ((namespace_t *) data_copy((data_t *) (o)))

#define data_is_module(d)      ((d) && (data_hastype((data_t *) (d), Module)))
#define data_as_module(d)      ((module_t *) (data_is_module((data_t *) (d)) ? (d) : NULL))
#define mod_free(o)            (data_free((data_t *) (o)))
#define mod_tostring(o)        (data_tostring((data_t *) (o)))
#define mod_copy(o)            ((module_t *) data_copy((data_t *) (o)))
#define data_create_module(o)  data_create(Module, (o))

#ifdef  __cplusplus
}
#endif

#endif /* __NAMESPACE_H__ */
