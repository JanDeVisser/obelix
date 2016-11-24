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

#include <config.h>
#include <array.h>
#include <dict.h>
#include <function.h>
#include <lexer.h>
#include <resolve.h>
#include <set.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OBLGRAMMAR_IMPEXP
  #if (defined __WIN32__) || (defined _WIN32)
    #ifdef oblgrammar_EXPORTS
      #define OBLGRAMMAR_IMPEXP	__DLL_EXPORT__
    #elif defined(OBL_STATIC)
      #define OBLGRAMMAR_IMPEXP extern
    #else /* ! oblgrammar_EXPORTS */
      #define OBLGRAMMAR_IMPEXP	__DLL_IMPORT__
    #endif
  #else /* ! __WIN32__ */
    #define OBLGRAMMAR_IMPEXP extern
  #endif /* __WIN32__ */
#endif /* OBLGRAMMAR_IMPEXP */

OBLGRAMMAR_IMPEXP int grammar_debug;

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

typedef int grammar_element_type_t;
OBLGRAMMAR_IMPEXP grammar_element_type_t   GrammarAction;
OBLGRAMMAR_IMPEXP grammar_element_type_t   GrammarVariable;
OBLGRAMMAR_IMPEXP grammar_element_type_t   GrammarElement;
OBLGRAMMAR_IMPEXP grammar_element_type_t   Grammar;
OBLGRAMMAR_IMPEXP grammar_element_type_t   NonTerminal;
OBLGRAMMAR_IMPEXP grammar_element_type_t   Rule;
OBLGRAMMAR_IMPEXP grammar_element_type_t   RuleEntry;

struct _grammar;
struct _grammar_element;

