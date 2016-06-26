/*
 * obelix/src/lexer/identifier.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

typedef enum _num_scanner_state {
  NumScannerStateNone = 0,
  NumScannerStatePlusMinus,
  NumScannerStateZero,
  NumScannerStateNumber,
  NumScannerStateFloat,
  NumScannerStateFloatFraction,
  NumScannerStateSciFloat,
  NumScannerStateSciFloatExp,
  NumScannerStateHexInteger,
  NumScannerStateDone
} num_scanner_state_t;

typedef struct _num_scanner {
  int                  scientific;
  int                  sign;
  int                  hex;
  int                  flt;
  num_scanner_state_t  state;
  token_code_t         code;
} num_scanner_t;

static         num_scanner_t * _num_scanner_create(void);
static         void            _num_scanner_free(num_scanner_t *);
static         num_scanner_t * _num_scanner_set(num_scanner_t *, char *);
static         token_code_t    _num_scanner_scan(num_scanner_t *, lexer_t *);
__DLL_EXPORT__ token_code_t    scanner_number(lexer_t *, scanner_t *);

/*
 * ---------------------------------------------------------------------------
 * N U M _ S C A N N E R
 * ---------------------------------------------------------------------------
 */

num_scanner_t * _num_scanner_create(void) {
  num_scanner_t *ret;
  
  ret = NEW(num_scanner_t);
  ret -> scientific = TRUE;
  ret -> sign = TRUE;
  ret -> hex = TRUE;
  ret -> flt = TRUE;
  return ret;
}

void _num_scanner_free(num_scanner_t *num_scanner) {
  if (num_scanner) {
    free(num_scanner);
  }
}

num_scanner_t * _num_scanner_set(num_scanner_t *num_scanner, char *params) {
  char *saveptr;
  char *param;
  
  for (param = strtok_r(params, " \t", &saveptr); 
       param; 
       param = strtok_r(NULL, " \t", &saveptr)) {
    if (!strcmp(param, "sci")) {
      num_scanner -> scientific = TRUE;
    } else if (!strcmp(param, "nosci")) {
      num_scanner -> scientific = FALSE;
    } else if (!strcmp(param, "signed")) {
      num_scanner -> sign = TRUE;
    } else if (!strcmp(param, "nosigned")) {
      num_scanner -> sign = FALSE;
    } else if (!strcmp(param, "hex")) {
      num_scanner -> hex = TRUE;
    } else if (!strcmp(param, "nohex")) {
      num_scanner -> hex = FALSE;
    } else if (!strcmp(param, "float")) {
      num_scanner -> flt = TRUE;
    } else if (!strcmp(param, "nofloat")) {
      num_scanner -> flt = FALSE;
    }
  }
  return num_scanner;
}

void _num_scanner_process(num_scanner_t *num_scanner, int ch) {
  switch (num_scanner -> state) {
    case NumScannerStateNone:
      if (num_scanner -> sign && ((ch == '-') || (ch == '+'))) {
        num_scanner -> state = NumScannerStatePlusMinus;
      } else if (ch == '0') {
        num_scanner -> state = NumScannerStateZero;
      } else if (isdigit(ch)) {
        num_scanner -> state = NumScannerStateNumber;
      } else if (num_scanner -> flt && (ch == '.')) {
        num_scanner -> state = NumScannerStateFloat;
      } else {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeNone;
      }
      break;
      
    case NumScannerStatePlusMinus:
      if (ch == '0') {
        num_scanner -> state = NumScannerStateZero;
      } else if (num_scanner -> flt && (ch == '.')) {
        num_scanner -> state = NumScannerStateFloat;
      } else if (isdigit(ch)) {
        num_scanner -> state = NumScannerStateNumber;
      } else {
        num_scanner -> state = NumScannerStateDone;
      }
      break;
      
    case NumScannerStateZero:
      /* FIXME: Second leading zero */
      if (isdigit(ch)) {
        /*
         * We don't want octal numbers. Therefore we strip
         * leading zeroes.
         */
        str_chop(lexer -> token, 2);
        str_append_char(lexer -> token, ch);
        num_scanner -> state = NumScannerStateNumber;
      } else if (num_scanner -> flt && (ch == '.')) {
        num_scanner -> state = NumScannerStateFloat;
      } else if (num_scanner -> hex && (ch == 'x')) {
        /*
         * Hexadecimals are returned including the leading
         * 0x. This allows us to send both base-10 and hex ints
         * to strtol and friends.
         */
        num_scanner -> state = NumScannerStateHexInteger;
      } else {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeInteger;
      }
      break;
      
    case NumScannerStateNumber:
      if (num_scanner -> flt && (ch == '.')) {
        num_scanner -> state = NumScannerStateFloat;
      } else if (num_scanner -> scientific && ('e')) {
        num_scanner -> state = NumScannerStateSciFloat;
      } else if (!isdigit(ch)) {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeInteger;
      }
      break;
      
    case NumScannerStateFloat:
      if (num_scanner -> scientific &&  (ch == 'e')) {
        num_scanner -> state = NumScannerStateSciFloat;
      } else if (!isdigit(ch)) {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeFloat;
      }
      break;
      
    case NumScannerStateSciFloat:
      if ((ch == '+') || (ch == '-')) {
        // Nothing
      } else if (isdigit(ch)) {
        num_scanner -> state = NumScannerStateSciFloatExp;
      } else {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeNone;
      }
      break;
      
    case NumScannerStateSciFloatExp:
      if (!isdigit(ch)) {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeFloat;
      }
      break;

    case NumScannerStateHexInteger:
      if (!isxdigit(ch)) {
        num_scanner -> state = NumScannerStateDone;
        num_scanner -> code = TokenCodeHexNumber;
      }
      break;
  }
}

token_code_t _num_scanner_scan(num_scanner_t *num_scanner, lexer_t *lexer) {
  int           ch;
  token_code_t  code;
  
  while ((ch = tolower(lexer_get_char(lexer))) && 
         (num_scanner -> state != NumScannerStateDone)) {
    _num_scanner_process(num_scanner, ch);
  }
  switch (num_scanner -> state) {
    case NumScannerStatePlusMinus:
      code = lexer -> current;
      break;
    case NumScannerStateNumber:
    case NumScannerStateZero:
      code = TokenCodeInteger;
      break;
    
  }
  lexer_push_back(lexer);
  return code;
}


/*
 * ---------------------------------------------------------------------------
 * N U M B E R  S C A N N E R
 * ---------------------------------------------------------------------------
 */


token_code_t scanner_number(lexer_t *lexer, scanner_t *scanner) {
  token_code_t code = TokenCodeNone;
  int          ch;
  num_scanner_t num_scanner;
  
  switch (lexer -> state) {
    case LexerStateFresh:
      num_scanner = _num_scanner_create();
      scanner -> data = num_scanner;
      break;

    case LexerStateParameter:
      _num_scanner_set(num_scanner, scanner -> parameter);
      break;
      
    case LexerStateInit:
      ch = lexer -> current;
      if (num_scanner -> state) {
        code = _num_scanner_scan(num_scanner, lexer);
      }
      break;
      
    case LexerStateStale:
      _num_scanner_free(num_scanner);
      scanner -> data = NULL;
      break;
  }
  return code;
}

