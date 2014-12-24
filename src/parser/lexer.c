/*
 * lexer.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <dict.h>
#include <lexer.h>

#ifdef LEXER_DEBUG
#undef LEXER_DEBUG
//#ifndef LEXER_DEBUG
//#define LEXER_DEBUG
#endif

static void       _dequotify(str_t *);

static int        _lexer_get_char(lexer_t *);
static void       _lexer_push_back(lexer_t *, int);
static void       _lexer_push_all_back(lexer_t *);
static lexer_t *  _lexer_keyword_match_reducer(token_t *, lexer_t *);
static int        _lexer_keyword_match(lexer_t *);
static token_t *  _lexer_match_token(lexer_t *, int);

typedef struct _lexer_state_str {
  lexer_state_t  state;
  char          *name;
} lexer_state_str_t;

static lexer_state_str_t lexer_state_names[] = {
    { LexerStateFresh,           "LexerStateFresh" },
    { LexerStateInit,            "LexerStateInit" },
    { LexerStateSuccess,         "LexerStateSuccess" },
    { LexerStateWhitespace,      "LexerStateWhitespace" },
    { LexerStateNewLine,         "LexerStateNewLine" },
    { LexerStateIdentifier,      "LexerStateIdentifier" },
    { LexerStateKeyword,         "LexerStateKeyword" },
    { LexerStateZero,            "LexerStateZero" },
    { LexerStateNumber,          "LexerStateNumber" },
    { LexerStateDecimalInteger,  "LexerStateDecimalInteger" },
    { LexerStateHexInteger,      "LexerStateHexInteger" },
    { LexerStateFloat,           "LexerStateFloat" },
    { LexerStateSciFloat,        "LexerStateSciFloat" },
    { LexerStateQuotedStr,       "LexerStateQuotedStr" },
    { LexerStateQuotedStrEscape, "LexerStateQuotedStrEscape" },
    { LexerStateHashPling,       "LexerStateHashPling" },
    { LexerStateSlash,           "LexerStateSlash" },
    { LexerStateBlockComment,    "LexerStateBlockComment" },
    { LexerStateLineComment,     "LexerStateLineComment" },
    { LexerStateStar,            "LexerStateStar" },
    { LexerStateDone,            "LexerStateDone" }
};

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
 * static utility functions
 */

void _dequotify(str_t *str) {
  int len, ix;

  len = str_len(str);
  if ((len >= 2) && (str_at(str, 0) == str_at(str, len - 1))) {
    switch (len) {
      case 2:
        str_erase(str);
        break;
      default:
        str_lchop(str, 1);
        str_chop(str, 1);
        break;
    }
  }
}

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

