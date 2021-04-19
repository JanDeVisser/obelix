/*
 * /obelix/src/parser/libparser.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LIBAST_H__
#define __LIBAST_H__

#include <oblconfig.h>

#include <stdio.h>

#include <ast.h>
#include <exception.h>
#include <lexer.h>
#include <token.h>

#define __ENUMERATE_AST_NODE_TYPE(t, base, ...)                                    \
extern ast_ ## t ## _t * _ast_ ## t  ## _new(ast_ ## t  ## _t *, va_list);         \
extern void              _ast_ ## t  ## _free(ast_ ## t ## _t *);                  \
extern data_t *          _ast_ ## t  ## _call(ast_ ## t  ## _t *, arguments_t *);  \
extern char *            _ast_ ## t  ## _tostring(ast_ ## t  ## _t *);             \
extern int AST ## t;
ENUMERATE_AST_NODE_TYPES
#undef __ENUMERATE_AST_NODE_TYPE

extern int ast_debug;
extern int script_debug;
extern int script_trace;

#endif /* __LIBAST_H__ */
