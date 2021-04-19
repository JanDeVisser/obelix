/*
 * /obelix/include/vm.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __RUNTIME_H__
#define __RUNTIME_H__

#include <stdarg.h>

#include <oblconfig.h>
#include <data.h>
#include <datastack.h>
#include <exception.h>
#include <list.h>
#include <name.h>
#include <nvp.h>
#include <set.h>
#include <ast.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------------------ */

/* -- F O R W A R D  D E C L A R A T I O N S ------------------------------ */

typedef struct _bound_method bound_method_t;
typedef struct _closure      closure_t;
typedef struct _module       module_t;
typedef struct _namespace    namespace_t;
typedef struct _object       object_t;
typedef struct _script       script_t;

/* O B J E C T _ T -------------------------------------------------------- */

struct _object {
  data_t        _d;
  data_t       *constructor;
  int           constructing;
  void         *ptr;
  dictionary_t *variables;
  data_t       *retval;
};

extern object_t *       object_create(data_t *);
extern data_t *         object_get(object_t *, char *);
extern data_t *         object_set(object_t *, char *, data_t *);
extern int              object_has(object_t *, char *);
extern data_t *         object_call(object_t *, arguments_t *);
extern unsigned int     object_hash(object_t *);
extern int              object_cmp(object_t *, object_t *);
extern data_t *         object_resolve(object_t *, char *);
extern object_t *       object_bind_all(object_t *, data_t *);
extern data_t *         object_ctx_enter(object_t *);
extern data_t *         object_ctx_leave(object_t *, data_t *);

extern int Object;

type_skel(object, Object, object_t);
#define data_create_object(o) data_create(Object, (o))

/* -- M O D U L E _ T ----------------------------------------------------- */

typedef data_t * (*import_t)(void *, module_t *);

typedef enum _modstate {
  ModStateUninitialized,
  ModStateLoading,
  ModStateActive
} modstate_t;

struct _module {
  data_t       _d;
  name_t      *name;
  data_t      *source;
  namespace_t *ns;
  modstate_t   state;
  object_t    *obj;
  closure_t   *closure;
  set_t       *imports;
  data_t      *parser;
};

extern module_t *    mod_create(namespace_t *, name_t *);
extern unsigned int  mod_hash(module_t *);
extern int           mod_cmp(module_t *, module_t *);
extern int           mod_cmp_name(module_t *, name_t *);
extern object_t *    mod_get(module_t *);
extern data_t *      mod_set(module_t *, script_t *, arguments_t *);
extern data_t *      mod_resolve(module_t *, char *);
extern data_t *      mod_import(module_t *, name_t *);
extern module_t *    mod_exit(module_t *, data_t *);
extern data_t *      mod_exit_code(module_t *);

extern int Module;

type_skel(mod, Module, module_t);
#define data_create_module(o)  data_create(Module, (o))

/* -- N A M E S P A C E _ T ----------------------------------------------- */

struct _namespace {
  data_t    _d;
  char     *name;
  void     *import_ctx;
  import_t  import_fnc;
  data_t   *exit_code;
  dict_t   *modules;
};

extern namespace_t * ns_create(char *, void *, import_t);
extern data_t *      ns_import(namespace_t *, name_t *);
extern data_t *      ns_execute(namespace_t *, name_t *, arguments_t *);
extern data_t *      ns_get(namespace_t *, name_t *);
extern namespace_t * ns_exit(namespace_t *, data_t *);
extern data_t *      ns_exit_code(namespace_t *);

extern int Namespace;

type_skel(ns, Namespace, namespace_t);

/* -- S C R I P T _ T ----------------------------------------------------- */

typedef enum _script_type {
  STNone = 0,
  STASync,
  STGenerator
} script_type_t;

struct _script {
  data_t         _d;
  script_t      *up;
  name_t        *name;
  name_t        *fullname;
  script_type_t  type;
  list_t        *baseclasses;
  dictionary_t  *functions;
  array_t       *params;
  module_t      *mod;
  ast_Script_t  *ast;
};

extern script_t *       script_create(data_t *, char *);
extern name_t *         script_fullname(script_t *);
extern int              script_cmp(script_t *, script_t *);
extern unsigned int     script_hash(script_t *);
extern void             script_list(script_t *);
extern script_t *       script_get_toplevel(script_t *);
extern data_t *         script_execute(script_t *, arguments_t *);
extern data_t *         script_create_object(script_t *, arguments_t *);
extern bound_method_t * script_bind(script_t *, object_t *);

extern int Script;

type_skel(script, Script, script_t);

#define data_create_script(s) data_create(Script, (s))

/* -- B O U N D M E T H O D _ T ------------------------------------------- */

typedef struct _bound_method {
  data_t     _d;
  script_t  *script;
  object_t  *self;
  closure_t *closure;
} bound_method_t;

extern bound_method_t * bound_method_create(script_t *, object_t *);
extern int              bound_method_cmp(bound_method_t *, bound_method_t *);
extern closure_t *      bound_method_get_closure(bound_method_t *);
extern data_t *         bound_method_execute(bound_method_t *, arguments_t *);

extern int BoundMethod;

type_skel(bound_method, BoundMethod, bound_method_t);

/* -- C L O S U R E _ T --------------------------------------------------- */

struct _closure {
  data_t           _d;
  struct _closure *up;
  script_t        *script;
  ast_Expr_t      *ast;
  data_t          *self;
  dictionary_t    *params;
  dictionary_t    *variables;
  data_t          *thread;
  int              line;
};

extern closure_t *        closure_create(script_t *, closure_t *, data_t *);
extern int                closure_cmp(closure_t *, closure_t *);
extern unsigned int       closure_hash(closure_t *);
extern data_t *           closure_set(closure_t *, char *, data_t *);
extern data_t *           closure_get(closure_t *, char *);
extern int                closure_has(closure_t *, char *);
extern data_t *           closure_resolve(closure_t *, char *);
extern data_t *           closure_execute(closure_t *, arguments_t *);
extern data_t *           closure_import(closure_t *, name_t *);
extern exception_t *      closure_yield(closure_t *, ast_Expr_t *);
extern data_t *           closure_eval(closure_t *, script_t *);

extern int Closure;
type_skel(closure, Closure, closure_t);
#define data_create_closure(c)  data_create(Closure, (c))

/*
 * -- G E N E R A T O R _ T ----------------------------------------------- */

typedef struct _generator {
  data_t        _d;
  closure_t    *closure;
  ast_Expr_t   *ast;
  exception_t  *status;
} generator_t;

extern generator_t *    generator_create(closure_t *, exception_t *);
extern data_t *         generator_next(generator_t *);
extern int              generator_has_next(generator_t *);
extern generator_t *    generator_interrupt(generator_t *);

extern int Generator;
type_skel(generator, Generator, generator_t);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __RUNTIME_H__ */
