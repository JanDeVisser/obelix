/*
 * /obelix/src/lexer.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LEXER_H__
#define __LEXER_H__

#include <array.h>
#include <data.h>
#include <dict.h>
#include <str.h>
#include <token.h>

#define LEXER_BUFSIZE    	16384
#define LEXER_INIT_TOKEN_SZ	256

extern int lexer_debug;

typedef enum _lexer_state {
  LexerStateFresh,
  LexerStateInit,
  LexerStateSuccess,
  LexerStateWhitespace,
  LexerStateNewLine,
  LexerStateIdentifier,
  LexerStateKeyword,
  LexerStatePlusMinus,
  LexerStateZero,
  LexerStateNumber,
  LexerStateDecimalInteger,
  LexerStateHexInteger,
  LexerStateFloat,
  LexerStateSciFloat,
  LexerStateQuotedStr,
  LexerStateQuotedStrEscape,
  LexerStateHashPling,
  LexerStateSlash,
  LexerStateBlockComment,
  LexerStateLineComment,
  LexerStateStar,
  LexerStateDone
} lexer_state_t;

typedef enum _lexer_option {
  LexerOptionIgnoreWhitespace,
  LexerOptionIgnoreNewLines,
  LexerOptionIgnoreAllWhitespace,
  LexerOptionCaseSensitive,
  LexerOptionHashPling,
  LexerOptionSignedNumbers,
  LexerOptionOnNewLine,
  LexerOptionLAST
} lexer_option_t;

typedef enum _kw_match_state {
  KMSInit,
  KMSPrefixMatched,
  KMSPrefixesMatched,
  KMSIdentifierFullMatch,
  KMSIdentifierFullMatchAndPrefixes,
  KMSFullMatch,
  KMSFullMatchAndPrefixes,
  KMSMatchLost,
  KMSNoMatch
} kw_match_state_t;

typedef struct _lexer {
  data_t             *reader;
  list_t             *keywords;
  long                options[LexerOptionLAST];
  str_t              *buffer;
  str_t              *pushed_back;
  str_t              *token;
  lexer_state_t       state;
  token_t            *last_match;
  struct _kw_matches *matches;
  char                quote;
  int                 prev_char;
  int                 line;
  int                 column;
  void               *data;
} lexer_t;

extern char *       lexer_state_name(lexer_state_t);
extern char *       lexer_option_name(lexer_option_t);

extern lexer_t *    lexer_create(data_t *);
extern lexer_t *    lexer_set_option(lexer_t *, lexer_option_t, long);
extern long         lexer_get_option(lexer_t *, lexer_option_t);
extern lexer_t *    lexer_add_keyword(lexer_t *, int, char *);
extern void         lexer_free(lexer_t *);
extern void         _lexer_tokenize(lexer_t *, reduce_t, void *);
extern token_t *    lexer_next_token(lexer_t *);
extern token_t *    lexer_rollup_to(lexer_t *, int);

#define lexer_tokenize(l, r, d) _lexer_tokenize((l), (reduce_t) (r), (d))

#endif /* __LEXER_H__ */
