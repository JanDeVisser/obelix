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
#include <str.h>
#include <token.h>

typedef struct _token_code_str {
  token_code_t  code;
  char         *name;
} token_code_str_t;

static inline void _token_init(void);
static token_t *   _token_new(token_t *, va_list);
static void        _token_free(token_t *);
static char *      _token_allocstring(token_t *);
static data_t *    _token_resolve(token_t *, char *);
static data_t *    _token_iswhitespace(token_t *, char *, array_t *, dict_t *);

static code_label_t token_code_names[] = {
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
  { TokenCodeLastToken,      "TokenCodeLastToken" },
  { TokenCodeEnd,            "TokenCodeEnd" },
  { TokenCodeExhausted,      "TokenCodeExhausted" },
  { -1,                      NULL }
};

       int     Token = -1;
static dict_t *custom_codes = NULL;

/* ------------------------------------------------------------------------ */

static vtable_t _vtable_token[] = {
  { .id = FunctionNew,         .fnc = (void_t) _token_new },
  { .id = FunctionFree,        .fnc = (void_t) _token_free },
  { .id = FunctionParse,       .fnc = (void_t) token_parse },
  { .id = FunctionAllocString, .fnc = (void_t) _token_allocstring },
  { .id = FunctionResolve,     .fnc = (void_t) _token_resolve },
  { .id = FunctionHash,        .fnc = (void_t) token_hash },
  { .id = FunctionCmp,         .fnc = (void_t) token_cmp },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_token[] = {
  { .type = -1,     .name = "iswhitespace", .method = (method_t) _token_iswhitespace, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,           .method = NULL,                           .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static typedescr_t _typedescr_token = {
  .type = -1,
  .type_name = "token",
  .vtable = _vtable_token
};

/* ------------------------------------------------------------------------ */

void _token_init(void) {
  if (Token < 0) {
    Token = typedescr_register_type(&_typedescr_token, _methoddescr_token);
    typedescr_set_size(Token, token_t);
  }
}

/*
 * public utility functions
 */

char * token_code_name(token_code_t code) {
  int   ix;
  char *ret = label_for_code(token_code_names, code);

  if (!ret) {
    if (!custom_codes) {
      custom_codes = intstr_dict_create();
    }
    ret = (char *) dict_get_int(custom_codes, code);
    if (!ret) {
      asprintf(&ret, "[Custom code %d]", code);
      dict_put_int(custom_codes, code, ret);
    }
  }
  return ret;
}

/* ------------------------------------------------------------------------ */

token_t * _token_new(token_t *token, va_list args) {
  return token_assign(token, va_arg(args, unsigned int), va_arg(args, char *));
}

void _token_free(token_t *token) {
  if (token) {
    free(token -> token);
  }
}

char * _token_allocstring(token_t *token) {
  char *buf;

  if (token -> code < 200) {
    asprintf(&buf, "[%s] '%s'",
	     token_code_name(token -> code), token_token(token));
  } else {
    asprintf(&buf, "[%s]", token_token(token));
  }
  return buf;
}

data_t * _token_resolve(token_t *token, char *name) {
  if (!strcmp(name, "code")) {
    return int_to_data(token -> code);
  } else if (!strcmp(name, "codename")) {
    return str_to_data(token_code_name(token -> code));
  } else if (!strcmp(name, "tokenstring")) {
    return str_to_data(token_token(token));
  } else if (!strcmp(name, "token")) {
    return token_todata(token);
  } else if (!strcmp(name, "line")) {
    return int_to_data(token -> line);
  } else if (!strcmp(name, "column")) {
    return int_to_data(token -> column);
  }
  return NULL;
}

data_t * _token_iswhitespace(token_t *self, char *n, array_t *args, dict_t *kwargs) {
  return int_to_data(token_iswhitespace(self));
}

/* ------------------------------------------------------------------------ */

/*
 * token_t - public interface
 */

token_t * token_create(unsigned int code, char *token) {
  _token_init();
  return (token_t *) data_create(Token, code, token);
}

token_t * token_parse(char *token) {
  token_t *ret = NULL;
  char    *dup = NULL;
  char    *ptr;
  long     code;

  if (!token) {
    goto done;
  }
  dup = strdup(token);
  ptr = strrchr(dup, ':');
  if (!ptr) {
    goto done;
  }
  *ptr++ = 0;
  if (strtoint(dup, &code)) {
    goto done;
  }
  ret = token_create(code, ptr);

done:
  if (dup) {
    free(dup);
  }
  return ret;
}

unsigned int token_hash(token_t *token) {
  return token -> code;
}

int token_cmp(token_t *token, token_t *other) {
  return token -> code - other -> code;
}

unsigned int token_code(token_t *token) {
  return token -> code;
}

char * token_token(token_t *token) {
  return (token -> token && *(token -> token)) ? token -> token : NULL;
}

token_t * token_assign(token_t *token, unsigned int code, char *t) {
  int len = (t) ? strlen(t) : 0;

  token -> code = code;
  if (len) {
    if (len < token -> size) {
      strcpy(token -> token, t);
    } else {
      free(token -> token);
      token -> token = strdup(t);
      token -> size = strlen(t) + 1;
    }
  } else if (token -> size) {
    *(token -> token) = 0;
  }
  return token;
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
  if (token) {
    str = token_token(token);
    type = -1;
    switch (token_code(token)) {
      case TokenCodeRawString:
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
        data = int_to_data(token_code(token));
        break;
    }
    if (!data) {
      data = data_parse(type, str);
    }
    assert(data);
#ifdef LEXER_DEBUG
    debug("token_todata: converted token [%s] to data value [%s]", token_tostring(token), data_tostring(data));
#endif
  } else {
#ifdef LEXER_DEBUG
    debug("token_todata: Not converting NULL token. Returning NULL");
#endif
  }
  return data;
}
