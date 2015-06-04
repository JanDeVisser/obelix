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

#include <bytecode.h>
#include <core.h>
#include <data.h>
#include <datastack.h>
#include <dict.h>
#include <instruction.h>
#include <list.h>
#include <parser.h>
#include <object.h>
#include <set.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int script_debug;

typedef struct _namespace    namespace_t;
typedef struct _module       module_t;
typedef struct _closure      closure_t;
typedef struct _bound_method bound_method_t;

typedef struct _script {
  data_t          data;
  struct _script *up;
  name_t         *name;
  name_t         *fullname;
  int             async;
  list_t         *baseclasses;
  dict_t         *functions;
  array_t        *params;
  module_t       *mod;
  bytecode_t     *bytecode;
} script_t;

#define data_is_script(d)     ((d) && (data_type((d)) == Script))
#define data_scriptval(d)     (data_is_script((d)) ? ((script_t *) (d) -> ptrval) : NULL)
#define script_copy(s)        ((script_t *) data_copy((data_t *) (s)))
#define script_tostring(s)    (data_tostring((data_t *) (s)))
#define script_free(s)        (data_free((data_t *) (s)))

#define data_create_script(s) data_create(Script, (s))

/* -- S C R I P T  P R O T O T Y P E S -------------------------------------*/

extern script_t *       script_create(module_t *, script_t *, char *);
extern name_t *         script_fullname(script_t *);
extern int              script_cmp(script_t *, script_t *);
extern unsigned int     script_hash(script_t *);
extern void             script_list(script_t *);
extern script_t *       script_get_toplevel(script_t *);
extern closure_t *      script_create_closure(script_t *, closure_t *, data_t *);
extern data_t *         script_execute(script_t *, array_t *, dict_t *);
extern data_t *         script_create_object(script_t *, array_t *, dict_t *);
extern bound_method_t * script_bind(script_t *, object_t *);

#ifdef __cplusplus
}
#endif

#endif /* __SCRIPT_H__ */
