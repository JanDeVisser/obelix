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

#ifndef __VM_H__
#define __VM_H__

#include <stdarg.h>

#include <oblconfig.h>
#include <data.h>
#include <datastack.h>
#include <exception.h>
#include <list.h>
#include <name.h>
#include <nvp.h>
#include <set.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------------------ */

#ifndef OBLVM_IMPEXP
  #define OBLVM_IMPEXP	__DLL_IMPORT__
#endif /* OBLVM_IMPEXP */

/* -- F O R W A R D  D E C L A R A T I O N S ------------------------------ */

typedef struct _bound_method bound_method_t;
typedef struct _closure      closure_t;
typedef struct _bytecode     bytecode_t;
typedef struct _instruction  instruction_t;
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

OBLVM_IMPEXP object_t *       object_create(data_t *);
OBLVM_IMPEXP data_t *         object_get(object_t *, char *);
OBLVM_IMPEXP data_t *         object_set(object_t *, char *, data_t *);
OBLVM_IMPEXP int              object_has(object_t *, char *);
OBLVM_IMPEXP data_t *         object_call(object_t *, arguments_t *);
OBLVM_IMPEXP unsigned int     object_hash(object_t *);
OBLVM_IMPEXP int              object_cmp(object_t *, object_t *);
OBLVM_IMPEXP data_t *         object_resolve(object_t *, char *);
OBLVM_IMPEXP object_t *       object_bind_all(object_t *, data_t *);
OBLVM_IMPEXP data_t *         object_ctx_enter(object_t *);
OBLVM_IMPEXP data_t *         object_ctx_leave(object_t *, data_t *);

OBLVM_IMPEXP int Object;

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

OBLVM_IMPEXP module_t *    mod_create(namespace_t *, name_t *);
OBLVM_IMPEXP unsigned int  mod_hash(module_t *);
OBLVM_IMPEXP int           mod_cmp(module_t *, module_t *);
OBLVM_IMPEXP int           mod_cmp_name(module_t *, name_t *);
OBLVM_IMPEXP object_t *    mod_get(module_t *);
OBLVM_IMPEXP data_t *      mod_set(module_t *, script_t *, arguments_t *);
OBLVM_IMPEXP data_t *      mod_resolve(module_t *, char *);
OBLVM_IMPEXP data_t *      mod_import(module_t *, name_t *);
OBLVM_IMPEXP module_t *    mod_exit(module_t *, data_t *);
OBLVM_IMPEXP data_t *      mod_exit_code(module_t *);

OBLVM_IMPEXP int Module;

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

OBLVM_IMPEXP namespace_t * ns_create(char *, void *, import_t);
OBLVM_IMPEXP data_t *      ns_import(namespace_t *, name_t *);
OBLVM_IMPEXP data_t *      ns_execute(namespace_t *, name_t *, arguments_t *);
OBLVM_IMPEXP data_t *      ns_get(namespace_t *, name_t *);
OBLVM_IMPEXP namespace_t * ns_exit(namespace_t *, data_t *);
OBLVM_IMPEXP data_t *      ns_exit_code(namespace_t *);

OBLVM_IMPEXP int Namespace;

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
  bytecode_t    *bytecode;
};

OBLVM_IMPEXP script_t *       script_create(data_t *, char *);
OBLVM_IMPEXP name_t *         script_fullname(script_t *);
OBLVM_IMPEXP int              script_cmp(script_t *, script_t *);
OBLVM_IMPEXP unsigned int     script_hash(script_t *);
OBLVM_IMPEXP void             script_list(script_t *);
OBLVM_IMPEXP script_t *       script_get_toplevel(script_t *);
OBLVM_IMPEXP data_t *         script_execute(script_t *, arguments_t *);
OBLVM_IMPEXP data_t *         script_create_object(script_t *, arguments_t *);
OBLVM_IMPEXP bound_method_t * script_bind(script_t *, object_t *);

OBLVM_IMPEXP int Script;

type_skel(script, Script, script_t);

#define data_create_script(s) data_create(Script, (s))

/* -- B O U N D M E T H O D _ T ------------------------------------------- */

struct _bound_method {
  data_t     _d;
  script_t  *script;
  object_t  *self;
  closure_t *closure;
};

