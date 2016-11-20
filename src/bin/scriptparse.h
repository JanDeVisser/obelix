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

#ifndef SCRIPTPARSE_IMPEXP
  #if (defined __WIN32__) || (defined _WIN32)
    #ifdef scriptparse_EXPORTS
      #define SCRIPTPARSE_IMPEXP	__DLL_EXPORT__
    #else /* ! scriptparse_EXPORTS */
      #define SCRIPTPARSE_IMPEXP	__DLL_IMPORT__
    #endif
  #else /* ! __WIN32__ */
    #define SCRIPTPARSE_IMPEXP extern
  #endif /* __WIN32__ */
#endif /* SCRIPTPARSE_IMPEXP */

#include <parser.h>

SCRIPTPARSE_IMPEXP int obelix_debug;

SCRIPTPARSE_IMPEXP parser_t * script_parse_init(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_done(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_mark_line(parser_t *parser, data_t *line);
SCRIPTPARSE_IMPEXP parser_t * script_make_nvp(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_init_function(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_setup_constructor(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_setup_function(parser_t *parser, data_t *func);
SCRIPTPARSE_IMPEXP parser_t * script_parse_deref_function(parser_t *parser, data_t *func);
SCRIPTPARSE_IMPEXP parser_t * script_parse_start_deferred_block(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_deferred_block(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_pop_deferred_block(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_instruction_bookmark(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_discard_instruction_bookmark(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_defer_bookmarked_block(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_instruction(parser_t *parser, data_t *type);
SCRIPTPARSE_IMPEXP parser_t * script_parse_assign(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_deref(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_push_token(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_pushval_from_stack(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_dupval(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_pushconst(parser_t *parser, data_t *constval);
SCRIPTPARSE_IMPEXP parser_t * script_parse_push_signed_val(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_unary_op(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_infix_op(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_call_op(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_jump(parser_t *parser, data_t *label);
SCRIPTPARSE_IMPEXP parser_t * script_parse_stash(parser_t *parser, data_t *stash);
SCRIPTPARSE_IMPEXP parser_t * script_parse_unstash(parser_t *parser, data_t *stash);
SCRIPTPARSE_IMPEXP parser_t * script_parse_reduce(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_comprehension(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_where(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_func_call(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_pop(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_nop(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_for(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_start_loop(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_loop(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_break(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_continue(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_if(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_test(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_elif(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_else(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_conditional(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_case_prolog(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_case(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_rollup_cases(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_start_function(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_baseclass_constructors(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_constructors(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_function(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_native_function(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_start_lambda(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_lambda(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_begin_context_block(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_throw_exception(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_leave(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_end_context_block(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_init_query(parser_t *parser);
SCRIPTPARSE_IMPEXP parser_t * script_parse_query(parser_t *parser);

#endif /* __SCRIPTPARSE_H__ */
