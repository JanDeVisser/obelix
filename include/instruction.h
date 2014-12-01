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

#include <array.h>
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
  ITNop
} instruction_type_t;

typedef void script_ctx_t;

typedef data_t * (*native_t)(script_ctx_t *, char *, array_t *);

typedef struct _function_def {
  char         *name;
  native_t      fnc;
  int           min_params;
  int           max_params;
} function_def_t;

typedef struct _instruction {
  instruction_type_t  type;
  char               *label;
  char               *name;
  int                 num;
  union {
    native_t          function;
    data_t           *value;
  };
} instruction_t;

typedef char * (*instr_fnc_t)(instruction_t *, script_ctx_t *);

extern function_def_t * function_def_create(char *, native_t, int, int);
extern function_def_t * function_def_copy(function_def_t *);
extern void             function_def_free(function_def_t *);

extern instruction_t *  instruction_create_assign(char *);
extern instruction_t *  instruction_create_pushvar(char *);
extern instruction_t *  instruction_create_pushval(data_t *);
extern instruction_t *  instruction_create_function(char *, int);
extern instruction_t *  instruction_create_test(char *);
extern instruction_t *  instruction_create_jump(char *);
extern instruction_t *  instruction_create_pop(void);
extern instruction_t *  instruction_create_nop(void);
extern char *           instruction_assign_label(instruction_t *);
extern instruction_t *  instruction_set_label(instruction_t *, char *);
extern char *           instruction_execute(instruction_t *, script_ctx_t *);
extern char *           instruction_tostring(instruction_t *);
extern void             instruction_free(instruction_t *);

#endif /* __INSTRUCTION_H__ */