OBLVM_IMPEXP bound_method_t * bound_method_create(script_t *, object_t *);
OBLVM_IMPEXP int              bound_method_cmp(bound_method_t *, bound_method_t *);
OBLVM_IMPEXP closure_t *      bound_method_get_closure(bound_method_t *);
OBLVM_IMPEXP data_t *         bound_method_execute(bound_method_t *, arguments_t *);

OBLVM_IMPEXP int BoundMethod;

type_skel(bound_method, BoundMethod, bound_method_t);

/* -- B Y T E C O D E _ T ------------------------------------------------- */

struct _bytecode {
  data_t       _d;
  data_t      *owner;
  list_t      *instructions;
  list_t      *main_block;
  datastack_t *deferred_blocks;
  datastack_t *bookmarks;
  datastack_t *pending_labels;
  dict_t      *labels;
  int          current_line;
};

OBLVM_IMPEXP bytecode_t * bytecode_create(data_t *owner);
OBLVM_IMPEXP bytecode_t * bytecode_push_instruction(bytecode_t *, data_t *);
OBLVM_IMPEXP bytecode_t * bytecode_start_deferred_block(bytecode_t *);
OBLVM_IMPEXP bytecode_t * bytecode_end_deferred_block(bytecode_t *);
OBLVM_IMPEXP bytecode_t * bytecode_pop_deferred_block(bytecode_t *);
OBLVM_IMPEXP bytecode_t * bytecode_bookmark(bytecode_t *);
OBLVM_IMPEXP bytecode_t * bytecode_discard_bookmark(bytecode_t *);
OBLVM_IMPEXP bytecode_t * bytecode_defer_bookmarked_block(bytecode_t *);
OBLVM_IMPEXP void         bytecode_list_and_mark(bytecode_t *, instruction_t *);
OBLVM_IMPEXP void         bytecode_list(bytecode_t *);

OBLVM_IMPEXP int Bytecode;

type_skel(bytecode, Bytecode, bytecode_t);

/* -- V M _ T ------------------------------------------------------------- */

#define NUM_STASHES     8

typedef enum _vmstatus {
  VMStatusNone     = 0,
  VMStatusBreak    = 1,
  VMStatusContinue = 2,
  VMStatusReturn   = 4,
  VMStatusExit     = 8,
  VMStatusYield    = 16
} VMStatus;

typedef struct _vm {
  data_t            _d;
  data_t           *stashes[NUM_STASHES];
  bytecode_t       *bytecode;
  data_t           *exception;
  int               status;
  datastack_t      *stack;
  datastack_t      *contexts;
  listprocessor_t  *processor;
  struct _debugger *debugger;
} vm_t;

OBLVM_IMPEXP vm_t *   vm_create(bytecode_t *);
OBLVM_IMPEXP data_t * vm_pop(vm_t *);
OBLVM_IMPEXP data_t * vm_peek(vm_t *);
OBLVM_IMPEXP data_t * vm_push(vm_t *, data_t *);
OBLVM_IMPEXP vm_t *   vm_dup(vm_t *);
OBLVM_IMPEXP data_t * vm_stash(vm_t *, unsigned int, data_t *);
OBLVM_IMPEXP data_t * vm_unstash(vm_t *, unsigned int);
OBLVM_IMPEXP nvp_t *  vm_push_context(vm_t *, char *, data_t *);
OBLVM_IMPEXP nvp_t *  vm_peek_context(vm_t *);
OBLVM_IMPEXP nvp_t *  vm_pop_context(vm_t *);
OBLVM_IMPEXP data_t * vm_execute(vm_t *, data_t *);
OBLVM_IMPEXP data_t * vm_initialize(vm_t *, data_t *);

OBLVM_IMPEXP int VM;

type_skel(vm, VM, vm_t);

/* -- S T A C K F R A M E _ T --------------------------------------------- */

typedef struct _stackframe {
  data_t      _d;
  bytecode_t *bytecode;
  char       *funcname;
  char       *source;
  int         line;
} stackframe_t;

OBLVM_IMPEXP stackframe_t *  stackframe_create(data_t *);
OBLVM_IMPEXP int             stackframe_cmp(stackframe_t *, stackframe_t *);

OBLVM_IMPEXP int Stackframe;

type_skel(stackframe, Stackframe, stackframe_t);

/* -- S T A C K T R A C E _ T --------------------------------------------- */

typedef struct _stacktrace {
  data_t       _d;
  datastack_t *stack;
} stacktrace_t;

