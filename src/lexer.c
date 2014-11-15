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

static void _dequotify(char *);

static int        _lexer_get_char(lexer_t *);
static void       _lexer_push_back(lexer_t *);
static void       _lexer_push_all_back(lexer_t *);
static lexer_t *  _lexer_keyword_match_reducer(token_t *, lexer_t *);
static int        _lexer_keyword_match(lexer_t *);
static token_t *  _lexer_match_token(lexer_t *, int);

/*
 * static utility functions
 */

void _dequotify(char *str) {
  int len, ix;

  len = strlen(str);
  if ((len >= 2) && (str[0] == str[len - 1])) {
    switch (len) {
      case 2:
        str[0] = str[1] = 0;
        break;
      default:
        for (ix = 1; ix < (len - 1); ix++) {
          str[ix - 1] = str[ix];
        }
        str[len-2] = str[len-1] = 0;
        break;
    }
  }
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
  return token_create(token -> code, token -> token);
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

/*
 * lexer_t - static methods
 */

int _lexer_get_char(lexer_t *lexer) {
  int ch;
  char *newtok;

  if (lexer -> old_token) {
    if (lexer -> old_token[lexer -> old_token_pos]) {
      return lexer -> old_token[lexer -> old_token_pos++];
    } else {
      free(lexer -> old_token);
      lexer -> old_token = NULL;
      lexer -> old_token_pos = 0;
    }
  }
  if ((lexer -> buflen <= 0) || (lexer -> bufpos > lexer -> buflen)) {
    memset(lexer -> buf, 0, LEXER_BUFSIZE);
    lexer -> buflen = reader_read(lexer -> reader, lexer -> buf, LEXER_BUFSIZE);
    if (0) {
      debug("lexer -> buflen: %d", lexer -> buflen);
    }
    if (lexer -> buflen <= 0) {
      return lexer -> buflen;
    }
    lexer -> bufpos = 0;
  }
  if (strlen(lexer -> current_token) >= (lexer -> token_size - 2)) {
    newtok = malloc(2 * lexer -> token_size);
    if (!newtok) {
      return -1;
    }
    strcpy(newtok, lexer -> current_token);
    if (lexer -> current_token != lexer -> short_token) {
      free(lexer -> current_token);
    }
    lexer -> current_token = newtok;
    lexer -> token_size *= 2;
  }
  return (int) lexer -> buf[lexer -> bufpos++];
}

void _lexer_push_back(lexer_t *lexer) {
  lexer -> current_token[--(lexer -> current_token_len)] = 0;
  lexer -> bufpos--;
}

void _lexer_push_all_back(lexer_t *lexer) {
  lexer -> old_token = strdup(lexer -> current_token);
  memset(lexer -> current_token, 0, lexer -> token_size);
  lexer -> current_token_len = 0;
}

lexer_t * _lexer_keyword_match_reducer(token_t *token, lexer_t *lexer) {
  char *kw;

  kw = token_token(token);
  if ((lexer -> current_token_len <= strlen(kw)) &&
      !strncmp(lexer -> current_token, kw, lexer -> current_token_len)) {
    list_append(lexer -> matches, token);
  }
  return lexer;
}

int _lexer_keyword_match(lexer_t *lexer) {
  int      ret;
  token_t *match;

  ret = TokenCodeNone;
  if (!lexer -> current_token[0]) return ret;

  lexer -> matches = list_create();
  if (lexer -> matches) {
    list_reduce(lexer -> keywords,
                (reduce_t) _lexer_keyword_match_reducer, lexer);
    if (list_size(lexer -> matches) == 1) {
      match = (token_t *) list_head(lexer -> matches);
      if (strlen(token_token(match)) == lexer -> current_token_len) {
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

token_t * _lexer_match_token(lexer_t *lexer, int ch) {
  token_code_t    code;
  token_t        *ret;
  char           *tok;
  int             ignore_nl;
  list_t *        matches;
  listiterator_t *iter;
  
  tok = lexer -> current_token;
  ignore_nl = lexer_get_option(lexer, LexerOptionIgnoreNewLines);
  if (0) {
    debug("matcher in - state: %d tok: %s ch: %c", lexer -> state, tok, ch);
  }
  if (ch > 0) {
    tok[lexer -> current_token_len++] = ch;
  }
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
        } else if (ch > 0) {
          code = ch;
        }
      }
      break;
    case LexerStateNewLine:
      if ((ch != '\n') && (ch != '\r')) {
        _lexer_push_back(lexer);
        code = TokenCodeNewLine;
      }
      break;
    case LexerStateWhitespace:
      if (!isspace(ch) ||
          (!ignore_nl && ((ch == '\r') || (ch == '\n')))) {
        _lexer_push_back(lexer);
        code = TokenCodeWhitespace;
      }
      break;
    case LexerStateIdentifier:
      if (!isalnum(ch) && (ch != '_')) {
        _lexer_push_back(lexer);
        code = TokenCodeIdentifier;
      }
      break;
    case LexerStateZero:
      if (isdigit(ch)) {
        lexer -> state = LexerStateNumber;
      } else if (ch == '.') {
        lexer -> state = LexerStateFloat;
      } else if ((ch == 'x') || (ch == 'X')) {
        memset(tok, 0, lexer -> token_size);
        lexer -> state = LexerStateHexInteger;
      } else {
        _lexer_push_back(lexer);
        code = TokenCodeInteger;
      }
      break;
    case LexerStateNumber:
      if (ch == '.') {
        lexer -> state = LexerStateFloat;
      } else if ((ch == 'e') || (ch == 'E')) {
        tok[strlen(tok) - 1] = 'e';
        lexer -> state = LexerStateSciFloat;
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer);
        code = TokenCodeInteger;
      }
      break;
    case LexerStateFloat:
      if ((ch == 'e') || (ch == 'E')) {
        tok[strlen(tok) - 1] = 'e';
        lexer -> state = LexerStateSciFloat;
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer);
        code = TokenCodeFloat;
      }
      break;
    case LexerStateSciFloat:
      if (((ch == '+') || (ch == '-')) && (tok[strlen(tok) - 2] == 'e')) {
        // Nothing
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer);
        code = TokenCodeFloat;
      }
      break;
    case LexerStateHexInteger:
      if (!isxdigit(ch)) {
        _lexer_push_back(lexer);
        code = TokenCodeHexNumber;
      }
      break;
    case LexerStateQuotedStr:
      if (ch == lexer -> quote) {
        code = (token_code_t) lexer -> quote;
        _dequotify(tok);
      } else if (ch == '\\') {
        lexer -> state = LexerStateQuotedStrEscape;
        tok[strlen(tok) - 1] = 0;
      }
      break;
    case LexerStateQuotedStrEscape:
      if (ch == 'n') {
        tok[strlen(tok) - 1] = '\n';
      } else if (ch == 'r') {
        tok[strlen(tok) - 1] = '\r';
      } else if (ch == 't') {
        tok[strlen(tok) - 1] = '\t';
      }
      lexer -> state = LexerStateQuotedStr;
      break;
    case LexerStateSlash:
      if (ch == '*') {
        memset(tok, 0, lexer -> token_size);
        lexer -> state = LexerStateBlockComment;
      } else if (ch == '/') {
        memset(tok, 0, lexer -> token_size);
        lexer -> state = LexerStateLineComment;
      } else {
        _lexer_push_back(lexer);
        code = TokenCodeSlash;
      }
      break;
    case LexerStateBlockComment:
      tok[0] = 0;
      if (ch == '*') {
        lexer -> state = LexerStateStar;
      }
      break;
    case LexerStateStar:
      tok[0] = 0;
      lexer -> state = (ch == '/') ? LexerStateInit : LexerStateBlockComment;
      break;
    case LexerStateLineComment:
      tok[0] = 0;
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
    ret = token_create(code, tok);
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
  if (0) {
    if (ret) {
      debug("          matcher out - state: %d token: %d/%s", lexer -> state, (ret) ? ret -> code : TokenCodeNone, (ret) ? ret -> token : "");
    } else {
      debug("  matcher out - state: %d", lexer -> state);
    }
  }
  return ret;
}

