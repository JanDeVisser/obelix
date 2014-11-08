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

#include "core.h"
#include "lexer.h"

static void _dequotify(char *);


/*
 * static utility functions
 */

void _dequotify(char *str) {
  int len, ix;

  len = strlen(str);
  if ((len >= 2) && (str[0] == str[1])) {
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

void token_free(token_t *token) {
  free(token -> token);
  free(token);
}

int token_code(token_t *token) {
  return token -> code;
}

char * token_token(token_t *token) {
  return token -> token;
}

/*
 * lexer_t - static methods
 */

int _lexer_get_char(lexer_t *lexer) {
  int ch;
  char *newtok;

  if ((lexer -> buflen <= 0) || (lexer -> bufpos > lexer -> buflen)) {
    memset(lexer -> buf, 0, LEXER_BUFSIZE);
    lexer -> buflen = read(lexer -> fh, lexer -> buf, LEXER_BUFSIZE);
    debug("lexer -> buflen: %d", lexer -> buflen);
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

void _lexer_push_back(lexer_t *lexer, char *tok) {
  tok[strlen(tok) - 1] = 0;
  lexer -> bufpos--;
}

token_t * _lexer_match_token(lexer_t *lexer, int ch) {
  int      code;
  token_t *ret;
  char    *tok;
  
  tok = lexer -> current_token;
  tok[strlen(tok)] = ch;
  code = CODE_TOKEN_NOTOKEN;
  ret = NULL;
  switch (lexer -> state) {
    case LexerTokenInit:
      if (isspace(ch)) {
        lexer -> state = LexerTokenWhitespace;
      } else if (isalpha(ch) || (ch == '_')) {
        lexer -> state = LexerTokenIdentifier;
      } else if (isdigit(ch)) {
        lexer -> state = LexerTokenInteger;
      } else if ((ch == '\'') || (ch == '"')) {
        lexer -> state = LexerTokenQuotedStr;
        lexer -> quote = ch;
      } else if (ch == '/') {
        lexer -> state = LexerTokenSlash;
      } else if (!ch) {
        /*
         * This function is called with ch = 0 to clear up any pending stuff.
         * Therefore we return a NULL token.
\         */
        return NULL;
      } else {
        code = ch;
      }
      break;
    case LexerTokenWhitespace:
      if (!isspace(ch)) {
        _lexer_push_back(lexer, tok);
        code = CODE_TOKEN_WHITESPACE;
      }
      break;
    case LexerTokenIdentifier:
      if (!isalnum(ch) && (ch != '_')) {
        _lexer_push_back(lexer, tok);
        code = CODE_TOKEN_IDENTIFIER;
      }
      break;
    case LexerTokenInteger:
      if (!isdigit(ch)) {
        _lexer_push_back(lexer, tok);
        code = CODE_TOKEN_INTEGER;
      }
      break;
    case LexerTokenQuotedStr:
      if (ch == lexer -> quote) {
        code = (lexer -> quote == '"')
            ? CODE_TOKEN_DQUOTED_STR
            : CODE_TOKEN_SQUOTED_STR;
        _dequotify(tok);
      } else if (ch == '\\') {
        lexer -> state = LexerTokenQuotedStrEscape;
        tok[strlen(tok) - 1] = 0;
      }
      break;
    case LexerTokenQuotedStrEscape:
      lexer -> state = LexerTokenQuotedStr;
      break;
    case LexerTokenSlash:
      if (ch == '*') {
        memset(tok, 0, lexer -> token_size);
        lexer -> state = LexerTokenBlockComment;
      } else if (ch == '/') {
        memset(tok, 0, lexer -> token_size);
        lexer -> state = LexerTokenLineComment;
      } else {
        _lexer_push_back(lexer, tok);
        code = CODE_TOKEN_SLASH;
      }
      break;
    case LexerTokenBlockComment:
      tok[0] = 0;
      if (ch == '*') {
        lexer -> state = LexerTokenStar;
      }
      break;
    case LexerTokenStar:
      tok[0] = 0;
      lexer -> state = (ch == '/') ? LexerTokenInit : LexerTokenBlockComment;
      break;
    case LexerTokenLineComment:
      tok[0] = 0;
      if ((ch == '\r') || (ch == '\n')) {
        lexer -> state = LexerTokenInit;
      }
      break;
  }
  if (code != CODE_TOKEN_NOTOKEN) {
    ret = token_create(code, tok);
  }
  return ret;
}

/*
 * lexer_t - public interface
 */

lexer_t * lexer_create(int fh) {
  lexer_t *ret;
  
  ret = NEW(lexer_t);
  if (ret) {
    ret -> fh = fh;
    ret -> buflen = ret -> bufpos = 0;
    ret -> state = LexerTokenFresh;
  }
  return ret;
}

void lexer_free(lexer_t *lexer) {
  free(lexer);
}

token_t * lexer_next_token(lexer_t *lexer) {
  char *newtok;
  int ch;
  token_t *ret;
  
  ret = NULL;
  lexer -> token_size = sizeof(lexer -> short_token);
  lexer -> current_token = lexer -> short_token;
  lexer -> state = LexerTokenInit;
  newtok = NULL;
  memset(lexer -> current_token, '\0', lexer -> token_size);
  do {
    ch = _lexer_get_char(lexer);
    if (ch <= 0) {
      ret = _lexer_match_token(lexer, 0);
      break;
    }
    ret = _lexer_match_token(lexer, ch);
  } while (ret == NULL);
  if (lexer -> current_token != lexer -> short_token) {
    free(lexer -> current_token);
  }
  return ret;
}
