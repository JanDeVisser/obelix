/*
 * token.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <assert.h>
#include <stdio.h>

#include <core.h>
#include <token.h>

typedef struct _token_code_str {
  token_code_t  code;
  char         *name;
} token_code_str_t;

static token_code_str_t token_code_names[] = {
  { TokenCodeError,          "TokenCodeError" },
  { TokenCodeNone,           "TokenCodeNone" },
  { TokenCodeEmpty,          "TokenCodeEmpty" },
  { TokenCodeWhitespace,     "TokenCodeWhitespace" },
  { TokenCodeNewLine,        "TokenCodeNewLine" },
  { TokenCodeIdentifier,     "TokenCodeIdentifier" },
  { TokenCodeInteger,        "TokenCodeInteger" },
  { TokenCodeHexNumber,      "TokenCodeHexNumber" },
  { TokenCodeFloat,          "TokenCodeFloat" },
  { TokenCodeSQuotedStr,     "TokenCodeSQuotedStr" },
  { TokenCodeDQuotedStr,     "TokenCodeDQuotedStr" },
  { TokenCodeBQuotedStr,     "TokenCodeBQuotedStr" },
  { TokenCodePlus,           "TokenCodePlus" },
  { TokenCodeMinus,          "TokenCodeMinus" },
  { TokenCodeDot,            "TokenCodeDot" },
  { TokenCodeComma,          "TokenCodeComma" },
  { TokenCodeQMark,          "TokenCodeQMark" },
  { TokenCodeExclPoint,      "TokenCodeExclPoint" },
  { TokenCodeOpenPar,        "TokenCodeOpenPar" },
  { TokenCodeClosePar,       "TokenCodeClosePar" },
  { TokenCodeOpenBrace,      "TokenCodeOpenBrace" },
  { TokenCodeCloseBrace,     "TokenCodeCloseBrace" },
  { TokenCodeOpenBracket,    "TokenCodeOpenBracket" },
  { TokenCodeCloseBracket,   "TokenCodeCloseBracket" },
  { TokenCodeLAngle,         "TokenCodeLAngle" },
  { TokenCodeRangle,         "TokenCodeRangle" },
  { TokenCodeAsterisk,       "TokenCodeAsterisk" },
  { TokenCodeSlash,          "TokenCodeSlash" },
  { TokenCodeBackslash,      "TokenCodeBackslash" },
  { TokenCodeColon,          "TokenCodeColon" },
  { TokenCodeSemiColon,      "TokenCodeSemiColon" },
  { TokenCodeEquals,         "TokenCodeEquals" },
  { TokenCodePipe,           "TokenCodePipe" },
  { TokenCodeAt,             "TokenCodeAt" },
  { TokenCodeHash,           "TokenCodeHash" },
  { TokenCodeDollar,         "TokenCodeDollar" },
  { TokenCodePercent,        "TokenCodePercent" },
  { TokenCodeHat,            "TokenCodeHat" },
  { TokenCodeAmpersand,      "TokenCodeAmpersand" },
  { TokenCodeTilde,          "TokenCodeTilde" },
  { TokenCodeEnd,            "TokenCodeEnd" }
};

/*
 * public utility functions
 */

char * token_code_name(token_code_t code) {
  int ix;
  static char buf[100];

  for (ix = 0; ix < sizeof(token_code_names) / sizeof(token_code_str_t); ix++) {
    if (token_code_names[ix].code == code) {
      return token_code_names[ix].name;
    }
  }
  snprintf(buf, 100, "[Custom code %d]", code);
  return buf;
}

/*
 * token_t - public interface
 */

token_t *token_create(int code, char *token) {
  token_t *ret;

  ret = NEW(token_t);
  if (ret) {
    ret -> code = code;
    ret -> token = strdup(token);
    if (!ret -> token) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

token_t * token_copy(token_t *token) {
  token_t *ret;

  ret = token_create(token -> code, token -> token);
  ret -> line = token -> line;
  ret -> column = token -> column;
  return ret;
}

void token_free(token_t *token) {
  if (token) {
    free(token -> token);
    free(token);
  }
}

unsigned int token_hash(token_t *token) {
  return token -> code;
}

int token_cmp(token_t *token, token_t *other) {
  return token -> code - other -> code;
}

int token_code(token_t *token) {
  return token -> code;
}

char * token_token(token_t *token) {
  return token -> token;
}

int token_iswhitespace(token_t *token) {
  return (token -> code == TokenCodeWhitespace) ||
         (token -> code == TokenCodeNewLine);
}

void token_dump(token_t *token) {
  fprintf(stderr, " '%s' (%d)", token_token(token), token_code(token));
}

data_t * token_todata(token_t *token) {
  data_t   *data;
  char     *str;
  int       type;

  data = NULL;
  str = token_token(token);
  type = -1;
  switch (token_code(token)) {
    case TokenCodeIdentifier:
    case TokenCodeDQuotedStr:
    case TokenCodeSQuotedStr:
    case TokenCodeBQuotedStr:
      type = String;
      break;
    case TokenCodeHexNumber:
    case TokenCodeInteger:
      type = Int;
      break;
    case TokenCodeFloat:
      type = Float;
      break;
    default:
      data = data_create(Int, token_code(token));
      break;
  }
  if (!data) {
    data = data_parse(type, str);
  }
  assert(data);
#ifdef LEXER_DEBUG
  debug("token_todata: converted token [%s] to data value [%s]", token_tostring(token), data_tostring(data));
#endif
  return data;
}

char * token_tostring(token_t *token) {
  static char buf[10][128];
  static int  ix = 0;
  char        *ptr;

  ptr = buf[ix++];
  snprintf(ptr, 128, "[%s]",
    (token -> code < 200) ? token_code_name(token -> code) : token -> token);
  if (token -> code < 200) {
    snprintf(ptr + strlen(ptr), 128 - strlen(ptr), " '%s'", token -> token);
  }
  ix %= 10;
  return ptr;
}


