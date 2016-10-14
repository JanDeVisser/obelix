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
  NumScannerStateSciFloatExpSign,
  NumScannerStateSciFloatExp,
  NumScannerStateHexInteger,
  NumScannerStateDone,
  NumScannerStateError
} num_scanner_state_t;

typedef struct _num_config {
  scanner_config_t _sc;
  int              scientific;
  int              sign;
  int              hex;
  int              flt;
} num_config_t;

static num_config_t * _num_config_create(num_config_t *config, va_list args);
static num_config_t * _num_config_set(num_config_t *, char *, data_t *);
static data_t *       _num_config_resolve(num_config_t *, char *);
static token_t *      _num_match(scanner_t *);

static vtable_t _vtable_numscanner_config[] = {
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
 * N U M _ C O N F I G
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

data_t * _num_config_resolve(num_config_t *num_config, char *param) {
  if (!strcmp(param, PARAM_SCI)) {
    return (data_t *) bool_get(num_config -> scientific);
  } else if (!strcmp(param, PARAM_SIGNED)) {
    return (data_t *) bool_get(num_config -> sign);
  } else if (!strcmp(param, PARAM_HEX)) {
    return (data_t *) bool_get(num_config -> hex);
  } else if (!strcmp(param, PARAM_FLOAT)) {
    return (data_t *) bool_get(num_config -> flt);
  } else {
    return NULL;
  }
}

token_code_t _num_scanner_process(scanner_t *scanner, int ch) {
  num_config_t  *config = (num_config_t *) scanner -> config;
  token_code_t   code;

  switch (scanner -> state) {
    case NumScannerStateNone:
      if (config -> sign && ((ch == '-') || (ch == '+'))) {
        scanner -> state = NumScannerStatePlusMinus;
      } else if (ch == '0') {
        scanner -> state = NumScannerStateZero;
      } else if (isdigit(ch)) {
        scanner -> state = NumScannerStateNumber;
      } else if (config -> flt && (ch == '.')) {
        scanner -> state = NumScannerStateFloat;
      } else {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeNone;
      }
      break;

    case NumScannerStatePlusMinus:
      if (ch == '0') {
        scanner -> state = NumScannerStateZero;
      } else if (config -> flt && (ch == '.')) {
        scanner -> state = NumScannerStateFloat;
      } else if (isdigit(ch)) {
        scanner -> state = NumScannerStateNumber;
      } else {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeNone;
      }
      break;

    case NumScannerStateZero:
      if (ch == '0') {
        /*
         * Chop the previous zero and keep the state. This zero will be chopped
         * next time around.
         */
        str_chop(scanner -> lexer -> token, 1);
      } else if (isdigit(ch)) {
        /*
         * We don't want octal numbers. Therefore we strip
         * leading zeroes.
         */
        str_chop(scanner -> lexer -> token, 1);
        scanner -> state = NumScannerStateNumber;
      } else if (config -> flt && (ch == '.')) {
        scanner -> state = NumScannerStateFloat;
      } else if (config -> hex && (ch == 'x')) {
        /*
         * Hexadecimals are returned including the leading
         * 0x. This allows us to send both base-10 and hex ints
         * to strtol and friends.
         */
        scanner -> state = NumScannerStateHexInteger;
      } else {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeInteger;
      }
      break;

    case NumScannerStateNumber:
      if (config -> flt && (ch == '.')) {
        scanner -> state = NumScannerStateFloat;
      } else if (config -> scientific && (ch == 'e')) {
        scanner -> state = NumScannerStateSciFloat;
      } else if (!isdigit(ch)) {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeInteger;
      }
      break;

    case NumScannerStateFloat:
      if (config -> scientific &&  (ch == 'e')) {
        scanner -> state = NumScannerStateSciFloat;
      } else if (!isdigit(ch)) {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeFloat;
      }
      break;

    case NumScannerStateSciFloat:
      if ((ch == '+') || (ch == '-')) {
        scanner -> state = NumScannerStateSciFloatExpSign;
      } else if (isdigit(ch)) {
        scanner -> state = NumScannerStateSciFloatExp;
      } else {
        scanner -> state = NumScannerStateError;
      }
      break;

    case NumScannerStateSciFloatExp:
      if (!isdigit(ch)) {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeFloat;
      }
      break;

    case NumScannerStateSciFloatExpSign:
      if (isdigit(ch)) {
        scanner -> state = NumScannerStateSciFloatExp;
      } else {
        scanner -> state = NumScannerStateError;
      }
      break;

    case NumScannerStateHexInteger:
      if (!isxdigit(ch)) {
        scanner -> state = NumScannerStateDone;
        code = TokenCodeHexNumber;
      }
      break;
  }
  if ((scanner -> state != NumScannerStateDone) && (scanner -> state != NumScannerStateError)){
    lexer_push(scanner -> lexer);
  }
  return code;
}

token_t * _num_match(scanner_t *scanner) {
  int           ch;
  token_code_t  code;
  token_t      *ret = NULL;

  for (scanner -> state = NumScannerStateNone;
       (scanner -> state != NumScannerStateDone) && (scanner -> state != NumScannerStateError);) {
    ch = tolower(lexer_get_char(scanner -> lexer));
    code = _num_scanner_process(scanner, ch);
  }
  if (scanner -> state == NumScannerStateError) {
    ret = token_create(TokenCodeError, "Malformed number");
    lexer_accept_token(scanner -> lexer, ret);
    token_free(ret);
  } else if (code != TokenCodeNone) {
    ret = lexer_accept(scanner -> lexer, code);
  }
  return scanner -> lexer -> last_token;
}

/*
 * ---------------------------------------------------------------------------
 * N U M B E R  S C A N N E R
 * ---------------------------------------------------------------------------
 */

typedescr_t * number_register(void) {
  typedescr_t *ret;

  NumScannerConfig = typedescr_create_and_register(NumScannerConfig,
                                                  "number",
                                                  _vtable_numscanner_config,
                                                  NULL);
  ret = typedescr_get(NumScannerConfig);
  typedescr_set_size(NumScannerConfig, num_config_t);
  return ret;
}
