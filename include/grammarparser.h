/*
 * /obelix/include/grammarparser.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __GRAMMARPARSER_H__
#define __GRAMMARPARSER_H__

#include <grammar.h>

typedef enum _gp_state_ {
  GPStateStart,
  GPStateOptions,
  GPStateOptionName,
  GPStateOptionValue,
  GPStateHeader,
  GPStateNonTerminal,
  GPStateRule,
  GPStateEntry,
  GPStateModifier,
  GPStateSeparator,
  GPStateError
} gp_state_t;

typedef struct _grammar_parser {
  data_t        *reader;
  grammar_t     *grammar;
  gp_state_t     state;
  gp_state_t     old_state;
  token_t       *last_token;
  ge_t          *ge;
  nonterminal_t *nonterminal;
  rule_t        *rule;
  rule_entry_t  *entry;
  int            modifier;
  int            dryrun;
  dict_t        *keywords;
  unsigned int   next_keyword_code;
} grammar_parser_t;

OBLGRAMMAR_IMPEXP grammar_parser_t * grammar_parser_create(data_t *);
OBLGRAMMAR_IMPEXP void               grammar_parser_free(grammar_parser_t *);
OBLGRAMMAR_IMPEXP grammar_t *        grammar_parser_parse(grammar_parser_t *);

#endif /* __GRAMMARPARSER_H__ */
