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

#include <datastack.h>
#include <grammar.h>
#include <list.h>

extern int parser_debug;

typedef struct _parser {
  grammar_t     *grammar;
  void          *data;
  list_t        *prod_stack;
  token_t       *last_token;
  datastack_t   *stack;
  dict_t        *variables;
} parser_t;

typedef parser_t * (*parser_fnc_t)(parser_t *);

extern parser_t *      parser_create(grammar_t *);
extern parser_t *      parser_clear(parser_t *);
extern parser_t *      parser_set(parser_t *, char *, data_t *);
extern data_t *        parser_get(parser_t *, char *);
extern data_t *        parser_pop(parser_t *, char *);
extern void            _parser_parse(parser_t *, reader_t *);
extern void            parser_free(parser_t *);

#define parser_parse(p, r)     _parser_parse((p), ((reader_t *) (r)))

#endif /* __PARSER_H__ */