char * lexer_state_name(lexer_state_t state) {
  int ix;
  static char buf[20];

  for (ix = 0; ix < sizeof(lexer_state_names) / sizeof(lexer_state_str_t); ix++) {
    if (lexer_state_names[ix].state == state) {
      return lexer_state_names[ix].name;
    }
  }
  sprintf(buf, "[Unknown state %d]", state);
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

/*
 * lexer_t - static methods
 */

int _lexer_get_char(lexer_t *lexer) {
  int   ch;
  int   ret;

  if (lexer -> pushed_back) {
    ch = str_readchar(lexer -> pushed_back);
    if (ch > 0) {
      return ch;
    } else {
      str_free(lexer -> pushed_back);
      lexer -> pushed_back = NULL;
    }
  }

  if (!lexer -> buffer) {
    lexer -> buffer = str_create(LEXER_BUFSIZE);
    if (!lexer -> buffer) {
      return 0;
    }
    ret = str_readinto(lexer -> buffer, lexer -> reader);
#ifdef LEXER_DEBUG
    debug("_lexer_get_char: Created buffer and did initial read: %d", ret);
#endif
    if (ret <= 0) {
      return 0;
    }
  }
  ch = str_readchar(lexer -> buffer);
#ifdef LEXER_DEBUG
    debug("_lexer_get_char: Read character: '%c'", (ch > 0) ? ch : '$');
#endif
  if (ch <= 0) {
    ret = str_readinto(lexer -> buffer, lexer -> reader);
#ifdef LEXER_DEBUG
    debug("_lexer_get_char: Read new chunk into buffer: %d", ret);
#endif
    if (ret <= 0) {
      return 0;
    } else {
      ch = str_readchar(lexer -> buffer);
#ifdef LEXER_DEBUG
      debug("_lexer_get_char: Read character from new chunk: '%c'", (ch > 0) ? ch : '$');
#endif
    }
  }
  return ch;
}

void _lexer_push_back(lexer_t *lexer, int ch) {
  if (ch <= 0) {
#ifdef LEXER_DEBUG
    debug("_lexer_push_back: Not pushing back EOF character");
#endif
    return;
  }
#ifdef LEXER_DEBUG
  debug("_lexer_push_back: in current_token: '%s' current_token_len: %d",
        str_chars(lexer -> token), str_len(lexer -> token));
#endif
  str_chop(lexer -> token, 1);
  str_pushback((lexer -> pushed_back) ? lexer -> pushed_back : lexer -> buffer, 1);
#ifdef LEXER_DEBUG
  debug("_lexer_push_back: out current_token: '%s' current_token_len: %d",
        str_chars(lexer -> token), str_len(lexer -> token));
#endif
}

void _lexer_push_all_back(lexer_t *lexer) {
  if (lexer -> pushed_back) {
    str_append(lexer -> pushed_back, lexer -> token);
  } else {
    lexer -> pushed_back = str_copy(lexer -> token);
  }
  str_erase(lexer -> token);
}

lexer_t * _lexer_keyword_match_reducer(token_t *token, lexer_t *lexer) {
  char *kw;

  kw = token_token(token);
  if ((str_len(lexer -> token) <= strlen(kw)) &&
      !str_ncmp_chars(lexer -> token, kw, str_len(lexer -> token))) {
#ifdef LEXER_DEBUG
    debug("_lexer_keyword_match_reducer: found potential match '%s for keyword '%s'", str_chars(lexer -> token), kw);
#endif
    list_append(lexer -> matches, token);
  }
  return lexer;
}

int _lexer_keyword_match(lexer_t *lexer) {
  int      ret;
  token_t *match;

  ret = TokenCodeNone;
  if (!str_len(lexer -> token)) return ret;

  lexer -> matches = list_create();
  if (lexer -> matches) {
    list_reduce(lexer -> keywords,
                (reduce_t) _lexer_keyword_match_reducer, lexer);
    if (list_size(lexer -> matches) == 1) {
      match = (token_t *) list_head(lexer -> matches);
      if (strlen(token_token(match)) == str_len(lexer -> token)) {
        ret = token_code(match);
      }
      lexer -> state = LexerStateKeyword;
    } else if (list_size(lexer -> matches) > 1) {
      lexer -> state = LexerStateKeyword;
    } else {
      lexer -> state = LexerStateInit;
    }
    list_set_free(lexer -> matches, NULL);
    list_free(lexer -> matches);
    lexer -> matches = NULL;
  }
  return ret;
}

void _lexer_update_location(lexer_t *lexer, int ch) {
  if (strchr("\r\n", ch)) {
    if (!strchr("\r\n", lexer -> prev_char) || (ch != lexer -> prev_char)) {
      lexer -> line++;
      lexer -> column = 0;
    }
  } else {
    lexer -> column++;
  }
}

token_t * _lexer_match_token(lexer_t *lexer, int ch) {
  token_code_t    code;
  token_t        *ret;
  int             ignore_nl;
  list_t         *matches;
  listiterator_t *iter;
  
  ignore_nl = lexer_get_option(lexer, LexerOptionIgnoreNewLines);
#ifdef LEXER_DEBUG
  debug("_lexer_match_token in - state: %s tok: %s ch: '%c'",
        lexer_state_name(lexer -> state), str_chars(lexer -> token), (ch > 0) ? ch : '$');
#endif
  _lexer_update_location(lexer, ch);
  str_append_char(lexer -> token, ch);
  code = TokenCodeNone;
  ret = NULL;
  switch (lexer -> state) {
    case LexerStateInit:
      if (!lexer -> no_kw_match) {
        code = _lexer_keyword_match(lexer);
      }
      if (!code && (lexer -> state == LexerStateInit)) {
        if (!ignore_nl && ((ch == '\r') || (ch == '\n'))) {
          lexer -> state = LexerStateNewLine;
        } else if (isspace(ch)) {
          lexer -> state = LexerStateWhitespace;
        } else if (isalpha(ch) || (ch == '_')) {
          lexer -> state = LexerStateIdentifier;
        } else if (ch == '0') {
          lexer -> state = LexerStateZero;
        } else if (isdigit(ch)) {
          lexer -> state = LexerStateNumber;
        } else if ((ch == '\'') || (ch == '"') || (ch == '`')) {
          lexer -> state = LexerStateQuotedStr;
          lexer -> quote = ch;
        } else if (ch == '/') {
          lexer -> state = LexerStateSlash;
        } else if ((ch == '#') && ((lexer -> line == 1) &&
                   (lexer -> column == 1)) &&
                   lexer_get_option(lexer, LexerOptionHashPling)) {
          lexer -> state = LexerStateHashPling;
        } else if (ch > 0) {
          code = ch;
        }
      }
      break;
    case LexerStateNewLine:
      if ((ch <= 0) || ((ch != '\n') && (ch != '\r'))) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeNewLine;
      }
      break;
    case LexerStateWhitespace:
      if (!isspace(ch) ||
          (!ignore_nl && ((ch == '\r') || (ch == '\n')))) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeWhitespace;
      }
      break;
    case LexerStateIdentifier:
      if (!isalnum(ch) && (ch != '_')) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeIdentifier;
      }
      break;
    case LexerStateZero:
      if (isdigit(ch)) {
	/*
	 * We don't want octal numbers. Therefore we strip
         * leading zeroes. Erasing the token leads to funny 
	 * business like '00x23' being recognized as Hex. We
	 * may fix that later, or maybe not.
	 */
        str_erase(lexer -> token);
        lexer -> state = LexerStateNumber;
      } else if (ch == '.') {
        lexer -> state = LexerStateFloat;
      } else if ((ch == 'x') || (ch == 'X')) {
	/*
	 * Heaxdecimals are returned including the leading
         * 0x. This allows us to send both base-10 and hex ints
         * to strtol and friends.
	 */
        lexer -> state = LexerStateHexInteger;
      } else {
        _lexer_push_back(lexer, ch);
        code = TokenCodeInteger;
      }
      break;
    case LexerStateNumber:
      if (ch == '.') {
        lexer -> state = LexerStateFloat;
      } else if ((ch == 'e') || (ch == 'E')) {
	str_chop(lexer -> token, 1);
        str_append_char(lexer -> token, 'e');
        lexer -> state = LexerStateSciFloat;
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeInteger;
      }
      break;
    case LexerStateFloat:
      if ((ch == 'e') || (ch == 'E')) {
	str_chop(lexer -> token, 1);
        str_append_char(lexer -> token, 'e');
        lexer -> state = LexerStateSciFloat;
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeFloat;
      }
      break;
    case LexerStateSciFloat:
      if (((ch == '+') || (ch == '-')) && (str_at(lexer -> token, -2) == 'e')) {
        // Nothing
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeFloat;
      }
      break;
    case LexerStateHexInteger:
      if (!isxdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeHexNumber;
      }
      break;
    case LexerStateQuotedStr:
      if (ch == lexer -> quote) {
        code = (token_code_t) lexer -> quote;
        _dequotify(lexer -> token);
      } else if (ch == '\\') {
        lexer -> state = LexerStateQuotedStrEscape;
        str_chop(lexer -> token, 1);
      }
      break;
    case LexerStateQuotedStrEscape:
      if (ch == 'n') {
        str_append_char(lexer -> token, '\n');
      } else if (ch == 'r') {
        str_append_char(lexer -> token, '\r');
      } else if (ch == 't') {
        str_append_char(lexer -> token, '\t');
      }
      lexer -> state = LexerStateQuotedStr;
      break;
    case LexerStateHashPling:
      if (ch == '!') {
        str_erase(lexer -> token);
        lexer -> state = LexerStateLineComment;
      } else {
        _lexer_push_back(lexer, ch);
        code = TokenCodeHash;
      }
      break;
    case LexerStateSlash:
      if (ch == '*') {
        str_erase(lexer -> token);
        lexer -> state = LexerStateBlockComment;
      } else if (ch == '/') {
        str_erase(lexer -> token);
        lexer -> state = LexerStateLineComment;
      } else {
        _lexer_push_back(lexer, ch);
        code = TokenCodeSlash;
      }
      break;
    case LexerStateBlockComment:
      str_erase(lexer -> token);
      if (ch == '*') {
        lexer -> state = LexerStateStar;
      }
      break;
    case LexerStateStar:
      str_erase(lexer -> token);
      lexer -> state = (ch == '/') ? LexerStateInit : LexerStateBlockComment;
      break;
    case LexerStateLineComment:
      str_erase(lexer -> token);
      if ((ch == '\r') || (ch == '\n') || (ch <= 0)) {
        lexer -> state = LexerStateInit;
      }
      break;
    case LexerStateKeyword:
      code = _lexer_keyword_match(lexer);
      if (!code && (lexer -> state == LexerStateInit)) {
        _lexer_push_all_back(lexer);
        lexer -> no_kw_match = 1;
      }
      break;
  }
  if (code != TokenCodeNone) {
    ret = token_create(code, str_chars(lexer -> token));
    lexer -> state = LexerStateSuccess;
  }
  if (ch <= 0) {
    if (!ret) {
      switch (lexer -> state) {
        case LexerStateBlockComment:
        case LexerStateSlash:
          ret = token_create(TokenCodeError, "Unterminated block comment");
          break;
        case LexerStateQuotedStr:
        case LexerStateQuotedStrEscape:
          ret = token_create(TokenCodeError, "Unterminated string");
          break;
      }
      lexer -> state = LexerStateDone;
    }
  }
  if (ret) {
    ret -> line = lexer -> line;
    ret -> column = lexer -> column;
  }
