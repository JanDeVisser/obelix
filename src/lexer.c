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

#include "core.h"


typedef struct _tokendef {
  int code;
  char *pattern;
} tokendef_t;

typedef struct _token {
  tokendef_t *tokendef;
  char *token;
} token_t;

#define LEXER_BUFSIZE    	16384
#define LEXER_SHORT_TOKEN_SZ	256

typedef struct _lexer {
  int   fh;
  char  buf[BUFSIZE];
  int   buflen;
  int   bufpos;
  char  short_token[256];
} lexer_t;

extern lexer_t * lexer_create(int);
extern token_t * lexer_next_token(lexer_t *);

#define		CODE_TOKEN_WHITESPACE   -1
#define		CODE_TOKEN_STRING	0
#define		CODE_TOKEN_INTEGER	1
#define		CODE_TOKEN_PLUS		((int) '+')
#define		CODE_TOKEN_MINUS	((int) '-')
#define		CODE_TOKEN_DOT		((int) '.')
#define		CODE_TOKEN_OPENPAR	((int) '(')
#define		CODE_TOKEN_CLOSEPAR	((int) ')')
#define		CODE_TOKEN_OPENBRACE	((int) '{')
#define		CODE_TOKEN_CLOSEBRACE	((int) '}')
#define		CODE_TOKEN_OPENBRACKET	((int) '[')
#define		CODE_TOKEN_CLOSEBRACKET	((int) ']')


static tokendef_t _token_definitions[] = {
  { code: CODE_TOKEN_WHITESPACE,   pattern: ":W+" },
  { code: CODE_TOKEN_STRING,       pattern: "[:A_][:X_]*" },
  { code: CODE_TOKEN_INTEGER,      pattern: "0+" },
  { code: CODE_TOKEN_PLUS,         pattern: "+" },
  { code: CODE_TOKEN_MINUS,        pattern: "-" },
  { code: CODE_TOKEN_ASTERISK,     pattern: "*" },
  { code: CODE_TOKEN_SLASH,        pattern: "/" },
  { code: CODE_TOKEN_DOT,          pattern: "." },
  { code: CODE_TOKEN_COMMA,        pattern: "," },
  { code: CODE_TOKEN_OPENPAR,      pattern: "(" },
  { code: CODE_TOKEN_CLOSEPAR,     pattern: ")" },
  { code: CODE_TOKEN_OPENBRACE,    pattern: "{" },
  { code: CODE_TOKEN_CLOSEBRACE,   pattern: "}" },
  { code: CODE_TOKEN_OPENBRACKET,  pattern: "[" },
  { code: CODE_TOKEN_CLOSEBRACKET, pattern: "]" }
};

int _lexer_get_char(lexer_t *lexer) {
  if ((lexer -> buflen <= 0) || (lexer -> bufpos >= lexer -> buflen)) {
    lexer -> buflen = read(lexer -> fh, lexer -> buf, LEXER_BUFSIZE);
    if (lexer -> buflen <= 0) {
      return lexer -> buflen;
    }
    lexer -> bufpos = 0;
  }
  return (int) lexer -> buf[lexer -> bufpos++];
}

void _lexer_push_back(lexer_t *lexer, int ch) {
  lexer -> bufpos--;
}

token_t * _lexer_match_token(lexer_t *lexer, char *tok, int ch, list_t *candidates) {
  int ix;
  char *pat;
  
  if (!list_size(candidates)) {
    for (ix = 0; ix < sizeof(_token_definitions) / sizeof(tokendef_t)); ix++) {
      pat = _token_definitions[ix].pattern;
      
    }
  }
}

lexer_t * lexer_create(int fh) {
  lexer_t *ret;
  
  ret = NEW(lexer_t);
  if (ret) {
    ret -> fh = fh;
    ret -> buflen = ret -> bufpos = 0;
  }
  return ret;
}

token_t * lexer_next_token(lexer_t *lexer) {
  list_t *candidates;
  char *tok, *newtok;
  int tokbufsize = 256;
  int ch;
  token_t *ret;
  
  candidates = list_create();
  if (!candidates) {
    return NULL;
  }
  ret = NULL;
  tok = lexer -> short_token;
  newtok = NULL;
  memset(tok, '\0', tokbufsize);
  do {
    ch = _lexer_get_char(lexer);
    if (ch < 0) {
      break;
    }
    if (ch && strlen(tok) >= tokbufsize - 2) {
      newtok = malloc(2 * tokbufsize);
      if (!newtok) break;
      strcpy(newtok, tok);
      if (tokbufsize > LEXER_SHORT_TOKEN_SZ) {
	free(tok);
      }
      tok = newtok;
      tokbufsize *= 2;
    }
    ret = _lexer_match_token(lexer, tok, ch, candidates);
  } while (ret == NULL);
  list_free(candidates);
  free(newtok);
  return ret;
}
