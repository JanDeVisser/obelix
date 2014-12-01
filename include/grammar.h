/*
 * /obelix/include/grammar.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __GRAMMAR_H__
#define __GRAMMAR_H__

#include <array.h>
#include <dict.h>
#include <lexer.h>
#include <resolve.h>
#include <set.h>

extern int grammar_debug;

typedef enum _parsing_strategy {
  ParsingStrategyTopDown,
  ParsingStrategyBottomUp
} strategy_t;

struct _rule;

typedef struct _grammar_fnc {
  char      *name;
  voidptr_t  fnc;
} grammar_fnc_t;

typedef struct _grammar {
  dict_t        *rules;
  struct _rule  *entrypoint;
  dict_t        *keywords;
  array_t       *lexer_options;
  strategy_t     strategy;
  char          *prefix;
  grammar_fnc_t *initializer;
  grammar_fnc_t *finalizer;
} grammar_t;

typedef struct _rule {
  grammar_t     *grammar;
  int            state;
  char          *name;
  array_t       *options;
  grammar_fnc_t *initializer;
  grammar_fnc_t *finalizer;
  set_t         *firsts;
  set_t         *follows;
  dict_t        *parse_table;
} rule_t;

typedef struct _rule_option {
  rule_t       *rule;
  int           seqnr;
  array_t      *items;
  set_t        *firsts;
  set_t        *follows;
} rule_option_t;

typedef struct _rule_item {
  rule_option_t *option;
  grammar_fnc_t *initializer;
  grammar_fnc_t *finalizer;
  int            terminal;
  union {
    token_t     *token;
    char        *rule;
  };
} rule_item_t;

extern grammar_fnc_t * grammar_fnc_create(char *, voidptr_t);
extern grammar_fnc_t * grammar_fnc_copy(grammar_fnc_t *);
extern void            grammar_fnc_free(grammar_fnc_t *);
extern char *          grammar_fnc_tostring(grammar_fnc_t *);

extern grammar_t *     grammar_create();
extern grammar_t *     _grammar_read(reader_t *);
extern void            grammar_free(grammar_t *);
extern grammar_t *     grammar_set_parsing_strategy(grammar_t *, strategy_t);
extern strategy_t      grammar_get_parsing_strategy(grammar_t *);
extern grammar_t *     grammar_set_options(grammar_t *, dict_t *);
extern rule_t *        grammar_get_rule(grammar_t *, char *);
extern grammar_t *     grammar_set_initializer(grammar_t *, grammar_fnc_t *);
extern grammar_fnc_t * grammar_get_initializer(grammar_t *);
extern grammar_t *     grammar_set_finalizer(grammar_t *, grammar_fnc_t *);
extern grammar_fnc_t * grammar_get_finalizer(grammar_t *);
extern grammar_t *     grammar_set_option(grammar_t *, lexer_option_t, long);
extern long            grammar_get_option(grammar_t *, lexer_option_t);
extern void            grammar_dump(grammar_t *);
extern grammar_fnc_t * grammar_resolve_function(grammar_t *, char *);

extern rule_t *        rule_create(grammar_t *, char *);
extern void            rule_free(rule_t *rule);
extern void            rule_set_options(rule_t *, dict_t *);
extern rule_t *        rule_set_initializer(rule_t *, grammar_fnc_t *);
extern grammar_fnc_t * rule_get_initializer(rule_t *);
extern rule_t *        rule_set_finalizer(rule_t *, grammar_fnc_t *);
extern grammar_fnc_t * rule_get_finalizer(rule_t *);
extern void            rule_dump(rule_t *);

extern rule_option_t * rule_option_create(rule_t *);
extern void            rule_option_free(rule_option_t *);
extern void            rule_option_dump(rule_option_t *);

extern rule_item_t *   rule_item_terminal(rule_option_t *, token_t *);
extern rule_item_t *   rule_item_non_terminal(rule_option_t *, char *);
extern rule_item_t *   rule_item_empty(rule_option_t *);
extern void            rule_item_set_options(rule_item_t *, dict_t *);
extern rule_item_t *   rule_item_set_initializer(rule_item_t *, grammar_fnc_t *);
extern grammar_fnc_t * rule_item_get_initializer(rule_item_t *);
extern rule_item_t *   rule_item_set_finalizer(rule_item_t *, grammar_fnc_t *);
extern grammar_fnc_t * rule_item_get_finalizer(rule_item_t *);
extern void            rule_item_free(rule_item_t *);
extern void            rule_item_dump(rule_item_t *);

#define grammar_read(r) _grammar_read(((reader_t *) (r)))

#endif /* __GRAMMAR_H__ */
