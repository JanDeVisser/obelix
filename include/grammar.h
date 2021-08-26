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

#include <oblconfig.h>
#include <array.h>
#include <dict.h>
#include <function.h>
#include <lexer.h>
#include <resolve.h>
#include <set.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int grammar_debug;

#define NONTERMINAL_DEF         200
#define NONTERMINAL_DEF_STR     ":="

/* grammar-wide options */
#define LIB_STR                 "lib"
#define PREFIX_STR              "prefix"
#define STRATEGY_STR            "strategy"
#define LEXER_STR               "lexer"
#define GRAMMAR_BUILD_FUNC_STR  "grammar_buildfunc"
#define LEXERCFG_BUILD_FUNC_STR "lexercfg_buildfunc"

/* ----------------------------------------------------------------------- */

typedef enum _parsing_strategy {
  ParsingStrategyTopDown,
  ParsingStrategyBottomUp
} strategy_t;

typedef struct _grammar_action {
  data_t      _d;
  data_t     *owner;
  function_t *fnc;
  data_t     *data;
} grammar_action_t;

typedef struct _grammar_variable {
  data_t  _d;
  data_t *value;
  data_t *owner;
} grammar_variable_t;

/* ----------------------------------------------------------------------- */

struct _grammar;
struct _grammar_element;

typedef struct _grammar_element {
  data_t                   _d;
  struct _grammar         *grammar;
  struct _grammar_element *owner;
  datalist_t              *actions;
  dictionary_t            *variables;
} ge_t;

typedef struct _nonterminal {
  ge_t          ge;
  unsigned int  state;
  datalist_t   *rules;
  set_t        *firsts;
  set_t        *follows;
  dict_t       *parse_table;
} nonterminal_t;

typedef struct _rule {
  ge_t                     ge;
  datalist_t              *entries;
  set_t                   *firsts;
  set_t                   *follows;
} rule_t;

typedef struct _rule_entry {
  ge_t     ge;
  int      terminal;
  token_t *token;
  char    *nonterminal;
} rule_entry_t;

typedef struct _grammar {
  ge_t            ge;
  dictionary_t   *nonterminals;
  nonterminal_t  *entrypoint;
  dict_t         *keywords;
  lexer_config_t *lexer;
  strategy_t      strategy;
  char           *prefix;
  char           *build_func;
  array_t        *libs;
  int             dryrun;
} grammar_t;

/* ----------------------------------------------------------------------- */

extern grammar_action_t *      grammar_action_create(function_t *, data_t *);
extern int                     grammar_action_cmp(grammar_action_t *, grammar_action_t *);
extern unsigned int            grammar_action_hash(grammar_action_t *);

#define grammar_action_tostring(ga)       (data_tostring((data_t *) (ga)))

extern grammar_variable_t *    grammar_variable_create(ge_t *, char *, data_t *);
#define grammar_variable_tostring(gv)     (data_tostring((data_t *) (gv)))

extern void_t                  ge_function(ge_t *, int);
extern ge_t *                  ge_add_action(ge_t *, grammar_action_t *);
extern ge_t *                  ge_set_variable(ge_t *, char *, data_t *);
extern grammar_variable_t *    ge_get_variable(ge_t *, char *);
extern ge_t *                  ge_set_option(ge_t *, token_t *, token_t *);
extern ge_t *                  ge_dump(ge_t *);

#define ge_tostring(ge)                   (data_tostring((data_t *) (ge)))

extern grammar_t *             grammar_create();
extern grammar_t *             grammar_set_parsing_strategy(grammar_t *, strategy_t);
extern function_t *            grammar_resolve_function(grammar_t *, char *);
extern grammar_t *             grammar_analyze(grammar_t *);

#define grammar_add_action(g, a)          ((grammar_t *) ge_add_action((ge_t *) (g), (a)))
#define grammar_set_variable(g, n, v)     ((grammar_t *) ge_set_variable((ge_t *) (g), (n), (v)))
#define grammar_set_option(g, n, t)       ((grammar_t *) ge_set_option((ge_t *) (g), (n), (t)))
#define grammar_get_nonterminal(g, r)     ((nonterminal_t *) dictionary_get((g) -> nonterminals, (r)))
#define grammar_get_parsing_strategy(g)   ((g) -> strategy)
#define grammar_dump(g)                   ((grammar_t *) ge_dump((ge_t *) (g)))
#define grammar_tostring(g)               (data_tostring((data_t *) (g)))

extern nonterminal_t *         nonterminal_create(grammar_t *, char *);
extern void                    nonterminal_dump(nonterminal_t *);
extern rule_t *                nonterminal_get_rule(nonterminal_t *, int);

#define nonterminal_get_grammar(nt)        ((((ge_t *) (nt)) -> grammar))
#define nonterminal_set_option(nt, n, t)   ((nonterminal_t *) ge_set_option((ge_t *) (nt), (n), (t)))
#define nonterminal_set_variable(nt, n, v) ((nonterminal_t *) ge_set_variable((ge_t *) (nt), (n), (v)))
#define nonterminal_add_action(nt, a)      ((nonterminal_t *) ge_add_action((ge_t *) (nt), (a)))
#define nonterminal_dump(nt)               ((nonterminal_t *) ge_dump((ge_t *) (nt)))
#define nonterminal_tostring(nt)           (data_tostring((data_t *) (nt)))

extern rule_t *                rule_create(nonterminal_t *);
extern rule_entry_t *          rule_get_entry(rule_t *, int);

#define rule_get_grammar(r)               ((((ge_t *) (r)) -> grammar))
#define rule_get_nonterminal(r)           ((nonterminal_t *) ((((ge_t *) (r)) -> owner)))
#define rule_set_variable(r, n, v)        ((rule_t *) ge_set_variable((ge_t *) (r), (n), (v)))
#define rule_set_option(r, n, t)          ((rule_t *) ge_set_option((ge_t *) (r), (n), (t)))
#define rule_add_action(r, a)             ((rule_t *) ge_add_action((ge_t *) (r), (a)))
#define rule_tostring(r)                  (data_tostring((data_t *) (r)))

extern rule_entry_t *          rule_entry_terminal(rule_t *, token_t *);
extern rule_entry_t *          rule_entry_non_terminal(rule_t *, char *);
extern rule_entry_t *          rule_entry_empty(rule_t *);

#define rule_entry_get_rule(re)           ((rule_t *) ((re) -> owner))
#define rule_entry_get_grammar(re)        ((((ge_t *) (re)) -> grammar))
#define rule_entry_set_variable(re, n, v) ((rule_entry_t *) ge_set_variable((ge_t *) (re), (n), (v)))
#define rule_entry_set_option(re, n, t)   ((rule_entry_t *) ge_set_option((ge_t *) (re), (n), (t)))
#define rule_entry_add_action(re, a)      ((rule_entry_t *) ge_add_action((ge_t *) (re), (a)))
#define rule_entry_dump(re)               ((rule_entry_t *) ge_dump((ge_t *) (r)))
#define rule_entry_free(re)               (data_free((data_t *) (re)))
#define rule_entry_tostring(re)           (data_tostring((data_t *) (re)))

#ifdef __cplusplus
}
#endif

#endif /* __GRAMMAR_H__ */