OBLVM_IMPEXP stacktrace_t * stacktrace_create(void);
OBLVM_IMPEXP int            stacktrace_cmp(stacktrace_t *, stacktrace_t *);
OBLVM_IMPEXP stacktrace_t * stacktrace_push(stacktrace_t *, data_t *);

OBLVM_IMPEXP int Stacktrace;

type_skel(stacktrace, Stacktrace, stacktrace_t);

/* -- I N S T R U C T I O N _ T ------------------------------------------- */

typedef data_t * (*execute_t)(instruction_t *, data_t *, vm_t *, bytecode_t *);

struct _instruction {
  data_t     _d;
  execute_t  execute;
  int        line;
  set_t     *labels;
  char      *name;
  data_t    *value;
};

typedef enum _callflag {
  CFNone        = 0x0000,
  CFInfix       = 0x0001,
  CFConstructor = 0x0002,
  CFVarargs     = 0x0004
} callflag_t;

typedef struct _function_call {
  data_t      _d;
  name_t     *name;
  callflag_t  flags;
  int         arg_count;
  array_t    *kwargs;
  char       *str;
} function_call_t;

OBLVM_IMPEXP int Instruction;
OBLVM_IMPEXP int Scope;
OBLVM_IMPEXP int ITByValue;
OBLVM_IMPEXP int ITByName;
OBLVM_IMPEXP int ITByNameValue;
OBLVM_IMPEXP int ITByValueOrName;

#define DeclareInstructionType(t)                                            \
OBLVM_IMPEXP int IT ## t;                                                    \
OBLVM_IMPEXP instruction_t * instruction_create_ ## t(char *, data_t *);

DeclareInstructionType(Assign);
DeclareInstructionType(Decr);
DeclareInstructionType(Dup);
DeclareInstructionType(EndLoop);
DeclareInstructionType(EnterContext);
DeclareInstructionType(FunctionCall);
DeclareInstructionType(Incr);
DeclareInstructionType(Iter);
DeclareInstructionType(Jump);
DeclareInstructionType(LeaveContext);
DeclareInstructionType(Next);
DeclareInstructionType(Nop);
DeclareInstructionType(Pop);
DeclareInstructionType(PushCtx);
DeclareInstructionType(PushVal);
DeclareInstructionType(Deref);
DeclareInstructionType(PushScope);
DeclareInstructionType(Return);
DeclareInstructionType(Stash);
DeclareInstructionType(Subscript);
DeclareInstructionType(Swap);
DeclareInstructionType(Test);
DeclareInstructionType(Throw);
DeclareInstructionType(Unstash);
DeclareInstructionType(VMStatus);
DeclareInstructionType(Yield);

OBLVM_IMPEXP instruction_t * instruction_create_byname(char *, char *, data_t *);
OBLVM_IMPEXP data_t *        instruction_create_enter_context(name_t *, data_t *);
OBLVM_IMPEXP data_t *        instruction_create_function(name_t *, callflag_t, long, array_t *);

OBLVM_IMPEXP instruction_t * instruction_assign_label(instruction_t *);
OBLVM_IMPEXP instruction_t * instruction_set_label(instruction_t *, data_t *);

OBLVM_IMPEXP void            instruction_trace(instruction_t *, char *, ...);

type_skel(instruction, Instruction, instruction_t);

