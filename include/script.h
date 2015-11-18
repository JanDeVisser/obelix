/*
 * /obelix/include/script.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <libparser.h>
#include <bytecode.h>
#include <data.h>
#include <list.h>
#include <name.h>
#include <object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _namespace    namespace_t;
typedef struct _module       module_t;
typedef struct _closure      closure_t;
typedef struct _bound_method bound_method_t;

typedef enum _script_type {
  STNone = 0,
  STASync,
  STGenerator
} script_type_t;

typedef struct _script {
  data_t          _d;
  struct _script *up;
  name_t         *name;
  name_t         *fullname;
  script_type_t   type;
  list_t         *baseclasses;
  dict_t         *functions;
  array_t        *params;
  module_t       *mod;
  bytecode_t     *bytecode;
} script_t;

#define data_is_script(d)     ((d) && data_hastype((d), Script))
#define data_as_script(d)     (data_is_script((d)) ? ((script_t *) (d)) : NULL)
#define script_copy(s)        ((script_t *) data_copy((data_t *) (s)))
#define script_tostring(s)    (data_tostring((data_t *) (s)))
#define script_free(s)        (data_free((data_t *) (s)))

#define data_create_script(s) data_create(Script, (s))

/* -- S C R I P T  P R O T O T Y P E S -------------------------------------*/

OBLPARSER_IMPEXP script_t *             script_create(struct _module *, script_t *, char *);
OBLPARSER_IMPEXP name_t *               script_fullname(script_t *);
OBLPARSER_IMPEXP int                    script_cmp(script_t *, script_t *);
OBLPARSER_IMPEXP unsigned int           script_hash(script_t *);
OBLPARSER_IMPEXP void                   script_list(script_t *);
OBLPARSER_IMPEXP script_t *             script_get_toplevel(script_t *);
OBLPARSER_IMPEXP closure_t *            script_create_closure(script_t *, struct _closure *, data_t *);
OBLPARSER_IMPEXP data_t *               script_execute(script_t *, array_t *, dict_t *);
OBLPARSER_IMPEXP data_t *               script_create_object(script_t *, array_t *, dict_t *);
OBLPARSER_IMPEXP struct _bound_method * script_bind(script_t *, object_t *);

OBLPARSER_IMPEXP int script_debug;
OBLPARSER_IMPEXP int Script;

#ifdef __cplusplus
}
#endif

#endif /* __SCRIPT_H__ */
