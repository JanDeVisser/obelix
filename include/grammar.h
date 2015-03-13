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

#define NONTERMINAL_DEF         200
#define NONTERMINAL_DEF_STR     ":="

/* grammar-wide options */
#define LIB_STR                 "lib"
#define PREFIX_STR              "prefix"
#define STRATEGY_STR            "strategy"
#define IGNORE_STR              "ignore"
#define CASE_SENSITIVE_STR      "case_sensitive"
#define HASHPLING_STR           "hashpling"
#define SIGNED_NUMBERS_STR      "signed_numbers"

/* ----------------------------------------------------------------------- */

typedef enum _parsing_strategy {
  ParsingStrategyTopDown,
  ParsingStrategyBottomUp
} strategy_t;

typedef struct _grammar_action {
  function_t *fnc;
  data_t     *data;
  char       *str;
  int         refs;
} grammar_action_t;

/* ----------------------------------------------------------------------- */

typedef enum _grammar_element_type {
  GETGrammarElement = 20,
  GETGrammar,
  GETNonTerminal,
  GETRule,
  GETRuleEntry,
  GETTerminal
} grammar_element_type_t;

struct _grammar;
struct _grammar_element;

typedef void * (*set_option_t)(struct _grammar_element *, token_t *, token_t *);

typedef struct _grammar_element {
  grammar_element_type_t   type;
  struct _grammar         *grammar;
  struct _grammar_element *owner;
  list_t                  *actions;
  dict_t                  *variables;
  set_option_t             set_option_delegate;
  void                    *ptr;
  char                    *str;
} ge_t;

typedef struct _nonterminal {
  ge_t                    *ge;
  int                      state;
  char                    *name;
  array_t                 *rules;
  set_t                   *firsts;
  set_t                   *follows;
  dict_t                  *parse_table;
} nonterminal_t;

typedef struct _rule {
  ge_t                    *ge;
  array_t                 *entries;
  set_t                   *firsts;
  set_t                   *follows;
} rule_t;

typedef struct _rule_entry {
  ge_t                    *ge;
  int                      terminal;
  char                    *str;
  union {
    token_t               *token;
    char                  *nonterminal;
  };
} rule_entry_t;

typedef struct _grammar {
  ge_t                    *ge;
  dict_t                  *nonterminals;
  nonterminal_t           *entrypoint;
  dict_t                  *keywords;
  array_t                 *lexer_options;
  strategy_t               strategy;
  char                    *prefix;
  int                      dryrun;
} grammar_t;

/* ----------------------------------------------------------------------- */

extern grammar_action_t * grammar_action_create(function_t *, data_t *);
extern grammar_action_t * grammar_action_copy(grammar_action_t *);
extern void               grammar_action_free(grammar_action_t *);
extern int                grammar_action_cmp(grammar_action_t *, grammar_action_t *);
extern unsigned int       grammar_action_hash(grammar_action_t *);
extern char *             grammar_action_tostring(grammar_action_t *);

extern void            ge_free(ge_t *);
extern typedescr_t *   ge_typedescr(ge_t *);
extern void_t          ge_function(ge_t *, int);
extern ge_t *          ge_add_action(ge_t *, grammar_action_t *);
extern ge_t *          ge_set_option(ge_t *, token_t *, token_t *);
extern char *          ge_tostring(ge_t *);

extern grammar_t *     grammar_create();
extern void            grammar_free(grammar_t *);
extern grammar_t *     grammar_set_parsing_strategy(grammar_t *, strategy_t);
extern strategy_t      grammar_get_parsing_strategy(grammar_t *);
extern nonterminal_t * grammar_get_nonterminal(grammar_t *, char *);
extern grammar_t *     grammar_set_lexer_option(grammar_t *, lexer_option_t, long);
extern long            grammar_get_lexer_option(grammar_t *, lexer_option_t);
extern void            grammar_dump(grammar_t *);
extern function_t *    grammar_resolve_function(grammar_t *, char *);
extern grammar_t *     grammar_analyze(grammar_t *);
extern char *          grammar_tostring(grammar_t *);

