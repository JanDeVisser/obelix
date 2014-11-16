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

#define LEXER_BUFSIZE    	16384
#define LEXER_SHORT_TOKEN_SZ	256

typedef enum _lexer_state {
  LexerStateFresh,
  LexerStateInit,
  LexerStateSuccess,
  LexerStateWhitespace,
  LexerStateNewLine,
  LexerStateIdentifier,
  LexerStateKeyword,
  LexerStateZero,
  LexerStateNumber,
  LexerStateDecimalInteger,
  LexerStateHexInteger,
  LexerStateFloat,
  LexerStateSciFloat,
  LexerStateQuotedStr,
  LexerStateQuotedStrEscape,
  LexerStateSlash,
  LexerStateBlockComment,
  LexerStateLineComment,
  LexerStateStar,
  LexerStateDone
} lexer_state_t;

typedef enum _token_code {
  TokenCodeError = -1,
  TokenCodeNone = 0,
  TokenCodeEmpty,
  TokenCodeWhitespace,
  TokenCodeNewLine,
  TokenCodeIdentifier = 'i',
  TokenCodeInteger = 'd',
  TokenCodeHexNumber = 'x',
  TokenCodeFloat = 'f',
  TokenCodeSQuotedStr = '\'',
  TokenCodeDQuotedStr = '\"',
  TokenCodeBQuotedStr = '`',
  TokenCodePlus = '+',
  TokenCodeMinus = '-',
  TokenCodeDot = '.',
  TokenCodeComma = ',',
  TokenCodeQMark = '?',
  TokenCodeExclPoint = '!',
  TokenCodeOpenPar = '(',
  TokenCodeClosePar = ')',
  TokenCodeOpenBrace = '{',
  TokenCodeCloseBrace = '}',
  TokenCodeOpenBracket = '[',
  TokenCodeCloseBracket = ']',
  TokenCodeLAngle = '<',
  TokenCodeRangle = '>',
  TokenCodeAsterisk = '*',
  TokenCodeSlash = '/',
  TokenCodeBackslash = '\\',
  TokenCodeColon = ':',
  TokenCodeSemiColon = ';',
  TokenCodeEquals = '=',
  TokenCodePipe = '|',
  TokenCodeAt = '@',
  TokenCodeHash = '#',
  TokenCodeDollar = '$',
  TokenCodePercent = '%',
  TokenCodeHat = '^',
  TokenCodeAmpersand = '&',
  TokenCodeTilde = '~',
  TokenCodeEnd = 199
} token_code_t;

typedef enum _lexer_option {
  LexerOptionIgnoreWhitespace,
  LexerOptionIgnoreNewLines,
  LexerOptionCaseSensitive,
  LexerOptionLAST
} lexer_option_t;

typedef struct _token {
  int   code;
  char *token;
} token_t;

typedef struct _lexer {
  void          *reader;
  list_t        *keywords;
  char           buf[LEXER_BUFSIZE];
  int            buflen;
  int            bufpos;
  char          *old_token;
  int            old_token_pos;
  array_t       *options;
  char           short_token[LEXER_SHORT_TOKEN_SZ];
  char          *current_token;
  int            token_size;
  int            current_token_len;
  lexer_state_t  state;
  list_t        *matches;
  int            no_kw_match;
  char           quote;
} lexer_t;

extern char *       lexer_state_name(lexer_state_t);
extern char *       token_code_name(token_code_t);

extern token_t *    token_create(int, char *);
extern token_t *    token_copy(token_t *);
extern void         token_free(token_t *);
extern unsigned int token_hash(token_t *);
extern int          token_cmp(token_t *, token_t *);
extern int          token_code(token_t *);
extern char *       token_token(token_t *);
extern int          token_iswhitespace(token_t *);
extern void         token_dump(token_t *);
extern char *       token_tostring(token_t *, char *, int);

extern lexer_t *    lexer_create(void *);
extern lexer_t *    lexer_set_option(lexer_t *, lexer_option_t, long);
extern long         lexer_get_option(lexer_t *, lexer_option_t);
extern lexer_t *    lexer_add_keyword(lexer_t *, token_t *);
extern void         lexer_free(lexer_t *);
extern void         _lexer_tokenize(lexer_t *, reduce_t, void *);
extern token_t *    lexer_next_token(lexer_t *);

#define lexer_tokenize(l, r, d) _lexer_tokenize((l), (reduce_t) (r), (d))

#endif /* __LEXER_H__ */