#ifdef LEXER_DEBUG
    if (ret) {
      debug("_lexer_match_token out - state: %s token: %s",
            lexer_state_name(lexer -> state), token_code_name(ret -> code));
    } else {
      debug("_lexer_match_token out - state: %s", lexer_state_name(lexer -> state));
    }
#endif
  lexer -> prev_char = ch;
  return ret;
}

/*
 * lexer_t - public interface
 */

lexer_t * _lexer_create(reader_t *reader) {
  lexer_t *ret;
  int      ix;
  
  ret = NEW(lexer_t);
  if (ret) {
    ret -> reader = reader;
    ret -> pushed_back = NULL;
    ret -> buffer = NULL;
    ret -> state = LexerStateFresh;
    ret -> last_match = NULL;
    ret -> line = 1;
    ret -> column = 0;
    ret -> prev_char = 0;
    ret -> options = array_create((int) LexerOptionLAST);
    if (!ret -> options) {
      free(ret);
      ret = NULL;
    } else {
      for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
        lexer_set_option(ret, ix, 0L);
      }
      ret -> keywords = list_create();
      if (!ret -> keywords) {
        array_free(ret -> options);
        free(ret);
        ret = NULL;
      } else {
        list_set_free(ret -> keywords, (free_t) token_free);
        ret -> token = str_create(LEXER_INIT_TOKEN_SZ);
        if (!ret -> token) {
          list_free(ret -> keywords);
          array_free(ret -> options);
          free(ret);
          ret = NULL;
        }
      }
    }
  }
  return ret;
}

