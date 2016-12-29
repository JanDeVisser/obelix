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

#include "liblexer.h"

#define PARAM_QUOTES   "quotes"

typedef enum _qstr_scanner_state {
  QStrInit,
  QStrQString,
  QStrEscape,
  QStrDone
} qstr_state_t;

typedef struct _qstr_config {
  scanner_config_t   _sc;
  str_t             *quotechars;
} qstr_config_t;

typedef struct _qstr_scanner {
  char   *quotechars;
  char    quote;
  data_t *quotechars_data;
} qstr_scanner_t;

static qstr_config_t *  _qstr_config_create(qstr_config_t *config, va_list args);
static data_t *         _qstr_config_resolve(qstr_config_t *, char *);
static qstr_config_t *  _qstr_config_set(qstr_config_t *, char *, data_t *);
static qstr_config_t *  _qstr_config_set_quotes(qstr_config_t *, data_t *);
static qstr_config_t *  _qstr_config_config(qstr_config_t *, array_t *);
static token_t *        _qstr_match(scanner_t *);

static qstr_scanner_t * _qstr_scanner_create(qstr_config_t *);
static void             _qstr_scanner_free(qstr_scanner_t *);
static scanner_t *      _qstr_scanner_config(scanner_t *, char *, data_t *);

static vtable_t _vtable_QStrScannerConfig[] = {
  { .id = FunctionNew,             .fnc = (void_t) _qstr_config_create },
  { .id = FunctionResolve,         .fnc = (void_t) _qstr_config_resolve },
  { .id = FunctionSet,             .fnc = (void_t) _qstr_config_set },
  { .id = FunctionMatch,           .fnc = (void_t) _qstr_match },
  { .id = FunctionGetConfig,       .fnc = (void_t) _qstr_config_config },
  { .id = FunctionDestroyScanner,  .fnc = (void_t) _qstr_scanner_free },
  { .id = FunctionReconfigScanner, .fnc = (void_t) _qstr_scanner_config },
  { .id = FunctionNone,           .fnc = NULL }
};

static int QStrScannerConfig = -1;

/* -- Q S T R _ C O N F I G  ---------------------------------------------- */

qstr_config_t *_qstr_config_create(qstr_config_t *config, va_list args) {
  _qstr_config_set_quotes(config, data_uncopy((data_t *) str_wrap("\"'`")));
  return config;
}

void _qstr_config_free(qstr_config_t *config) {
  if (config) {
    str_free(config -> quotechars);
  }
}

qstr_config_t * _qstr_config_set(qstr_config_t *config,
                                 char *name, data_t *value) {
  if (!strcmp(PARAM_QUOTES, name)) {
    return _qstr_config_set_quotes(config, value);
  } else {
    return NULL;
  }
}

qstr_config_t * _qstr_config_set_quotes(qstr_config_t *config, data_t *chars) {
  str_free(config -> quotechars);
  config -> quotechars = str_from_data(chars);
  debug(lexer, "Setting quotes to '%s'", (chars) ? str_chars(config -> quotechars) : "null");
  return config;
}

data_t * _qstr_config_resolve(qstr_config_t *config, char *name) {
  if (!strcmp(name, PARAM_QUOTES)) {
    return data_copy((data_t *) config -> quotechars);
  } else {
    return NULL;
  }
}

qstr_config_t * _qstr_config_config(qstr_config_t *config, array_t *cfg) {
  array_push(cfg, nvp_create(str_to_data(PARAM_QUOTES), data_copy((data_t *) config -> quotechars)));
  return config;
}

/* -- Q S T R _ S C A N N E R --------------------------------------------- */

qstr_scanner_t * _qstr_scanner_create(qstr_config_t *config) {
  qstr_scanner_t *qstr_scanner;

  qstr_scanner = NEW(qstr_scanner_t);
  if (config -> quotechars && str_len(config -> quotechars)) {
    qstr_scanner -> quotechars_data = data_copy((data_t *) config -> quotechars);
    qstr_scanner -> quotechars = data_tostring(qstr_scanner -> quotechars_data);
  } else {
    qstr_scanner -> quotechars_data = NULL;
    qstr_scanner -> quotechars = NULL;
  }
  qstr_scanner -> quote = 0;
  return qstr_scanner;
}

