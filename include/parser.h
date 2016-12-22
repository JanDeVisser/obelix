/*
 * /obelix/include/parser.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __PARSER_H__
#define __PARSER_H__

#include <data.h>
#include <datastack.h>
#include <dictionary.h>
#include <grammar.h>
#include <lexer.h>
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OBLPARSER_IMPEXP
  #define OBLPARSER_IMPEXP	__DLL_IMPORT__
#endif /* OBLPARSER_IMPEXP */

OBLPARSER_IMPEXP int Parser;
OBLPARSER_IMPEXP int parser_debug;
OBLPARSER_IMPEXP int ErrorLexerExhausted;

typedef enum {
  ParserStateNone = 0x01,
  ParserStateNonTerminal = 0x02,
  ParserStateTerminal = 0x04,
  ParserStateRule = 0x08,
  ParserStateAll = 0x0F,
  ParserStateDone = 0x10,
  ParserStateError = 0x20
} parser_state_t;

typedef struct _parser {
  dictionary_t    _d;
  grammar_t      *grammar;
  lexer_t        *lexer;
  void           *data;
  list_t         *prod_stack;
  parser_state_t  state;
  token_t        *last_token;
  data_t         *error;
  datastack_t    *stack;
  dict_t         *variables;
} parser_t;

#define data_is_parser(d)     ((d) && data_hastype((data_t *) (d), Parser))
#define data_as_parser(d)     (data_is_parser((d)) ? ((parser_t *) (d)) : NULL)
#define parser_copy(p)        ((parser_t *) data_copy((data_t *) (p)))
#define parser_free(p)        (data_free((data_t *) (p)))
#define parser_tostring(p)    (data_tostring((data_t *) (p)))
#define parser_set(p,n,v)     (dictionary_set((dictionary_t *) (p), (n), (v)))
#define parser_get(p,n)       (dictionary_get((dictionary_t *) (p), (n)))
#define parser_pop(p,n)       (dictionary_pop((dictionary_t *) (p), (n)))

typedef parser_t * (*parser_data_fnc_t)(parser_t *, data_t *);
typedef parser_t * (*parser_fnc_t)(parser_t *);

OBLPARSER_IMPEXP parser_t * parser_create(grammar_t *);
OBLPARSER_IMPEXP parser_t * parser_clear(parser_t *);
OBLPARSER_IMPEXP data_t *   parser_parse(parser_t *, data_t *);
OBLPARSER_IMPEXP data_t *   parser_resume(parser_t *, data_t *);

/* -- P A R S E R L I B  F U N C T I O N S -------------------------------- */

OBLPARSER_IMPEXP parser_t * parser_log(parser_t *, data_t *);
OBLPARSER_IMPEXP parser_t * parser_set_variable(parser_t *, data_t *);
OBLPARSER_IMPEXP parser_t * parser_pushval(parser_t *, data_t *);
OBLPARSER_IMPEXP parser_t * parser_push(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_push_token(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_push_const(parser_t *, data_t *);
OBLPARSER_IMPEXP parser_t * parser_discard(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_dup(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_push_tokenstring(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_bookmark(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_pop_bookmark(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_rollup_list(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_rollup_name(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_rollup_nvp(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_new_counter(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_incr(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_count(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_discard_counter(parser_t *);

#ifdef __cplusplus
}
#endif

#endif /* __PARSER_H__ */
