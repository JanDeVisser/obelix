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

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */
    
typedef enum _instruction_type {
  Instruction = 22,
  ITByName = 23,
  ITByValue = 24,
} instruction_type_t;

typedef struct _instruction {
  data_t    data;
  int       line;
  char      label[9];
  char     *name;
  data_t   *value;
} instruction_t;

typedef enum _callflag {
  CFNone        = 0x0000,
  CFInfix       = 0x0001,
  CFConstructor = 0x0002,
  CFVarargs     = 0x0004
} callflag_t;

extern int Instruction;

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

extern char *           instruction_assign_label(instruction_t *);
extern instruction_t *  instruction_set_label(instruction_t *, data_t *);
extern data_t *         instruction_execute(instruction_t *, struct _closure *);
extern instruction_t *  instruction_copy(instruction_t *);
extern char *           instruction_tostring(instruction_t *);
extern void             instruction_free(instruction_t *);

#define instruction_create_decr()     data_create(ITDecr, NULL, NULL)
#define instruction_create_incr()     data_create(ITIncr, NULL, NULL)
#define instruction_create_iter()     data_create(ITIter, NULL, NULL)
#define instruction_create_pop()      data_create(ITPop, NULL, NULL)
#define instruction_create_dup()      data_create(ITDup, NULL, NULL)
#define instruction_create_swap()     data_create(ITSwap, NULL, NULL)
#define instruction_create_mark(l)    data_create(ITNop, NULL, data_create(Int, (l)))
#define instruction_create_nop()      data_create(ITNop, NULL, NULL)
#define instruction_create_pushctx()  data_create(ITPushCtx, NULL, NULL)
#define instruction_create_stash(s)   data_create(ITStash, NULL, data_create(Int, (s)))
#define instruction_create_throw()    data_create(ITThrow, NULL, NULL)
#define instruction_create_return()   data_create(ITReturn, NULL, NULL)
#define instruction_create_unstash(s) data_create(ITUnstash, NULL, data_create(Int, (s)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __INSTRUCTION_H__ */
