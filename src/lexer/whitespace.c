/*
 * obelix/src/lexer/keywords.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <ctype.h>

#include <function.h>
#include <lexer.h>

#define PARAM_IGNOREWS     "ignorews"
#define PARAM_IGNORENL     "ignorenl"
#define PARAM_IGNOREALL    "ignoreall"
#define PARAM_ONNEWLINE    "onnewline"

typedef lexer_t (*onnewline_t)(lexer_t *);

typedef enum _ws_scanner_state {
  WSSInit,
  WSSWhitespace,
  WSSCR,
  WSSNewline,
  WSSDone
} ws_state_t;

typedef struct _ws_config {
  scanner_config_t  _sc;
  int               ignore_nl;
  int               ignore_ws;
  function_t       *onnewline;
} ws_config_t;

static ws_config_t *      _ws_config_create(ws_config_t *config, va_list args);
static data_t *           _ws_config_resolve(ws_config_t *, char *);
static ws_config_t *      _ws_config_set(ws_config_t *, char *, data_t *);
static token_t *          _ws_match(scanner_t *);

static vtable_t _vtable_wsscanner_config[] = {
  { .id = FunctionNew,     .fnc = (void_t ) _ws_config_create },
  { .id = FunctionResolve, .fnc = (void_t ) _ws_config_resolve },
  { .id = FunctionSet,     .fnc = (void_t ) _ws_config_set },
  { .id = FunctionUsr1,    .fnc = (void_t ) _ws_match },
  { .id = FunctionUsr2,    .fnc = NULL },
  { .id = FunctionNone,    .fnc = NULL }
};

static int WSScannerConfig = -1;

/* -- W S _ C O N F I G  -------------------------------------------------- */

ws_config_t * _ws_config_create(ws_config_t *config, va_list args) {
  config -> ignore_nl = FALSE;
  config -> ignore_ws = FALSE;
  return config;
}

ws_config_t * _ws_config_set(ws_config_t *ws_config, char *name, data_t *data) {
  function_t *fnc = NULL;
  ws_config_t *ret = ws_config;

  if (!strcmp(name, PARAM_IGNOREWS)) {
    ws_config -> ignore_ws = data_intval(data);
  } else if (!strcmp(name, PARAM_IGNORENL)) {
    ws_config -> ignore_nl = data_intval(data);
  } else if (!strcmp(name, PARAM_IGNOREALL)) {
    ws_config -> ignore_nl = data_intval(data);
    ws_config -> ignore_ws = data_intval(data);
  } else if (!strcmp(name, PARAM_ONNEWLINE)) {
    if (data) {
      fnc = data_as_function(data);
      if (!fnc) {
        fnc = function_parse(data_tostring(data));
      }
    }
    ws_config -> onnewline = function_copy(fnc);
  } else {
    ret = NULL;
  }
  return ret;
}

data_t * _ws_config_resolve(ws_config_t *ws_config, char *name) {
  if (!strcmp(name, PARAM_IGNOREWS)) {
    return (data_t *) bool_get(ws_config -> ignore_ws);
  } else if (!strcmp(name, PARAM_IGNORENL)) {
    return (data_t *) bool_get(ws_config -> ignore_nl);
  } else if (!strcmp(name, PARAM_IGNOREALL)) {
    return (data_t *) bool_get(ws_config -> ignore_ws && ws_config -> ignore_nl);
  } else if (!strcmp(name, PARAM_ONNEWLINE)) {
    return data_copy((data_t *) ws_config -> onnewline);
  } else {
    return NULL;
  }
}

token_t * _ws_match(scanner_t *scanner) {
  char         ch;
  token_t     *ret = NULL;
  ws_config_t *ws_config = (ws_config_t *) scanner -> config;
  int          nl_is_ws;

  nl_is_ws = ws_config -> ignore_nl && !ws_config -> ignore_ws;
  scanner -> state = WSSInit;
  
  for (ch = lexer_get_char(scanner -> lexer);
       ch && (scanner -> state != WSSDone);
       ch = lexer_get_char(scanner -> lexer)) {

    if ((ch == '\r') || ((scanner -> state != WSSCR) && (ch == '\n'))) {
      if (ws_config -> onnewline) {
        ((onnewline_t) ws_config -> onnewline -> fnc)(scanner -> lexer);
      }
    }

    switch (scanner -> state) {

      case WSSInit:
        if (isspace(ch)) {
          switch (ch) {
            case '\r':
              scanner -> state = (nl_is_ws) ? WSSWhitespace : WSSCR;
              break;
            case '\n':
              scanner -> state = (nl_is_ws) ? WSSWhitespace : WSSNewline;
              break;
            default:
              scanner -> state = WSSWhitespace;
              break;
          }
          lexer_push(scanner -> lexer);
        } else {
          scanner -> state = WSSDone;
        }
        break;

      case WSSCR:
        if (ch == '\n') {
          scanner -> state = WSSNewline;
          lexer_push(scanner -> lexer);
        } else if (ch == '\r') {
          lexer_push(scanner -> lexer);
        } else {
          if (ws_config -> ignore_nl) {
            lexer_reset(scanner -> lexer);
          } else {
            ret = lexer_accept(scanner -> lexer, TokenCodeNewLine);
          }
          scanner -> state = WSSDone;
        }
        break;

      case WSSNewline:
        if ((ch == '\r') || (ch == '\n')) {
          if (ch == '\r') {
            scanner -> state = WSSCR;
          }
          lexer_push(scanner -> lexer);
        } else {
          if (ws_config -> ignore_nl) {
            lexer_reset(scanner -> lexer);
          } else {
            ret = lexer_accept(scanner -> lexer, TokenCodeNewLine);
          }
          scanner -> state = WSSDone;
        }
        break;

      case WSSWhitespace:
        if (!isspace(ch) ||
            (!nl_is_ws && ((ch == '\r') || (ch == '\n')))) {
          if (ws_config -> ignore_ws) {
            lexer_reset(scanner -> lexer);
          } else {
            ret = lexer_accept(scanner -> lexer, TokenCodeWhitespace);
          }
          scanner -> state = WSSDone;
        } else {
          lexer_push(scanner -> lexer);
        }
        break;
    }
  }
  return ret;
}

void whitespace_register(void) {
  lexer_init();
  WSScannerConfig = typedescr_create_and_register(WSScannerConfig,
                                                  "whitespace",
                                                  _vtable_wsscanner_config,
                                                  NULL);
  typedescr_assign_inheritance(typedescr_get(WSScannerConfig), ScannerConfig);
  typedescr_set_size(WSScannerConfig, ws_config_t);
  scanner_config_register(typedescr_get(WSScannerConfig));
}
