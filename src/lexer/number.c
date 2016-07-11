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

#define PARAM_SCI     "sci"
#define PARAM_SIGNED  "signed"
#define PARAM_HEX     "hex"
#define PARAM_FLOAT   "float"

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

typedef struct _num_config {
  int                  scientific;
  int                  sign;
  int                  hex;
  int                  flt;
} num_config_t;

static num_config_t * _num_config_create(num_config_t *config, va_list args);
static num_config_t * _num_config_set(num_config_t *, char *, data_t *);
static data_t *       _num_config_resolve(num_config_t *, char *);
static token_t *      _num_match(scanner_t *);

static vtable_t _vtable_idscanner_config[] = {
  { .id = FunctionNew,     .fnc = (void_t ) _num_config_create },
  { .id = FunctionResolve, .fnc = (void_t ) _num_config_resolve },
  { .id = FunctionSet,     .fnc = (void_t ) _num_config_set },
  { .id = FunctionUsr1,    .fnc = (void_t ) _num_match },
  { .id = FunctionUsr2,    .fnc = NULL },
  { .id = FunctionNone,    .fnc = NULL }
};

static int NumScannerConfig = -1;

/*
 * ---------------------------------------------------------------------------
 * N U M _ S C A N N E R
 * ---------------------------------------------------------------------------
 */

num_config_t * _num_config_create(num_config_t *config, va_list args) {
  config -> scientific = TRUE;
  config -> sign = TRUE;
  config -> hex = TRUE;
  config -> flt = TRUE;
  return config;
}

num_config_t * _num_config_set(num_config_t *num_config, char *param, data_t *value) {
  if (!strcmp(param, PARAM_SCI)) {
    num_config -> scientific = data_intval(value);
  } else if (!strcmp(param, PARAM_SIGNED)) {
    num_config -> sign = data_intval(value);
  } else if (!strcmp(param, PARAM_HEX)) {
    num_config -> hex = data_intval(value);
  } else if (!strcmp(param, PARAM_FLOAT)) {
    num_config -> flt = data_intval(value);
  }
  return num_config;
}

data_t * _num_config_resolve(num_config_t *num_config, char *name) {
  if (!strcmp(param, PARAM_SCI)) {
    return bool_get(num_config -> scientific)
  } else if (!strcmp(param, PARAM_SIGNED)) {
    return bool_get(num_config -> sign);
  } else if (!strcmp(param, PARAM_HEX)) {
    return bool_get(num_config -> hex);
  } else if (!strcmp(param, PARAM_FLOAT)) {
    return bool_get(num_config -> flt);
  } else {
    return NULL;
  }
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

typedescr_t * numbers_register(void) {
  typedescr_t *ret;

  NumScannerConfig = typedescr_create_and_register(NumScannerConfig,
                                                  "numbers",
                                                  _vtable_numscanner_config,
                                                  NULL);
  ret = typedescr_get(NumScannerConfig);
  typedescr_set_size(NumScannerConfig, numsc_config_t);
  return ret;
}
