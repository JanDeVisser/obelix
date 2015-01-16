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

/* generic options (can be set on any grammar element) */
#define PUSH                    201
#define PUSH_STR                "push"
#define PUSH_INCR               202
#define PUSH_INCR_STR           "push++"
#define INIT                    203
#define INIT_STR                "init"
#define DONE                    204
#define DONE_STR                "done"
#define INCR                    205
#define INCR_STR                "++"

/* grammar-wide options */
#define LIB                     210
#define LIB_STR                 "lib"
#define PREFIX                  211
#define PREFIX_STR              "prefix"
#define STRATEGY                212
#define STRATEGY_STR            "strategy"
#define IGNORE                  213
#define IGNORE_STR              "ignore"
#define CASE_SENSITIVE          214
#define CASE_SENSITIVE_STR      "case_sensitive"
#define HASHPLING               215
#define HASHPLING_STR           "hashpling"
#define SIGNED_NUMBERS          216
#define SIGNED_NUMBERS_STR      "signed_numbers"

typedef enum _parsing_strategy {
  ParsingStrategyTopDown,
  ParsingStrategyBottomUp
} strategy_t;

typedef enum _grammar_element_type {
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
  function_t              *initializer;
  function_t              *finalizer;
  set_t                   *pushvalues;
  dict_t                  *variables;
  set_option_t             set_option_delegate;
  void                    *ptr;
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

typedef struct _pushvalue {
  token_t                 *value;
  int                      incr;
} pushvalue_t;

extern pushvalue_t *   pushvalue_create(token_t *, int);
extern pushvalue_t *   pushvalue_copy(pushvalue_t *);
extern void            pushvalue_free(pushvalue_t *);
extern int             pushvalue_cmp(pushvalue_t *, pushvalue_t *);
extern unsigned int    pushvalue_hash(pushvalue_t *);
extern char *          pushvalue_tostring(pushvalue_t *pushvalue);

extern void            ge_free(ge_t *);
extern ge_t *          ge_add_pushvalue(ge_t *, pushvalue_t *);
extern ge_t *          ge_set_initializer(ge_t *, function_t *);
extern function_t *    ge_get_initializer(ge_t *);
extern ge_t *          ge_set_finalizer(ge_t *, function_t *);
extern function_t *    ge_get_finalizer(ge_t *);
extern ge_t *          ge_set_option(ge_t *, token_t *, token_t *);

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

#define grammar_add_pushvalue(e)    (((grammar_t *) ge_add_pushvalue((e) -> ge)) -> ptr)
#define grammar_set_initializer(e)  (((grammar_t *) ge_get_initializer((e) -> ge)) -> ptr)
#define grammar_get_initializer(e)  ge_get_initializer((e) -> ge)
#define grammar_set_finalizer(e)    (((grammar_t *) ge_get_finalizer((e) -> ge)) -> ptr)
#define grammar_get_finalizer(e)    ge_get_finalizer((e) -> ge)
#define grammar_set_option(e, n, t) (((grammar_t *) ge_set_option((e) -> ge, (n), (t))) -> ptr)

extern nonterminal_t * nonterminal_create(grammar_t *, char *);
extern void            nonterminal_free(nonterminal_t *);
extern void            nonterminal_dump(nonterminal_t *);
extern rule_t *        nonterminal_get_rule(nonterminal_t *, int);

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

#define rule_get_nonterminal(e)  ((nonterminal_t *) ((e) -> ge -> owner -> ptr))
#define rule_get_grammar(e)      (((e) -> ge -> grammar))
#define rule_add_pushvalue(e)    (((rule_t *) ge_add_pushvalue((e) -> ge)) -> ptr)
#define rule_add_incr(e)         (((rule_t *) ge_add_incr((e) -> ge)) -> ptr)
#define rule_set_initializer(e)  (((rule_t *) ge_get_initializer((e) -> ge)) -> ptr)
#define rule_get_initializer(e)  ge_get_initializer((e) -> ge)
#define rule_set_finalizer(e)    (((rule_t *) ge_get_finalizer((e) -> ge)) -> ptr)
#define rule_get_finalizer(e)    ge_get_finalizer((e) -> ge)
#define rule_set_option(e, n, t) (((rule_t *) ge_set_option((e) -> ge, (n), (t))) -> ptr)

extern rule_entry_t *  rule_entry_terminal(rule_t *, token_t *);
extern rule_entry_t *  rule_entry_non_terminal(rule_t *, char *);
extern rule_entry_t *  rule_entry_empty(rule_t *);
extern void            rule_entry_free(rule_entry_t *);
extern void            rule_entry_dump(rule_entry_t *);

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
