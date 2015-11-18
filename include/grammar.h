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

#include <libparser.h>
#include <array.h>
#include <dict.h>
#include <function.h>
#include <lexer.h>
#include <resolve.h>
#include <set.h>

#ifdef __cplusplus
extern "C" {
#endif

OBLPARSER_IMPEXP int grammar_debug;

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
#define ON_NEWLINE_STR          "on_newline"

/* ----------------------------------------------------------------------- */

typedef enum _parsing_strategy {
  ParsingStrategyTopDown,
  ParsingStrategyBottomUp
} strategy_t;

typedef struct _grammar_action {
  data_t      _d;
  function_t *fnc;
  data_t     *data;
} grammar_action_t;

/* ----------------------------------------------------------------------- */

typedef int grammar_element_type_t;
OBLPARSER_IMPEXP grammar_element_type_t   GrammarAction;
OBLPARSER_IMPEXP grammar_element_type_t   GrammarElement;
OBLPARSER_IMPEXP grammar_element_type_t   Grammar;
OBLPARSER_IMPEXP grammar_element_type_t   NonTerminal;
OBLPARSER_IMPEXP grammar_element_type_t   Rule;
OBLPARSER_IMPEXP grammar_element_type_t   RuleEntry;
OBLPARSER_IMPEXP grammar_element_type_t   Terminal;

struct _grammar;
struct _grammar_element;

typedef void * (*set_option_t)(struct _grammar_element *, token_t *, token_t *);

typedef struct _grammar_element {
  data_t                   _d;
  struct _grammar         *grammar;
  struct _grammar_element *owner;
  list_t                  *actions;
  dict_t                  *variables;
  set_option_t             set_option_delegate;
} ge_t;

typedef struct _nonterminal {
  ge_t                     ge;
  int                      state;
  char                    *name;
  array_t                 *rules;
  set_t                   *firsts;
  set_t                   *follows;
  dict_t                  *parse_table;
} nonterminal_t;

typedef struct _rule {
  ge_t                     ge;
  array_t                 *entries;
  set_t                   *firsts;
  set_t                   *follows;
} rule_t;

typedef struct _rule_entry {
  ge_t                     ge;
  int                      terminal;
  union {
    token_t               *token;
    char                  *nonterminal;
  };
} rule_entry_t;

typedef struct _grammar {
  ge_t           ge;
  dict_t        *nonterminals;
  nonterminal_t *entrypoint;
  dict_t        *keywords;
  array_t       *lexer_options;
  strategy_t     strategy;
  char          *prefix;
  array_t       *libs;
  int            dryrun;
} grammar_t;

/* ----------------------------------------------------------------------- */

OBLPARSER_IMPEXP grammar_action_t *      grammar_action_create(function_t *, data_t *);
OBLPARSER_IMPEXP int                     grammar_action_cmp(grammar_action_t *, grammar_action_t *);
OBLPARSER_IMPEXP unsigned int            grammar_action_hash(grammar_action_t *);

#define grammar_action_copy(ga)          ((grammar_action_t *) data_copy((data_t *) (ga)))
#define grammar_action_free(ga)          (data_free((data_t *) (ga)))
#define grammar_action_tostring(ga)      (data_tostring((data_t *) (ga)))

OBLPARSER_IMPEXP void_t                  ge_function(ge_t *, int);
OBLPARSER_IMPEXP ge_t *                  ge_add_action(ge_t *, grammar_action_t *);
OBLPARSER_IMPEXP ge_t *                  ge_set_option(ge_t *, token_t *, token_t *);

#define ge_typedescr(ge)                 (data_typedescr((data_t *) (ge)))
#define ge_copy(ge)                      ((ge_t *) data_copy((data_t *) (ge)))
#define ge_free(ge)                      (data_free((data_t *) (ge)))
#define ge_tostring(ge)                  (data_tostring((data_t *) (ge)))

OBLPARSER_IMPEXP grammar_t *             grammar_create();
OBLPARSER_IMPEXP grammar_t *             grammar_set_parsing_strategy(grammar_t *, strategy_t);
OBLPARSER_IMPEXP strategy_t              grammar_get_parsing_strategy(grammar_t *);
OBLPARSER_IMPEXP nonterminal_t *         grammar_get_nonterminal(grammar_t *, char *);
OBLPARSER_IMPEXP grammar_t *             grammar_set_lexer_option(grammar_t *, lexer_option_t, long);
OBLPARSER_IMPEXP long                    grammar_get_lexer_option(grammar_t *, lexer_option_t);
OBLPARSER_IMPEXP void                    grammar_dump(grammar_t *);
OBLPARSER_IMPEXP function_t *            grammar_resolve_function(grammar_t *, char *);
OBLPARSER_IMPEXP grammar_t *             grammar_analyze(grammar_t *);

#define grammar_add_action(g, a)         ((grammar_t *) ge_add_action((ge_t *) (g), (a)))
#define grammar_set_option(g, n, t)      ((grammar_t *) ge_set_option((ge_t *) (g), (n), (t)))
#define grammar_copy(g)                  ((grammar_t *) data_copy((data_t *) (g)))
#define grammar_free(g)                  (data_free((data_t *) (g)))
#define grammar_tostring(g)              (data_tostring((data_t *) (g)))

OBLPARSER_IMPEXP nonterminal_t *         nonterminal_create(grammar_t *, char *);
OBLPARSER_IMPEXP void                    nonterminal_dump(nonterminal_t *);
OBLPARSER_IMPEXP rule_t *                nonterminal_get_rule(nonterminal_t *, int);

#define nonterminal_get_grammar(nt)      ((((ge_t *) (nt)) -> grammar))
#define nonterminal_set_option(nt, n, t) ((nonterminal_t *) ge_set_option((ge_t *) (nt), (n), (t)))
#define nonterminal_add_action(nt, a)    ((nonterminal_t *) ge_add_action((ge_t *) (nt), (a)))
#define nonterminal_copy(nt)             ((nonterminal_t *) data_copy((data_t *) (nt)))
#define nonterminal_free(nt)             (data_free((data_t *) (nt)))
#define nonterminal_tostring(nt)         (data_tostring((data_t *) (nt)))

OBLPARSER_IMPEXP rule_t *                rule_create(nonterminal_t *);
OBLPARSER_IMPEXP void                    rule_dump(rule_t *);
OBLPARSER_IMPEXP rule_entry_t *          rule_get_entry(rule_t *, int);

#define rule_get_grammar(r)              ((((ge_t *) (r)) -> grammar))
#define rule_get_nonterminal(r)          ((nonterminal_t *) ((((ge_t *) (r)) -> owner)))
#define rule_set_option(r, n, t)         ((rule_t *) ge_set_option((ge_t *) (r), (n), (t)))
#define rule_add_action(r, a)            ((rule_t *) ge_add_action((ge_t *) (r), (a)))
#define rule_copy(r)                     ((rule_t *) data_copy((data_t *) (r)))
#define rule_free(r)                     (data_free((data_t *) (r)))
#define rule_tostring(r)                 (data_tostring((data_t *) (r)))

OBLPARSER_IMPEXP rule_entry_t *          rule_entry_terminal(rule_t *, token_t *);
OBLPARSER_IMPEXP rule_entry_t *          rule_entry_non_terminal(rule_t *, char *);
OBLPARSER_IMPEXP rule_entry_t *          rule_entry_empty(rule_t *);
OBLPARSER_IMPEXP void                    rule_entry_dump(rule_entry_t *);

#define rule_entry_get_rule(re)          ((rule_t *) ((re) -> owner))
#define rule_entry_get_grammar(re)       ((((ge_t *) (re)) -> grammar))
#define rule_entry_set_option(re, n, t)  ((rule_entry_t *) ge_set_option((ge_t *) (re), (n), (t)))
#define rule_entry_add_action(re, a)     ((rule_entry_t *) ge_add_action((ge_t *) (re), (a)))
#define rule_entry_copy(re)              ((rule_entry_t *) data_copy((data_t *) (re)))
#define rule_entry_free(re)              (data_free((data_t *) (re)))
#define rule_entry_tostring(re)          (data_tostring((data_t *) (re)))

#ifdef __cplusplus
}
#endif

#endif /* __GRAMMAR_H__ */
