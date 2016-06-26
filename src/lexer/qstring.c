/*
 * obelix/src/lexer/qstring.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <stdlib.h>
#include <ctype.h>

#include <lexer.h>

static         int          _is_identifier(str_t *);

typedef enum _id_foldcase {
  IDCaseSensitive,
  IDFoldToLower,
  IDFoldToUpper
} id_foldcase_t;

typedef struct _id_scanner {
  int            underscore;
  int            digits;
  id_foldcase_t  foldcase;
} id_scanner_t;

static         id_scanner_t * _id_scanner_create(void);
static         void           _id_scanner_free(id_scanner_t *);
static         id_scanner_t * _id_scanner_set(id_scanner_t *, char *);
static         token_code_t   _id_scanner_scan(id_scanner_t *, lexer_t *);
static         int            _id_scanner_is_identifier(id_scanner_t *, str_t *);
__DLL_EXPORT__ token_code_t   scanner_identifier(lexer_t *, scanner_t *);


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
 * ---------------------------------------------------------------------------
 * I D _ S C A N N E R
 * ---------------------------------------------------------------------------
 */

id_scanner_t * _id_scanner_create(void) {
  id_scanner_t *ret;
  
  ret = NEW(id_scanner_t);
  ret -> underscore = TRUE;
  ret -> digits = TRUE;
  ret -> foldcase = IDCaseSensitive;
  return ret;
}

void _id_scanner_free(id_scanner_t *id_scanner) {
  if (id_scanner) {
    free(id_scanner);
  }
}

id_scanner_t * _id_scanner_set(id_scanner_t *id_scanner, char *params) {
  char *saveptr;
  char *param;
  
  for (param = strtok_r(params, " \t", &saveptr); 
       param; 
       param = strtok_r(NULL, " \t", &saveptr)) {
    if (!strcmp(param, "toupper")) {
      id_scanner -> foldcase = IDFoldToUpper;
    } else if (!strcmp(param, "tolower")) {
      id_scanner -> foldcase = IDFoldToLower;
    } else if (!strcmp(param, "nodigits")) {
      id_scanner -> digits = FALSE;
    } else if (!strcmp(param, "nounderscore")) {
      id_scanner -> underscore = FALSE;
    }
  }
  return id_scanner;
}

token_code_t _id_scanner_scan(id_scanner_t *id_scanner, lexer_t *lexer) {
  int ch;
  
  for (ch = lexer_get_char(lexer);
       ch && (isalpha(ch) || (id_scanner -> underscore && (ch == '_') || (id_scanner -> digits && isdigit(ch))));
       ch = lexer_get_char(lexer));
  lexer_push_back(lexer);
  switch (id_scanner -> foldcase) {
    case IDFoldToLower:
      str_tolower(lexer -> token);
      break;
    case IDFoldToUpper
      str_toupper(lexer -> token);
      break;
  }
  return TokenCodeIdentifier;
}


int _id_scanner_is_identifier(id_scanner_t *id_scanner, str_t *str) {
  char *s = str_chars(str);
  char *ptr;
  int   ret = TRUE;

  for (ptr = s; *ptr; ptr++) {
    if (s == ptr) {
      ret = isalpha(*ptr) || (id_scanner -> underscore && (*ptr == '_'));
    } else {
      ret = isalnum(*ptr) || (id_scanner -> underscore && (*ptr == '_'));
    }
    if (!ret) break;
  }
  return ret;
}

/*
 * ---------------------------------------------------------------------------
 * I D E N T I F I E R S C A N N E R
 * ---------------------------------------------------------------------------
 */


token_code_t scanner_identifier(lexer_t *lexer, scanner_t *scanner) {
  token_code_t code = TokenCodeNone;
  int          ch;
  id_scanner_t id_scanner;
  
  switch (lexer -> state) {
    case LexerStateFresh:
      id_scanner = _id_scanner_create();
      scanner -> data = id_scanner;
      break;

    case LexerStateParameter:
      _id_scanner_set(id_scanner, scanner -> parameter);
      break;
      
    case LexerStateInit:
      if (_id_scanner_is_identifier(id_scanner, lexer -> token)) {
        code = _id_scanner_scan(id_scanner, lexer);
      }
      break;
      
    case LexerStateStale:
      _id_scanner_free(id_scanner);
      scanner -> data = NULL;
      break;
  }
  return code;
}

token_code_t _lexer_state_qstring_handler(lexer_t *lexer, int ch) {
  token_code_t code = TokenCodeNone;

  switch (lexer -> state) {
    case LexerStateInit:
      if ((ch == '\'') || (ch == '"') || (ch == '`')) {
        lexer -> state = LexerStateQuotedStr;
        lexer -> quote = ch;
      }
      break;

    case LexerStateQuotedStr:
      if (ch == lexer -> quote) {
        code = (token_code_t) lexer -> quote;
        _dequotify(lexer -> token);
      } else if (ch == '\\') {
        lexer -> state = LexerStateQuotedStrEscape;
        str_chop(lexer -> token, 1);
      } else if (ch <= 0) {
        code = TokenCodeError;
      }
      break;
    case LexerStateQuotedStrEscape:
      if (ch == 'n') {
        str_append_char(lexer -> token, '\n');
      } else if (ch == 'r') {
        str_append_char(lexer -> token, '\r');
      } else if (ch == 't') {
        str_append_char(lexer -> token, '\t');
      } else if (ch <= 0) {
        code = TokenCodeError;
      }
      lexer -> state = LexerStateQuotedStr;
      break;
  }
  if (code == TokenCodeError) {
    str_free(lexer -> token);
    lexer -> token = str_copy_chars("Unterminated string")
  }
  return code;
}