#define grammar_add_action(a)       (((grammar_t *) ge_add_action((a) -> ge)) -> ptr)
#define grammar_set_option(e, n, t) (((grammar_t *) ge_set_option((e) -> ge, (n), (t))) -> ptr)

extern nonterminal_t * nonterminal_create(grammar_t *, char *);
extern void            nonterminal_free(nonterminal_t *);
extern void            nonterminal_dump(nonterminal_t *);
extern rule_t *        nonterminal_get_rule(nonterminal_t *, int);
extern char *          nonterminal_tostring(nonterminal_t *);

#define nonterminal_get_grammar(e)      (((e) -> ge -> grammar))
#define nonterminal_add_pushvalue(e)    (((nonterminal_t *) ge_add_pushvalue((e) -> ge)) -> ptr)
#define nonterminal_add_incr(e)         (((nonterminal_t *) ge_add_incr((e) -> ge)) -> ptr)
#define nonterminal_set_initializer(e)  (((nonterminal_t *) ge_get_initializer((e) -> ge)) -> ptr)
#define nonterminal_get_initializer(e)  ge_get_initializer((e) -> ge)
#define nonterminal_set_finalizer(e)    (((nonterminal_t *) ge_get_finalizer((e) -> ge)) -> ptr)
#define nonterminal_get_finalizer(e)    ge_get_finalizer((e) -> ge)
#define nonterminal_set_option(e, n, t) (((nonterminal_t *) ge_set_option((e) -> ge, (n), (t))) -> ptr)

extern rule_t *        rule_create(nonterminal_t *);
extern void            rule_free(rule_t *);
extern void            rule_dump(rule_t *);
extern rule_entry_t *  rule_get_entry(rule_t *, int);
extern char *          rule_tostring(rule_t *);

#define rule_get_nonterminal(e)  ((nonterminal_t *) ((e) -> ge -> owner -> ptr))
#define rule_get_grammar(e)      (((e) -> ge -> grammar))
#define rule_add_action(a)       (((rule_t *) ge_add_pushvalue((a) -> ge)) -> ptr)
#define rule_set_option(e, n, t) (((rule_t *) ge_set_option((e) -> ge, (n), (t))) -> ptr)

extern rule_entry_t *  rule_entry_terminal(rule_t *, token_t *);
extern rule_entry_t *  rule_entry_non_terminal(rule_t *, char *);
extern rule_entry_t *  rule_entry_empty(rule_t *);
extern void            rule_entry_free(rule_entry_t *);
extern void            rule_entry_dump(rule_entry_t *);
extern char *          rule_entry_tostring(rule_entry_t *);

#define rule_entry_get_rule(e)         ((rule_t *) ((e) -> ge -> owner -> ptr))
#define rule_entry_get_grammar(e)      (((e) -> ge -> grammar))
#define rule_entry_add_pushvalue(e)    (((rule_entry_t *) ge_add_pushvalue((e) -> ge)) -> ptr)
#define rule_entry_add_incr(e)         (((rule_entry_t *) ge_add_incr((e) -> ge)) -> ptr)
#define rule_entry_set_initializer(e)  (((rule_entry_t *) ge_get_initializer((e) -> ge)) -> ptr)
#define rule_entry_get_initializer(e)  ge_get_initializer((e) -> ge)
#define rule_entry_set_finalizer(e)    (((rule_entry_t *) ge_get_finalizer((e) -> ge)) -> ptr)
#define rule_entry_get_finalizer(e)    ge_get_finalizer((e) -> ge)
#define rule_entry_set_option(e, n, t) (((rule_entry_t *) ge_set_option((e) -> ge, (n), (t))) -> ptr)

#endif /* __GRAMMAR_H__ */