lexer_t * lexer_set_option(lexer_t *lexer, lexer_option_t option, long value) {
  array_set(lexer -> options, (int) option, (void *) value);
  return lexer;
}

long lexer_get_option(lexer_t *lexer, lexer_option_t option) {
  return (long) array_get(lexer -> options, (int) option);
}

lexer_t * lexer_add_keyword(lexer_t *lexer, int code, char *token) {
  list_append(lexer -> keywords, token_create(code, token));
  return lexer;
}

void lexer_free(lexer_t *lexer) {
  if (lexer) {
    token_free(lexer -> last_match);
    array_free(lexer -> options);
    list_free(lexer -> keywords);
    str_free(lexer -> token);
    str_free(lexer -> pushed_back);
    str_free(lexer -> buffer);
    free(lexer);
  }
}

void _lexer_tokenize(lexer_t *lexer, reduce_t parser, void *data) {
  token_t *token;
  int      code;

  do {
    lexer -> last_match = lexer_next_token(lexer);
    code = token_code(lexer -> last_match);
    data = parser(lexer -> last_match, data);
    token_free(lexer -> last_match);
    lexer -> last_match = NULL;
  } while (data && (code != TokenCodeEnd));
}

token_t * lexer_next_token(lexer_t *lexer) {
  int ch;
  token_t *ret;
  int ignore_ws;
  int first;
  
  first = lexer -> state == LexerStateFresh;
  ret = NULL;
  ignore_ws = lexer_get_option(lexer, LexerOptionIgnoreWhitespace);

  do {
    lexer -> state = LexerStateInit;
    str_erase(lexer -> token);
    if (lexer -> pushed_back) {
      str_free(lexer -> pushed_back);
      lexer -> pushed_back = NULL;
    }
    lexer -> no_kw_match = 0;

    do {
      if (ret) {
        token_free(ret);
        ret = NULL;
      }
      ch = _lexer_get_char(lexer);
      ret = _lexer_match_token(lexer, ch);
    } while ((lexer -> state != LexerStateDone) && (lexer -> state != LexerStateSuccess));
#ifdef LEXER_DEBUG
    if (ret) {
      debug("lexer_next_token: matched token: %s [%s]", token_code_name(ret -> code), ret -> token);
    }
#endif
  } while (ignore_ws && ret && token_iswhitespace(ret));

  if (!ret && (lexer -> state == LexerStateDone)) {
    ret = token_create(TokenCodeEnd, "$$");
    ret -> line = lexer -> line;
    ret -> column = lexer -> column;
  }

#ifdef LEXER_DEBUG
  if (ret) {
    debug("lexer_next_token out: token: %s [%s], state %s",
          token_code_name(ret -> code), ret -> token, lexer_state_name(lexer -> state));
  }
#endif
  return ret;
}
