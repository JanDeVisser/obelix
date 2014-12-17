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

typedef enum _instruction_type {
  ITAssign,
  ITPushVar,
  ITPushVal,
  ITFunction,
  ITTest,
  ITPop,
  ITJump,
  ITImport,
  ITNop
} instruction_type_t;

typedef struct _instruction {
  instruction_type_t  type;
  char               *label;
  char               *name;
  data_t             *value;
  int                 num;
} instruction_t;

struct _closure;

extern instruction_t *  instruction_create(int, char *, data_t *);
extern instruction_t *  instruction_create_assign(array_t *);
extern instruction_t *  instruction_create_pushvar(array_t *);
extern instruction_t *  instruction_create_pushval(data_t *);
extern instruction_t *  instruction_create_function(char *, long);
extern instruction_t *  instruction_create_test(char *);
extern instruction_t *  instruction_create_jump(char *);
extern instruction_t *  instruction_create_import(array_t *);
extern instruction_t *  instruction_create_pop(void);
extern instruction_t *  instruction_create_nop(void);
extern char *           instruction_assign_label(instruction_t *);
extern instruction_t *  instruction_set_label(instruction_t *, char *);
extern data_t *         instruction_execute(instruction_t *, struct _closure *);
extern char *           instruction_tostring(instruction_t *);
extern void             instruction_free(instruction_t *);

#endif /* __INSTRUCTION_H__ */