/*
 * lexer_t - public interface
 */

lexer_t * lexer_create(void *reader) {
  lexer_t *ret;
  int      ix;
  
  ret = NEW(lexer_t);
  if (ret) {
    ret -> reader = reader;
    ret -> buflen = ret -> bufpos = -1;
    ret -> state = LexerStateFresh;
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

lexer_t * lexer_add_keyword(lexer_t *lexer, token_t *token) {
  list_append(lexer -> keywords, token);
  return lexer;
}

void lexer_free(lexer_t *lexer) {
  if (lexer) {
    array_free(lexer -> options);
    list_free(lexer -> keywords);
    free(lexer);
  }
}

void _lexer_tokenize(lexer_t *lexer, reduce_t parser, void *data) {
  token_t *token;

  for (token = lexer_next_token(lexer); token_code(token) != TokenCodeEnd; token = lexer_next_token(lexer)) {
    data = parser(token, data);
    token_free(token);
  }
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
    lexer -> token_size = sizeof(lexer -> short_token);
    lexer -> current_token = lexer -> short_token;
    memset(lexer -> current_token, '\0', lexer -> token_size);
    lexer -> current_token_len = 0;
    lexer -> old_token = NULL;
    lexer -> old_token_pos = 0;
    lexer -> no_kw_match = 0;

    do {
      ch = _lexer_get_char(lexer);
      ret = _lexer_match_token(lexer, ch);
    } while ((lexer -> state != LexerStateDone) && (lexer -> state != LexerStateSuccess));
    if (lexer -> current_token != lexer -> short_token) {
      free(lexer -> current_token);
    }
  } while (ignore_ws && ret && token_iswhitespace(ret));

  if (!ret && (lexer -> state == LexerStateDone)) {
    ret = token_create(TokenCodeEnd, "$$");
  }


  if (0) {
    if (ret) {
      debug("              next token: %d/%s/", ret -> code, ret -> token);
    }
  }
  return ret;
}
