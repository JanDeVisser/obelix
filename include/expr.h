/*
 * expr.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#ifndef __EXPR_H__
#define __EXPR_H__

#include <dict.h>
#include <data.h>

typedef struct _context {
  dict_t          *vars;
  struct _context *up;
} context_t;

extern context_t * context_create(context_t *);
extern void        context_free(context_t *);
extern context_t * context_up(context_t *);
extern data_t *    context_resolve(context_t *, char *);
extern context_t * context_set(context_t *, char *, data_t *);

typedef data_t * (*eval_t)(context_t *, void *, list_t *);

typedef struct _expr {
  struct _expr *up;
  eval_t        eval;
  void         *data;
  free_t        data_free;
  context_t    *context;
  list_t       *nodes;
} expr_t;


extern expr_t * expr_create(expr_t *, eval_t, void *);
extern expr_t * expr_set_context(expr_t *, context_t *);
extern expr_t * expr_set_data_free(expr_t *, free_t);
extern expr_t * expr_add_node(expr_t *, expr_t *);
extern void     expr_free(expr_t *);
extern data_t * expr_evaluate(expr_t *);

extern expr_t * expr_str_literal(expr_t *, char *);
extern expr_t * expr_int_literal(expr_t *, int);
extern expr_t * expr_float_literal(expr_t *, float);
extern expr_t * expr_bool_literal(expr_t *, unsigned char);
extern expr_t * expr_deref(expr_t *, char *);
extern expr_t * expr_funccall(expr_t *, eval_t);

#endif /* __EXPR_H__ */
