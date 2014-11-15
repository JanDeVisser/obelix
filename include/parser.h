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

#include <array.h>
#include <dict.h>
#include <lexer.h>
#include <list.h>
#include <parser.h>
#include <resolve.h>
#include <set.h>

struct _parser;
struct _rule;

typedef struct _parser * (*parser_fnc_t)(struct _parser *);

typedef struct _grammar {
  dict_t       *rules;
  struct _rule *entrypoint;
  dict_t       *keywords;
  array_t      *lexer_options;
  parser_fnc_t  initializer;
  parser_fnc_t  finalizer;
  resolve_t    *resolve;
} grammar_t;

typedef struct _rule {
  grammar_t    *grammar;
  int           state;
  char         *name;
  array_t      *options;
  parser_fnc_t  initializer;
  parser_fnc_t  finalizer;
  set_t        *firsts;
  set_t        *follows;
  dict_t       *parse_table;
} rule_t;

typedef struct _rule_option {
  rule_t       *rule;
  array_t      *items;
  set_t        *firsts;
  set_t        *follows;
} rule_option_t;

typedef struct _rule_item {
  rule_option_t  *option;
  parser_fnc_t    initializer;
  parser_fnc_t    finalizer;
  int             terminal;
  union {
    token_t  *token;
    char     *rule;
  };
} rule_item_t;

typedef struct _parser {
  grammar_t     *grammar;
  void          *data;
  list_t        *tokens;
} parser_t;

extern grammar_t *     grammar_create();
extern grammar_t *     _grammar_read(reader_t *);
extern void            grammar_free(grammar_t *);
extern void            grammar_set_options(grammar_t *, dict_t *);
extern rule_t *        grammar_get_rule(grammar_t *, char *);
extern grammar_t *     grammar_set_initializer(grammar_t *, parser_fnc_t);
extern parser_fnc_t    grammar_get_initializer(grammar_t *);
extern grammar_t *     grammar_set_finalizer(grammar_t *, parser_fnc_t);
extern parser_fnc_t    grammar_get_finalizer(grammar_t *);
extern grammar_t *     grammar_set_option(grammar_t *, lexer_option_t, long);
extern long            grammar_get_option(grammar_t *, lexer_option_t);
extern void            grammar_dump(grammar_t *);

extern rule_t *        rule_create(grammar_t *, char *);
extern void            rule_free(rule_t *rule);
extern void            rule_set_options(rule_t *, dict_t *);
extern rule_t *        rule_set_initializer(rule_t *, parser_fnc_t);
extern parser_fnc_t    rule_get_initializer(rule_t *);
extern rule_t *        rule_set_finalizer(rule_t *, parser_fnc_t);
extern parser_fnc_t    rule_get_finalizer(rule_t *);
extern void            rule_dump(rule_t *);

extern rule_option_t * rule_option_create(rule_t *);
extern void            rule_option_free(rule_option_t *);
extern void            rule_option_dump(rule_option_t *);

extern rule_item_t *   rule_item_terminal(rule_option_t *, token_t *);
extern rule_item_t *   rule_item_non_terminal(rule_option_t *, char *);
extern rule_item_t *   rule_item_empty(rule_option_t *);
extern void            rule_item_set_options(rule_item_t *, dict_t *);
extern rule_item_t *   rule_item_set_initializer(rule_item_t *, parser_fnc_t);
extern parser_fnc_t    rule_item_get_initializer(rule_item_t *);
extern rule_item_t *   rule_item_set_finalizer(rule_item_t *, parser_fnc_t);
extern parser_fnc_t    rule_item_get_finalizer(rule_item_t *);
extern void            rule_item_free(rule_item_t *);
extern void            rule_item_dump(rule_item_t *);

extern parser_t *      parser_create(grammar_t *);
extern void *          parser_get_data(parser_t *);
extern parser_t        parser_set_data(parser_t *, void *);
extern parser_t *      _parser_read_grammar(reader_t *);
extern void            _parser_parse(parser_t *, reader_t *);
extern void            parser_free(parser_t *);

#define grammar_read(r)        _grammar_read(((reader_t *) (r)))
#define parser_read_grammar(r) _parser_read_grammar(((reader_t *) (r)))
#define parser_parser(p, r)    _parser_parse((p), ((reader_t *) (r)))

#endif /* __PARSER_H__ */
