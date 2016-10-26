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
  char              *quotechars;
} qstr_config_t;

static qstr_config_t * _qstr_config_create(qstr_config_t *config, va_list args);
static data_t *        _qstr_config_resolve(qstr_config_t *, char *);
static qstr_config_t * _qstr_config_set(qstr_config_t *, char *, data_t *);
static qstr_config_t * _qstr_config_set_quotes(qstr_config_t *, char *);
static qstr_config_t * _qstr_config_config(qstr_config_t *, array_t *);
static token_t *       _qstr_match(scanner_t *);

static vtable_t _vtable_qstrscanner_config[] = {
  { .id = FunctionNew,       .fnc = (void_t) _qstr_config_create },
  { .id = FunctionResolve,   .fnc = (void_t) _qstr_config_resolve },
  { .id = FunctionSet,       .fnc = (void_t) _qstr_config_set },
  { .id = FunctionMatch,     .fnc = (void_t) _qstr_match },
  { .id = FunctionGetConfig, .fnc = (void_t) _qstr_config_config },
  { .id = FunctionNone,      .fnc = NULL }
};

static int QStrScannerConfig = -1;

/* -- Q S T R _ C O N F I G  ---------------------------------------------- */

qstr_config_t *_qstr_config_create(qstr_config_t *config, va_list args) {
  config -> quotechars = strdup("\"'`");
  return config;
}

void _qstr_config_free(qstr_config_t *config) {
  if (config) {
    free(config -> quotechars);
  }
}

qstr_config_t * _qstr_config_set(qstr_config_t *config,
                                 char *name, data_t *value) {
  if (!strcmp(PARAM_QUOTES, name)) {
    return _qstr_config_set_quotes(config, data_tostring(value));
  } else {
    return NULL;
  }
}

qstr_config_t * _qstr_config_set_quotes(qstr_config_t *config, char *chars) {
  char *newstr;

  if (!config -> quotechars) {
    config -> quotechars = strdup(chars);
  } else {
    newstr = (char *) _new(strlen(config -> quotechars) + strlen(chars) + 1);
    strcpy(newstr, config -> quotechars);
    strcat(newstr, chars);
    free(config -> quotechars);
    config -> quotechars = newstr;
  }
  return config;
}

data_t * _qstr_config_resolve(qstr_config_t *config, char *name) {
  if (!strcmp(name, PARAM_QUOTES)) {
    return (data_t *) str_wrap(config -> quotechars);
  } else {
    return NULL;
  }
}

qstr_config_t * _qstr_config_config(qstr_config_t *config, array_t *cfg) {
  array_push(cfg, nvp_create(str_to_data(PARAM_QUOTES), (data_t *) str_copy_chars(config -> quotechars)));
  return config;
}

token_t * _qstr_match(scanner_t *scanner) {
  char           ch;
  token_t       *ret = NULL;
  qstr_config_t *qstr_config = (qstr_config_t *) scanner -> config;

  if (!qstr_config -> quotechars || !*qstr_config -> quotechars) {
    return NULL;
  }
  mdebug(lexer, "_qstr_match quotechars: %s", qstr_config -> quotechars);

  for (scanner -> state = QStrInit; scanner -> state != QStrDone; ) {
    ch = lexer_get_char(scanner -> lexer);
    if (!ch) {
      break;
    }

    switch (scanner->state) {
      case QStrInit:
        if (strchr(qstr_config -> quotechars, ch)) {
          lexer_discard(scanner -> lexer);
          scanner -> data = (void *) (intptr_t) ch;
          scanner -> state = QStrQString;
          mdebug(lexer, "Start of quotes string, quote '%c'", ch);
        } else {
          scanner -> state = QStrDone;
        }
        break;

      case QStrQString:
        if (ch == (int) (intptr_t) scanner -> data) {
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

typedescr_t * qstring_register(void) {
  typedescr_t *ret;

  QStrScannerConfig = typedescr_create_and_register(
      QStrScannerConfig, "qstring", _vtable_qstrscanner_config, NULL);
  ret = typedescr_get(QStrScannerConfig);
  typedescr_set_size(QStrScannerConfig, qstr_config_t);
  return ret;
}
