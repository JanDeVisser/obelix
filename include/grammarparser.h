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

#include <core.h>
#include <grammar.h>

typedef enum _gp_state_ {
  GPStateStart,
  GPStateOptions,
  GPStateOptionName,
  GPStateHeader,
  GPStateNonTerminal,
  GPStateRule,
  GPStateEntry,
  GPStateError
} gp_state_t;

typedef struct _grammar_parser {
  reader_t      *reader;
  grammar_t     *grammar;
  gp_state_t     state;
  gp_state_t     old_state;
  char          *string;
  ge_t          *ge;
  nonterminal_t *nonterminal;
  rule_t        *rule;
  rule_entry_t  *entry;
} grammar_parser_t;

extern grammar_parser_t * _grammar_parser_create(reader_t *);
extern void               grammar_parser_free(grammar_parser_t *);
extern grammar_t *        grammar_parser_parse(grammar_parser_t *);

#define grammar_parser_create(r) _grammar_parser_create(((reader_t *) (r)))

#endif /* __GRAMMARPARSER_H__ */