#define instruction_create_assign(n)        ((data_t *) instruction_create_Assign(name_tostring((n)), (data_t *) (n)))
#define instruction_create_decr()           ((data_t *) instruction_create_Decr(NULL, NULL))
#define instruction_create_dup()            ((data_t *) instruction_create_Dup(NULL, NULL))
#define instruction_create_incr()           ((data_t *) instruction_create_Incr(NULL, NULL))
#define instruction_create_iter()           ((data_t *) instruction_create_Iter(NULL, NULL))
#define instruction_create_jump(l)          ((data_t *) instruction_create_Jump(data_tostring((l)), NULL))
#define instruction_create_leave_context(n) ((data_t *) instruction_create_LeaveContext(name_tostring((n)), (data_t *) name_create(1, (n))))
#define instruction_create_mark(l)          ((data_t *) instruction_create_Nop(NULL, int_to_data((l))))
#define instruction_create_nop()            ((data_t *) instruction_create_Nop(NULL, NULL))
#define instruction_create_next(n)          ((data_t *) instruction_create_Next(data_tostring((n)), NULL))
#define instruction_create_pop()            ((data_t *) instruction_create_Pop(NULL, NULL))
#define instruction_create_pushctx()        ((data_t *) instruction_create_PushCtx(NULL, NULL))
#define instruction_create_pushscope()      ((data_t *) instruction_create_PushScope(NULL, NULL))
#define instruction_create_pushval(v)       ((data_t *) instruction_create_PushVal(NULL, data_copy((v))))
#define instruction_create_deref(n)         ((data_t *) instruction_create_Deref(name_tostring((n)), (data_t *) (n)))
#define instruction_create_return()         ((data_t *) instruction_create_Return(NULL, NULL))
#define instruction_create_stash(s)         ((data_t *) instruction_create_Stash(NULL, int_to_data(s)))
#define instruction_create_swap()           ((data_t *) instruction_create_Swap(NULL, NULL))
#define instruction_create_test(l)          ((data_t *) instruction_create_Test(data_tostring((l)), NULL))
#define instruction_create_throw()          ((data_t *) instruction_create_Throw(NULL, NULL))
#define instruction_create_unstash(s)       ((data_t *) instruction_create_Unstash(NULL, int_to_data(s)))

/* -- C L O S U R E _ T --------------------------------------------------- */

struct _closure {
  data_t           _d;
  struct _closure *up;
  script_t        *script;
  bytecode_t      *bytecode;
  data_t          *self;
  dictionary_t    *params;
  dictionary_t    *variables;
  data_t          *thread;
  int              line;
};

OBLVM_IMPEXP closure_t *        closure_create(script_t *, closure_t *, data_t *);
OBLVM_IMPEXP int                closure_cmp(closure_t *, closure_t *);
OBLVM_IMPEXP unsigned int       closure_hash(closure_t *);
OBLVM_IMPEXP data_t *           closure_set(closure_t *, char *, data_t *);
OBLVM_IMPEXP data_t *           closure_get(closure_t *, char *);
OBLVM_IMPEXP int                closure_has(closure_t *, char *);
OBLVM_IMPEXP data_t *           closure_resolve(closure_t *, char *);
OBLVM_IMPEXP data_t *           closure_execute(closure_t *, arguments_t *);
OBLVM_IMPEXP data_t *           closure_import(closure_t *, name_t *);
OBLVM_IMPEXP exception_t *      closure_yield(closure_t *, vm_t *);
OBLVM_IMPEXP data_t *           closure_eval(closure_t *, script_t *);

OBLVM_IMPEXP int Closure;
type_skel(closure, Closure, closure_t);
#define data_create_closure(c)  data_create(Closure, (c))

/*
 * -- G E N E R A T O R _ T ----------------------------------------------- */

typedef struct _generator {
  data_t       _d;
  closure_t   *closure;
  vm_t        *vm;
  exception_t *status;
} generator_t;

OBLVM_IMPEXP generator_t *    generator_create(closure_t *, vm_t *, exception_t *);
OBLVM_IMPEXP data_t *         generator_next(generator_t *);
OBLVM_IMPEXP int              generator_has_next(generator_t *);
OBLVM_IMPEXP generator_t *    generator_interrupt(generator_t *);

OBLVM_IMPEXP int Generator;
type_skel(generator, Generator, generator_t);

/*
 * -- D E B U G G E R ---------------------------------------------------- */

typedef enum _debugcmd {
  DebugCmdNone,
  DebugCmdGo,
  DebugCmdHalt
} debugcmd_t;

typedef enum _debugstatus {
  DebugStatusRun,
  DebugStatusSingleStep,
  DebugStatusRunOut,
  DebugStatusHalt
} debugstatus_t;

typedef struct _debugger {
  data_t        *scope;
  vm_t          *vm;
  bytecode_t    *bytecode;
  debugstatus_t  status;
  debugcmd_t     last_command;
} debugger_t;

OBLVM_IMPEXP debugger_t * debugger_create(vm_t *vm, data_t *scope);
OBLVM_IMPEXP void         debugger_start(debugger_t *);
OBLVM_IMPEXP debugcmd_t   debugger_step_before(debugger_t *, instruction_t *);
OBLVM_IMPEXP void         debugger_step_after(debugger_t *, instruction_t *, data_t *);
OBLVM_IMPEXP void         debugger_exit(debugger_t *, data_t *);
OBLVM_IMPEXP void         debugger_free(debugger_t *);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_H__ */
