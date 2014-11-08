/*
 * core.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CORE_H__
#define __CORE_H__

#include <stdarg.h>
#include <stdlib.h>

#define TRUE   1
#define FALSE  0
#define NEW(t) ( (t *) new( sizeof(t) ) )

typedef void *  (*void_t)(void);
typedef int     (*cmp_t)(void *, void *);
typedef int     (*hash_t)(void *);
typedef char *  (*tostring_t)(void *);
typedef void *  (*reduce_t)(void *, void *);
typedef void *  (*obj_reduce_t)(void *, reduce_t, void *);
typedef void    (*visit_t)(void *);
typedef visit_t free_t;

typedef enum _enum_function_type {
  Void, Visitor, Reducer, Stringifier, Destructor, Evaluator
} function_type_t; 

typedef struct _function_union {
  void_t     void_fnc;
  visit_t    visitor;
  reduce_t   reducer;
  tostring_t stringfier;
  free_t     destructor;
} function_ptr_t;

typedef struct _function {
  function_type_t type;
  function_ptr_t  fnc;
} function_t;

extern function_ptr_t no_func_ptr;
extern function_t no_func;

#define NOFUNCPTR no_func_ptr
#define NOFUNC no_func

typedef struct _reduce_ctx {
  void           *obj;
  void           *user;
  void           *data;
  function_ptr_t fnc;
} reduce_ctx;

extern void * new(int);
extern void * new_ptrarray(int);
extern void * resize_block(void *, int);
extern void * resize_ptrarray(void *, int);

extern unsigned int hash(void *, size_t);
extern void         debug(char *, ...);

extern char *       strrand(char *, size_t);
extern unsigned int strhash(char *);

extern reduce_ctx * reduce_ctx_create(void *, void *, function_ptr_t);

#endif /* __CORE_H__ */
