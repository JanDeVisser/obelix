/*
 * /obelix/src/grammar/libgrammar.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LIBGRAMMAR_H__
#define __LIBGRAMMAR_H__

#ifndef oblgrammar_EXPORTS
  #define OBLGRAMMAR_IMPEXP extern
  #ifndef OBL_STATIC
    #define OBL_STATIC
  #endif
#endif

#include <stdio.h>

#include <grammar.h>
#include <list.h>

typedef struct _ge_dump_ctx {
  data_t              *obj;
  struct _ge_dump_ctx *parent;
  data_t              *stream;
} ge_dump_ctx_t;

typedef ge_t *   (*ge_dump_fnc_t)(ge_dump_ctx_t *);
typedef data_t * (*ge_get_children_fnc_t)(data_t *, list_t *);

extern void        grammar_init(void);
extern void        grammar_action_register(void);
extern void        grammar_element_register(void);
extern void        nonterminal_register(void);
extern void        rule_register(void);
extern void        rule_entry_register(void);

extern list_t *    ge_append_child(data_t *, list_t *);

extern void         _grammar_get_firsts_visitor(entry_t *);
extern int *        _grammar_check_LL1_reducer(entry_t *, int *);
extern void         _grammar_build_parse_table_visitor(entry_t *);
extern function_t * _grammar_resolve_function(grammar_t *, char *, char *);

extern set_t *      _nonterminal_get_follows(nonterminal_t *);
extern set_t *      _nonterminal_get_firsts(nonterminal_t *);
extern int          _nonterminal_check_LL1(nonterminal_t *);
extern void         _nonterminal_build_parse_table(nonterminal_t *);
extern grammar_t *  _nonterminal_dump_terminal(token_code_t, grammar_t *);

extern set_t *      _rule_get_firsts(rule_t *);
extern void         _rule_build_parse_table(rule_t *);
extern set_t *      _rule_get_follows(rule_t *);
extern rule_t *     _rule_add_parse_table_entry(long, rule_t *);

extern set_t *      _rule_entry_get_firsts(rule_entry_t *, set_t *);
extern set_t *      _rule_entry_get_follows(rule_entry_t *, set_t *);

#endif /* __LIBGRAMMAR_H__ */
