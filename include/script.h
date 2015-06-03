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

typedef struct _namespace namespace_t;
typedef struct _module module_t;

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

typedef struct _bound_method {
  script_t  *script;
  object_t  *self;
  closure_t *closure;
  char      *str;
  int        refs;
} bound_method_t;

typedef data_t * (*native_t)(char *, array_t *, dict_t *);

typedef struct _native_fnc {
  script_t *script;
  name_t   *name;
  int       async;
  native_t  native_method;
  array_t  *params;
  int       refs;  
} native_fnc_t;

extern data_t *            data_create_script(script_t *);
extern data_t *            data_create_closure(closure_t *);

#define data_is_script(d)  ((d) && (data_type((d)) == Script))
#define data_scriptval(d)  (data_is_script((d)) ? ((script_t *) (d) -> ptrval) : NULL)
#define script_copy(s)     ((script_t *) data_copy((s)))
#define script_tostring(s) (data_tostring((data_t *) (s)))
#define script_free(s)     (data_free((data_t *) (s)))

#define data_is_boundmethod(d)  ((d) && (data_type((d)) == BoundMethod))
#define data_boundmethodval(d)  (data_is_boundmethod((d)) ? ((bound_method_t *) (d) -> ptrval) : NULL)

#define data_is_native(d)  ((d) && (data_type((d)) == Native))
#define data_nativeval(d)  (data_is_native((d)) ? ((native_fnc_t *) (d) -> ptrval) : NULL)

/* -- S C R I P T  P R O T O T Y P E S -------------------------------------*/

extern script_t *       script_create(module_t *, script_t *, char *);
extern name_t *         script_fullname(script_t *);
extern int              script_cmp(script_t *, script_t *);
extern unsigned int     script_hash(script_t *);
extern void             script_list(script_t *);
extern script_t *       script_get_toplevel(script_t *);
extern script_t *       script_push_instruction(script_t *, instruction_t *);
extern script_t *       script_start_deferred_block(script_t *);
extern script_t *       script_end_deferred_block(script_t *);
extern script_t *       script_pop_deferred_block(script_t *);
extern script_t *       script_bookmark(script_t *);
extern script_t *       script_discard_bookmark(script_t *);
extern script_t *       script_defer_bookmarked_block(script_t *);
extern closure_t *      script_create_closure(script_t *, closure_t *, data_t *);
extern data_t *         script_execute(script_t *, array_t *, dict_t *);
extern data_t *         script_create_object(script_t *, array_t *, dict_t *);
extern bound_method_t * script_bind(script_t *, object_t *);

/* -- B O U N D  M E T H O D  P R O T O T Y P E S ------------------------- */

extern bound_method_t * bound_method_create(script_t *, object_t *);
extern void             bound_method_free(bound_method_t *);
extern bound_method_t * bound_method_copy(bound_method_t *);
extern int              bound_method_cmp(bound_method_t *, bound_method_t *);
extern char *           bound_method_tostring(bound_method_t *);
extern data_t *         bound_method_execute(bound_method_t *, array_t *, dict_t *);

/* -- N A T I V E  F N C  P R O T O T Y P E S ----------------------------- */

extern native_fnc_t *   native_fnc_create(script_t *, char *name, native_t);
extern native_fnc_t *   native_fnc_copy(native_fnc_t *);
extern void             native_fnc_free(native_fnc_t *);
extern data_t *         native_fnc_execute(native_fnc_t *, array_t *, dict_t *);
extern char *           native_fnc_tostring(native_fnc_t *);
extern int              native_fnc_cmp(native_fnc_t *, native_fnc_t *);

#ifdef __cplusplus
}
#endif

#endif /* __SCRIPT_H__ */
