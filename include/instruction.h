/*
 * /obelix/include/instruction.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include <libparser.h>
#include <bytecode.h>
#include <data.h>
#include <name.h>
#include <set.h>
#include <vm.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */
  
struct _instruction;
  
typedef data_t * (*execute_t)(struct _instruction *, data_t *, vm_t *, bytecode_t *);

typedef struct _instruction {
  data_t     _d;
  execute_t  execute;
  int        line;
  set_t     *labels;
  char      *name;
  data_t    *value;
} instruction_t;

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

OBLPARSER_IMPEXP int Instruction;
OBLPARSER_IMPEXP int ITByValue;
OBLPARSER_IMPEXP int ITByName;
OBLPARSER_IMPEXP int ITByNameValue;
OBLPARSER_IMPEXP int ITByValueOrName;

#define DeclareInstructionType(t)                                            \
OBLPARSER_IMPEXP int IT ## t;                                                \
OBLPARSER_IMPEXP instruction_t * instruction_create_ ## t(char *, data_t *);

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

OBLPARSER_IMPEXP instruction_t * instruction_create_byname(char *, char *, data_t *);
OBLPARSER_IMPEXP void            instruction_trace(char *, char *, ...);

OBLPARSER_IMPEXP data_t *        instruction_create_enter_context(name_t *, data_t *);
OBLPARSER_IMPEXP data_t *        instruction_create_function(name_t *, callflag_t, long, array_t *);

OBLPARSER_IMPEXP instruction_t * instruction_assign_label(instruction_t *);
OBLPARSER_IMPEXP instruction_t * instruction_set_label(instruction_t *, data_t *);

#define data_is_instruction(d)  ((d) && data_hastype((d), Instruction))
#define data_as_instruction(d)  ((instruction_t *) (data_is_instruction((d)) ? (d) : NULL))
#define instruction_free(i)     (data_free((data_t *) (i)))
#define instruction_tostring(i) (data_tostring((data_t *) (i)))
#define instruction_copy(i)     ((instruction_t *) data_copy((data_t *) (i)))

#define instruction_create_assign(n)        ((data_t *) instruction_create_Assign(name_tostring((n)), (data_t *) name_create(1, (n))))
#define instruction_create_decr()           ((data_t *) instruction_create_Decr(NULL, NULL))
#define instruction_create_dup()            ((data_t *) instruction_create_Dup(NULL, NULL))
#define instruction_create_incr()           ((data_t *) instruction_create_Incr(NULL, NULL))
#define instruction_create_iter()           ((data_t *) instruction_create_Iter(NULL, NULL))
#define instruction_create_jump(l)          ((data_t *) instruction_create_Jump(data_tostring((l)), NULL))
#define instruction_create_leave_context(n) ((data_t *) instruction_create_LeaveContext(name_tostring((n)), (data_t *) name_create(1, (n))))
#define instruction_create_mark(l)          ((data_t *) instruction_create_Nop(NULL, int_create((l))))
#define instruction_create_nop()            ((data_t *) instruction_create_Nop(NULL, NULL))
#define instruction_create_next(n)          ((data_t *) instruction_create_Next(data_tostring((n)), NULL))
#define instruction_create_pop()            ((data_t *) instruction_create_Pop(NULL, NULL))
#define instruction_create_pushctx()        ((data_t *) instruction_create_PushCtx(NULL, NULL))
#define instruction_create_pushscope()      ((data_t *) instruction_create_PushScope(NULL, NULL))
#define instruction_create_pushval(v)       ((data_t *) instruction_create_PushVal(NULL, data_copy((v))))
#define instruction_create_deref(n)         ((data_t *) instruction_create_Deref(name_tostring((n)), (data_t *) name_create(1, (n))))
#define instruction_create_return()         ((data_t *) instruction_create_Return(NULL, NULL))
#define instruction_create_stash(s)         ((data_t *) instruction_create_Stash(NULL, int_create(s)))
#define instruction_create_swap()           ((data_t *) instruction_create_Swap(NULL, NULL))
#define instruction_create_test(l)          ((data_t *) instruction_create_Test(data_tostring((l)), NULL))
#define instruction_create_throw()          ((data_t *) instruction_create_Throw(NULL, NULL))
#define instruction_create_unstash(s)       ((data_t *) instruction_create_Unstash(NULL, int_create(s)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __INSTRUCTION_H__ */
