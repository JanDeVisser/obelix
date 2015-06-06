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

typedef enum _token_code {
  TokenCodeNone = 0,
  TokenCodeError,
  TokenCodeEmpty,
  TokenCodeWhitespace,
  TokenCodeRawString,
  TokenCodeNewLine,
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
  char         *token;
  int           line;
  int           column;
} token_t;

extern char *       token_code_name(token_code_t);

extern token_t *    token_create(unsigned int, char *);
extern unsigned int token_hash(token_t *);
extern int          token_cmp(token_t *, token_t *);
extern unsigned int token_code(token_t *);
extern char *       token_token(token_t *);
extern int          token_iswhitespace(token_t *);
extern void         token_dump(token_t *);
extern data_t *     token_todata(token_t *);

extern int Token;

#define data_is_token(d)     ((d) && data_hastype((d), Token))
#define data_tokenval(d)     (data_is_token((d)) ? ((lexer_t *) ((d) -> ptrval)) : NULL)
#define token_copy(t)        ((token_t *) data_copy((data_t *) (t)))
#define token_free(t)        (data_free((data_t *) (t)))
#define token_tostring(t)    (data_tostring((data_t *) (t)))

#define strtoken_dict_create()  dict_set_tostring_data( \
                                  dict_set_tostring_key( \
                                    dict_set_free_data( \
                                      dict_set_free_key( \
                                        dict_set_hash( \
                                          dict_create((cmp_t) strcmp),\
                                          (hash_t) strhash), \
                                        (free_t) free), \
                                      (free_t) data_free), \
                                    (tostring_t) chars), \
                                  (tostring_t) data_tostring)

#define tokenset_create()     set_set_tostring( \
                                set_set_hash( \
                                  set_set_free( \
                                    set_create((cmp_t) token_cmp), \
                                    (free_t) data_free), \
                                  token_hash), \
                                data_tostring)

#endif /* TOKEN_H */