typedef struct _grammar_element {
  data_t                   _d;
  struct _grammar         *grammar;
  struct _grammar_element *owner;
  list_t                  *actions;
  dict_t                  *variables;
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
  ge_t            ge;
  dict_t         *nonterminals;
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

OBLGRAMMAR_IMPEXP grammar_action_t *      grammar_action_create(function_t *, data_t *);
OBLGRAMMAR_IMPEXP int                     grammar_action_cmp(grammar_action_t *, grammar_action_t *);
OBLGRAMMAR_IMPEXP unsigned int            grammar_action_hash(grammar_action_t *);

#define grammar_action_copy(ga)           ((grammar_action_t *) data_copy((data_t *) (ga)))
#define grammar_action_free(ga)           (data_free((data_t *) (ga)))
#define grammar_action_tostring(ga)       (data_tostring((data_t *) (ga)))

OBLGRAMMAR_IMPEXP grammar_variable_t *    grammar_variable_create(ge_t *, char *, data_t *);
#define grammar_variable_copy(gv)         ((grammar_variable_t *) data_copy((data_t *) (gv)))
#define grammar_variable_free(gv)         (data_free((data_t *) (gv)))
#define grammar_variable_tostring(gv)     (data_tostring((data_t *) (gv)))

OBLGRAMMAR_IMPEXP void_t                  ge_function(ge_t *, int);
OBLGRAMMAR_IMPEXP ge_t *                  ge_add_action(ge_t *, grammar_action_t *);
OBLGRAMMAR_IMPEXP ge_t *                  ge_set_variable(ge_t *, char *, data_t *);
OBLGRAMMAR_IMPEXP grammar_variable_t *    ge_get_variable(ge_t *, char *);
OBLGRAMMAR_IMPEXP ge_t *                  ge_set_option(ge_t *, token_t *, token_t *);
OBLGRAMMAR_IMPEXP ge_t *                  ge_dump(ge_t *);

#define ge_typedescr(ge)                  (data_typedescr((data_t *) (ge)))
#define ge_copy(ge)                       ((ge_t *) data_copy((data_t *) (ge)))
#define ge_free(ge)                       (data_free((data_t *) (ge)))
#define ge_tostring(ge)                   (data_tostring((data_t *) (ge)))

OBLGRAMMAR_IMPEXP grammar_t *             grammar_create();
OBLGRAMMAR_IMPEXP grammar_t *             grammar_set_parsing_strategy(grammar_t *, strategy_t);
OBLGRAMMAR_IMPEXP function_t *            grammar_resolve_function(grammar_t *, char *);
OBLGRAMMAR_IMPEXP grammar_t *             grammar_analyze(grammar_t *);

#define grammar_add_action(g, a)          ((grammar_t *) ge_add_action((ge_t *) (g), (a)))
#define grammar_set_variable(g, n, v)     ((grammar_t *) ge_set_variable((ge_t *) (g), (n), (v)))
#define grammar_set_option(g, n, t)       ((grammar_t *) ge_set_option((ge_t *) (g), (n), (t)))
#define grammar_get_nonterminal(g, r)     ((nonterminal_t *) dict_get((g) -> nonterminals, (r)))
#define grammar_get_parsing_strategy(g)   ((g) -> strategy)
#define grammar_dump(g)                   ((grammar_t *) ge_dump((ge_t *) (g)))
#define grammar_copy(g)                   ((grammar_t *) data_copy((data_t *) (g)))
#define grammar_free(g)                   (data_free((data_t *) (g)))
#define grammar_tostring(g)               (data_tostring((data_t *) (g)))

OBLGRAMMAR_IMPEXP nonterminal_t *         nonterminal_create(grammar_t *, char *);
OBLGRAMMAR_IMPEXP void                    nonterminal_dump(nonterminal_t *);
OBLGRAMMAR_IMPEXP rule_t *                nonterminal_get_rule(nonterminal_t *, int);

#define nonterminal_get_grammar(nt)        ((((ge_t *) (nt)) -> grammar))
#define nonterminal_set_option(nt, n, t)   ((nonterminal_t *) ge_set_option((ge_t *) (nt), (n), (t)))
#define nonterminal_set_variable(nt, n, v) ((nonterminal_t *) ge_set_variable((ge_t *) (nt), (n), (v)))
#define nonterminal_add_action(nt, a)      ((nonterminal_t *) ge_add_action((ge_t *) (nt), (a)))
#define nonterminal_dump(nt)                ((nonterminal_t *) ge_dump((ge_t *) (nt)))
#define nonterminal_copy(nt)               ((nonterminal_t *) data_copy((data_t *) (nt)))
#define nonterminal_free(nt)               (data_free((data_t *) (nt)))
#define nonterminal_tostring(nt)           (data_tostring((data_t *) (nt)))

OBLGRAMMAR_IMPEXP rule_t *                rule_create(nonterminal_t *);

#define rule_get_grammar(r)               ((((ge_t *) (r)) -> grammar))
#define rule_get_nonterminal(r)           ((nonterminal_t *) ((((ge_t *) (r)) -> owner)))
#define rule_set_variable(r, n, v)        ((rule_t *) ge_set_variable((ge_t *) (r), (n), (v)))
#define rule_set_option(r, n, t)          ((rule_t *) ge_set_option((ge_t *) (r), (n), (t)))
#define rule_add_action(r, a)             ((rule_t *) ge_add_action((ge_t *) (r), (a)))
#define rule_get_entry(r, ix)             ((rule_entry_t *) array_get(r -> entries, ix))
#define rule_dump(r)                      ((rule_t *) ge_dump((ge_t *) (r)))
#define rule_copy(r)                      ((rule_t *) data_copy((data_t *) (r)))
#define rule_free(r)                      (data_free((data_t *) (r)))
#define rule_tostring(r)                  (data_tostring((data_t *) (r)))

OBLGRAMMAR_IMPEXP rule_entry_t *          rule_entry_terminal(rule_t *, token_t *);
OBLGRAMMAR_IMPEXP rule_entry_t *          rule_entry_non_terminal(rule_t *, char *);
OBLGRAMMAR_IMPEXP rule_entry_t *          rule_entry_empty(rule_t *);

#define rule_entry_get_rule(re)           ((rule_t *) ((re) -> owner))
#define rule_entry_get_grammar(re)        ((((ge_t *) (re)) -> grammar))
#define rule_entry_set_variable(re, n, v) ((rule_entry_t *) ge_set_variable((ge_t *) (re), (n), (v)))
#define rule_entry_set_option(re, n, t)   ((rule_entry_t *) ge_set_option((ge_t *) (re), (n), (t)))
#define rule_entry_add_action(re, a)      ((rule_entry_t *) ge_add_action((ge_t *) (re), (a)))
#define rule_entry_dump(re)               ((rule_entry_t *) ge_dump((ge_t *) (r)))
#define rule_entry_copy(re)               ((rule_entry_t *) data_copy((data_t *) (re)))
#define rule_entry_free(re)               (data_free((data_t *) (re)))
#define rule_entry_tostring(re)           (data_tostring((data_t *) (re)))

#ifdef __cplusplus
}
#endif

#endif /* __GRAMMAR_H__ */
