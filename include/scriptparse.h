/*
 * /obelix/include/scriptparse.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __SCRIPTPARSE_H__
#define __SCRIPTPARSE_H__

#include <parser.h>

extern parser_t *       script_parse_init(parser_t *);
extern parser_t *       script_parse_done(parser_t *);
extern parser_t *       script_parse_push_last_token(parser_t *);
extern parser_t *       script_parse_push_false(parser_t *);
extern parser_t *       script_parse_push_true(parser_t *);
extern parser_t *       script_parse_param_count(parser_t *);
extern parser_t *       script_parse_push_param(parser_t *);
extern parser_t *       script_parse_emit_assign(parser_t *);
extern parser_t *       script_parse_emit_pushvar(parser_t *);
extern parser_t *       script_parse_emit_pushval(parser_t *);
extern parser_t *       script_parse_push_signed_val(parser_t *);
extern parser_t *       script_parse_emit_unary_op(parser_t *);
extern parser_t *       script_parse_emit_infix_op(parser_t *);
extern parser_t *       script_parse_emit_func_call(parser_t *);
extern parser_t *       script_parse_emit_new_list(parser_t *);
extern parser_t *       script_parse_emit_import(parser_t *);
extern parser_t *       script_parse_emit_jump(parser_t *);
extern parser_t *       script_parse_emit_pop(parser_t *);
extern parser_t *       script_parse_emit_nop(parser_t *);
extern parser_t *       script_parse_emit_test(parser_t *);
extern parser_t *       script_parse_push_label(parser_t *);
extern parser_t *       script_parse_emit_else(parser_t *);
extern parser_t *       script_parse_emit_end(parser_t *);
extern parser_t *       script_parse_emit_end_while(parser_t *);
extern parser_t *       script_parse_start_function(parser_t *);
extern parser_t *       script_parse_end_function(parser_t *);

#endif /* __SCRIPTPARSE_H__ */