void _qstr_scanner_free(qstr_scanner_t *qstr_scanner) {
  if (qstr_scanner) {
    data_free(qstr_scanner -> quotechars_data);
    free(qstr_scanner);
  }
}

scanner_t * _qstr_scanner_config(scanner_t *scanner, char *param, data_t *value) {
  qstr_config_t  *qstr_config = (qstr_config_t *) scanner -> config;
  qstr_scanner_t *qstr_scanner = (qstr_scanner_t *) scanner -> data;

  if (!qstr_scanner) {
    qstr_scanner = _qstr_scanner_create(qstr_config);
    scanner -> data = qstr_scanner;
  }
  if (!strcmp(param, PARAM_QUOTES) && data_notnull(value)) {
    data_free(qstr_scanner -> quotechars_data);
    qstr_scanner -> quotechars_data = data_copy(value);
    qstr_scanner -> quotechars = data_tostring(qstr_scanner -> quotechars_data);
    debug(lexer, "Reconfig: Setting quotes to '%s'", qstr_scanner -> quotechars);
  }
  return scanner;
}

/* ------------------------------------------------------------------------ */

token_t * _qstr_match(scanner_t *scanner) {
  char            ch;
  token_t        *ret = NULL;
  qstr_config_t  *qstr_config = (qstr_config_t *) scanner -> config;
  qstr_scanner_t *qstr_scanner = (qstr_scanner_t *) scanner -> data;

  if (!qstr_scanner) {
    qstr_scanner = _qstr_scanner_create(qstr_config);
    scanner -> data = qstr_scanner;
  }
  if (!qstr_scanner -> quotechars) {
    debug(lexer, "_qstr_match NO quotechars");
    return NULL;
  }
  debug(lexer, "_qstr_match quotechars: %s", qstr_scanner -> quotechars);

  for (scanner -> state = QStrInit; scanner -> state != QStrDone; ) {
    ch = lexer_get_char(scanner -> lexer);
    if (!ch) {
      break;
    }

    switch (scanner->state) {
      case QStrInit:
        if (strchr(qstr_scanner -> quotechars, ch)) {
          lexer_discard(scanner -> lexer);
          qstr_scanner -> quote = ch;
          scanner -> state = QStrQString;
          debug(lexer, "Start of quotes string, quote '%c'", ch);
        } else {
          scanner -> state = QStrDone;
        }
        break;

      case QStrQString:
        if (ch == qstr_scanner -> quote) {
          lexer_discard(scanner -> lexer);
          ret = lexer_accept(scanner -> lexer, (token_code_t) ch);
          scanner -> state = QStrDone;
        } else if (ch == '\\') {
          lexer_discard(scanner -> lexer);
          scanner -> state = QStrEscape;
        } else {
          lexer_push(scanner -> lexer);
        }
        break;

      case QStrEscape:
        if (ch == 'r') {
          lexer_push_as(scanner -> lexer, '\r');
        } else if (ch == 'n') {
          lexer_push_as(scanner -> lexer, '\n');
        } else if (ch == 't') {
          lexer_push_as(scanner -> lexer, '\t');
        } else {
          lexer_push(scanner -> lexer);
        }
        scanner -> state = QStrQString;
        break;
    }
  }
  if (!ch && ((scanner -> state == QStrQString) || (scanner -> state == QStrEscape)))  {
    ret = token_create(TokenCodeError, "Unterminated string");
    lexer_accept_token(scanner -> lexer, ret);
    token_free(ret);
  }
  return scanner -> lexer -> last_token;
}

__DLL_EXPORT__ typedescr_t * qstring_register(void) {
  typedescr_register_with_name(QStrScannerConfig, "qstring", qstr_config_t);
  return typedescr_get(QStrScannerConfig);
}
