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

/* -- F O R W A R D  D E C L A R A T I O N S ------------------------------ */

typedef struct _bytecode     bytecode_t;
typedef struct _instruction  instruction_t;

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

extern bytecode_t * bytecode_create(data_t *owner);
extern bytecode_t * bytecode_push_instruction(bytecode_t *, data_t *);
extern bytecode_t * bytecode_start_deferred_block(bytecode_t *);
extern bytecode_t * bytecode_end_deferred_block(bytecode_t *);
extern bytecode_t * bytecode_pop_deferred_block(bytecode_t *);
extern bytecode_t * bytecode_bookmark(bytecode_t *);
extern bytecode_t * bytecode_discard_bookmark(bytecode_t *);
extern bytecode_t * bytecode_defer_bookmarked_block(bytecode_t *);
extern void         bytecode_list_and_mark(bytecode_t *, instruction_t *);
extern void         bytecode_list(bytecode_t *);

extern int Bytecode;

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

extern vm_t *   vm_create(bytecode_t *);
extern data_t * vm_pop(vm_t *);
extern data_t * vm_peek(vm_t *);
extern data_t * vm_push(vm_t *, data_t *);
extern vm_t *   vm_dup(vm_t *);
extern data_t * vm_stash(vm_t *, unsigned int, data_t *);
extern data_t * vm_unstash(vm_t *, unsigned int);
extern nvp_t *  vm_push_context(vm_t *, char *, data_t *);
extern nvp_t *  vm_peek_context(vm_t *);
extern nvp_t *  vm_pop_context(vm_t *);
extern data_t * vm_execute(vm_t *, data_t *);
extern data_t * vm_initialize(vm_t *, data_t *);

extern int VM;

type_skel(vm, VM, vm_t);

/* -- S T A C K F R A M E _ T --------------------------------------------- */

typedef struct _stackframe {
  data_t      _d;
  bytecode_t *bytecode;
  char       *funcname;
  char       *source;
  int         line;
} stackframe_t;

extern stackframe_t *  stackframe_create(data_t *);
extern int             stackframe_cmp(stackframe_t *, stackframe_t *);

extern int Stackframe;

type_skel(stackframe, Stackframe, stackframe_t);

/* -- S T A C K T R A C E _ T --------------------------------------------- */

typedef struct _stacktrace {
  data_t       _d;
  datastack_t *stack;
} stacktrace_t;

extern stacktrace_t * stacktrace_create(void);
extern int            stacktrace_cmp(stacktrace_t *, stacktrace_t *);
extern stacktrace_t * stacktrace_push(stacktrace_t *, data_t *);

extern int Stacktrace;

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

extern int Instruction;
extern int Scope;
extern int ITByValue;
extern int ITByName;
extern int ITByNameValue;
extern int ITByValueOrName;

#define DeclareInstructionType(t)                                            \
extern int IT ## t;                                                    \
extern instruction_t * instruction_create_ ## t(char *, data_t *);

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

extern instruction_t * instruction_create_byname(char *, char *, data_t *);
extern data_t *        instruction_create_enter_context(name_t *, data_t *);
extern data_t *        instruction_create_function(name_t *, callflag_t, long, array_t *);

extern instruction_t * instruction_assign_label(instruction_t *);
extern instruction_t * instruction_set_label(instruction_t *, data_t *);

extern void            instruction_trace(instruction_t *, char *, ...);

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

extern debugger_t * debugger_create(vm_t *vm, data_t *scope);
extern void         debugger_start(debugger_t *);
extern debugcmd_t   debugger_step_before(debugger_t *, instruction_t *);
extern void         debugger_step_after(debugger_t *, instruction_t *, data_t *);
extern void         debugger_exit(debugger_t *, data_t *);
extern void         debugger_free(debugger_t *);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_H__ */
