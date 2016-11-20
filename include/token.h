/*
 * token.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TOKEN_H__
#define	__TOKEN_H__

#include <data.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _token_code {
  TokenCodeNone = 0,
  TokenCodeError,
  TokenCodeEmpty,
  TokenCodeWhitespace = ' ',
  TokenCodeRawString,
  TokenCodeNewLine = '\n',
  TokenCodeLastToken,
  TokenCodeEnd,
  TokenCodeExhausted,
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
  TokenCodeTilde = '~'
} token_code_t;

typedef struct _token {
  data_t        _d;
  unsigned int  code;
  int           size;
  char         *token;
  int           line;
  int           column;
} token_t;

OBLLEXER_IMPEXP char *       token_code_name(token_code_t);

OBLLEXER_IMPEXP token_t *    token_create(unsigned int, char *);
OBLLEXER_IMPEXP token_t *    token_parse(char *);
OBLLEXER_IMPEXP unsigned int token_hash(token_t *);
OBLLEXER_IMPEXP int          token_cmp(token_t *, token_t *);
OBLLEXER_IMPEXP unsigned int token_code(token_t *);
OBLLEXER_IMPEXP char *       token_token(token_t *);
OBLLEXER_IMPEXP token_t *    token_assign(token_t *, unsigned int, char *);
OBLLEXER_IMPEXP int          token_iswhitespace(token_t *);
OBLLEXER_IMPEXP void         token_dump(token_t *);
OBLLEXER_IMPEXP data_t *     token_todata(token_t *);

OBLLEXER_IMPEXP int Token;

#define data_is_token(d)     ((d) && data_hastype((d), Token))
#define data_tokenval(d)     (data_is_token((d)) ? ((lexer_t *) ((d) -> ptrval)) : NULL)
#define token_copy(t)        ((token_t *) data_copy((data_t *) (t)))
#define token_free(t)        (data_free((data_t *) (t)))
#define token_tostring(t)    (data_tostring((data_t *) (t)))

#define strtoken_dict_create()  strdata_dict_create()
#define tokenset_create()       data_set_create()

#ifdef __cplusplus
}
#endif

#endif /* __TOKEN_H__ */
