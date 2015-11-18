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

#include <libparser.h>
#include <data.h>
#include <datastack.h>
#include <grammar.h>
#include <lexer.h>
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

OBLPARSER_IMPEXP int Parser;
OBLPARSER_IMPEXP int parser_debug;

typedef struct _parser {
  data_t       _d;
  grammar_t   *grammar;
  lexer_t     *lexer;
  void        *data;
  list_t      *prod_stack;
  token_t     *last_token;
  int          line;
  int          column;
  data_t      *error;
  datastack_t *stack;
  dict_t      *variables;
  function_t  *on_newline;
} parser_t;

#define data_is_parser(d)     ((d) && data_hastype((data_t *) (d), Parser))
#define data_as_parser(d)     (data_is_parser((d)) ? ((parser_t *) (d)) : NULL)
#define parser_copy(p)        ((parser_t *) data_copy((data_t *) (p)))
#define parser_free(p)        (data_free((data_t *) (p)))
#define parser_tostring(p)    (data_tostring((data_t *) (p)))

typedef parser_t * (*parser_data_fnc_t)(parser_t *, data_t *);
typedef parser_t * (*parser_fnc_t)(parser_t *);

OBLPARSER_IMPEXP parser_t * parser_create(grammar_t *);
OBLPARSER_IMPEXP parser_t * parser_clear(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_set(parser_t *, char *, data_t *);
OBLPARSER_IMPEXP data_t *   parser_get(parser_t *, char *);
OBLPARSER_IMPEXP data_t *   parser_pop(parser_t *, char *);
OBLPARSER_IMPEXP data_t *   parser_parse(parser_t *, data_t *);

OBLPARSER_IMPEXP parser_t * parser_pushval(parser_t *, data_t *);
OBLPARSER_IMPEXP parser_t * parser_push(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_bookmark(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_rollup_list(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_rollup_name(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_new_counter(parser_t *);
OBLPARSER_IMPEXP parser_t * parser_incr(parser_t *);

#ifdef __cplusplus
}
#endif

#endif /* __PARSER_H__ */
