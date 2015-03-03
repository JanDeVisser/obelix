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

#include <core.h>
#include <data.h>
#include <datastack.h>
#include <dict.h>
#include <instruction.h>
#include <list.h>
#include <parser.h>
#include <object.h>

extern int script_debug;

typedef struct _namespace namespace_t;

typedef struct _script {
  struct _script    *up;
  name_t            *name;
  list_t            *instructions;
  dict_t            *functions;
  array_t           *params;
  dict_t            *labels;
  char              *label;
  struct _namespace *ns;
  int                refs;
} script_t;

typedef struct _closure {
  struct _closure   *up;
  script_t          *script;
  data_t            *self;
  dict_t            *variables;
  struct _namespace *imports;
  datastack_t       *stack;
  int                refs;
} closure_t;

typedef data_t * (*native_t)(char *, array_t *, dict_t *);

typedef struct _native_fnc {
  script_t *script;
  name_t   *name;
  native_t  native_method;
  array_t  *params;
  int       refs;  
} native_fnc_t;

extern data_t *            data_create_script(script_t *);
extern data_t *            data_create_closure(closure_t *);

#define data_is_script(d)  ((d) && (data_type((d)) == Script))
#define data_scriptval(d)  (data_is_script((d)) ? ((script_t *) (d) -> ptrval) : NULL)
#define data_is_closure(d) ((d) && (data_type((d)) == Closure))
#define data_closureval(d) (data_is_closure((d)) ? ((closure_t *) (d) -> ptrval) : NULL)
#define data_is_native(d)  ((d) && (data_type((d)) == Native))
#define data_nativeval(d)  (data_is_native((d)) ? ((native_fnc_t *) (d) -> ptrval) : NULL)

extern script_t *       script_create(namespace_t *, script_t *, char *);
extern script_t *       script_copy(script_t *);
extern void             script_free(script_t *);
extern char *           script_tostring(script_t *);
extern void             script_list(script_t *);
extern script_t *       script_get_toplevel(script_t *);
extern script_t *       script_push_instruction(script_t *, instruction_t *);
extern closure_t *      script_create_closure(script_t *, data_t *, closure_t *);
extern data_t *         script_execute(script_t *, array_t *, dict_t *);

/*
 * closure_t prototypes
 */
extern void             closure_free(closure_t *);
extern char *           closure_tostring(closure_t *);
extern data_t *         closure_pop(closure_t *);
extern closure_t *      closure_push(closure_t *, data_t *);
extern data_t *         closure_set(closure_t *, char *, data_t *);
extern data_t *         closure_get(closure_t *, char *);
extern int              closure_has(closure_t *, char *);
extern data_t *         closure_resolve(closure_t *, char *);
extern data_t *         closure_execute(closure_t *, array_t *, dict_t *);
extern data_t *         closure_import(closure_t *, name_t *);
  
extern native_fnc_t *   native_fnc_create(script_t *, char *name, native_t);
extern native_fnc_t *   native_fnc_copy(native_fnc_t *);
extern void             native_fnc_free(native_fnc_t *);
extern data_t *         native_fnc_execute(native_fnc_t *, array_t *, dict_t *);
extern char *           native_fnc_tostring(native_fnc_t *);
extern int              native_fnc_cmp(native_fnc_t *, native_fnc_t *);

#endif /* __SCRIPT_H__ */
