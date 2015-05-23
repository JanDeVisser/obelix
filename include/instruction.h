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

#include <core.h>
#include <data.h>
#include <name.h>

typedef enum _instruction_type {
  ITAssign,
  ITDecr,
  ITDup,
  ITEnterContext,
  ITFunctionCall,
  ITImport,
  ITIncr,
  ITIter,
  ITJump,
  ITLeaveContext,
  ITNext,
  ITNop,
  ITPop,
  ITPushVal,
  ITPushVar,
  ITStash,
  ITSubscript,
  ITSwap,
  ITTest,
  ITThrow,
  ITUnstash
} instruction_type_t;

typedef struct _instruction {
  instruction_type_t  type;
  int                 line;
  char                label[9];
  char               *name;
  data_t             *value;
  int                 refs;
  char               *str;
} instruction_t;

typedef enum _callflag {
  CFNone        = 0x0000,
  CFInfix       = 0x0001,
  CFConstructor = 0x0002,
  CFVarargs     = 0x0004
} callflag_t;

struct _closure;

extern instruction_t *  instruction_create(int, char *, data_t *);
extern instruction_t *  instruction_create_assign(name_t *);
extern instruction_t *  instruction_create_enter_context(name_t *, data_t *);
extern instruction_t *  instruction_create_leave_context(name_t *);
extern instruction_t *  instruction_create_pushvar(name_t *);
extern instruction_t *  instruction_create_pushval(data_t *);
extern instruction_t *  instruction_create_function(name_t *, callflag_t, long, array_t *);
extern instruction_t *  instruction_create_test(data_t *);
extern instruction_t *  instruction_create_next(data_t *);
extern instruction_t *  instruction_create_jump(data_t *);
extern instruction_t *  instruction_create_import(name_t *);
extern instruction_t *  instruction_create_mark(int);
extern instruction_t *  instruction_create_stash(unsigned int);
extern instruction_t *  instruction_create_unstash(unsigned int);

extern char *           instruction_assign_label(instruction_t *);
extern instruction_t *  instruction_set_label(instruction_t *, data_t *);
extern data_t *         instruction_execute(instruction_t *, struct _closure *);
extern instruction_t *  instruction_copy(instruction_t *);
extern char *           instruction_tostring(instruction_t *);
extern void             instruction_free(instruction_t *);

#define instruction_create_decr()  instruction_create(ITDecr, NULL, NULL)
#define instruction_create_incr()  instruction_create(ITIncr, NULL, NULL)
#define instruction_create_iter()  instruction_create(ITIter, NULL, NULL)
#define instruction_create_pop()   instruction_create(ITPop, NULL, NULL)
#define instruction_create_dup()   instruction_create(ITDup, NULL, NULL)
#define instruction_create_swap()  instruction_create(ITSwap, NULL, NULL)
#define instruction_create_nop()   instruction_create(ITNop, NULL, NULL)
#define instruction_create_throw() instruction_create(ITThrow, NULL, NULL)

#endif /* __INSTRUCTION_H__ */
