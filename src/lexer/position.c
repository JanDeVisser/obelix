/*
 * obelix/src/lexer/whitespace.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
#include <function.h>

#define PARAM_ONNEWLINE    "onnewline"

typedef lexer_t (*onnewline_t)(lexer_t *);

typedef enum _pos_scanner_state {
  PosInit,
  PosCR,
  PosNewline,
  PosDone
} pos_state_t;

typedef struct _pos_config {
  scanner_config_t  _sc;
  function_t       *onnewline;
} pos_config_t;

static pos_config_t * _pos_config_create(pos_config_t *config, va_list args);
static data_t *       _pos_config_resolve(pos_config_t *, char *);
static pos_config_t * _pos_config_set(pos_config_t *, char *, data_t *);
static pos_config_t * _pos_config_config(pos_config_t *, array_t *);
static token_t *      _pos_match(scanner_t *);

static vtable_t _vtable_posscanner_config[] = {
  { .id = FunctionNew,       .fnc = (void_t ) _pos_config_create },
  { .id = FunctionResolve,   .fnc = (void_t ) _pos_config_resolve },
  { .id = FunctionSet,       .fnc = (void_t ) _pos_config_set },
  { .id = FunctionMatch2,    .fnc = (void_t ) _pos_match },
  { .id = FunctionGetConfig, .fnc = (void_t ) _pos_config_config },
  { .id = FunctionNone,      .fnc = NULL }
};

static int PosScannerConfig = -1;
static int position_debug = 0;

/* -- P O S _ C O N F I G  ------------------------------------------------ */

pos_config_t * _pos_config_create(pos_config_t *config, va_list args) {
  config -> _sc.priority = -10;
  config -> onnewline = NULL;
  return config;
}

pos_config_t * _pos_config_set(pos_config_t *pos_config, char *name, data_t *data) {
  function_t   *fnc = NULL;
  pos_config_t *ret = pos_config;

  if (!strcmp(name, PARAM_ONNEWLINE)) {
    if (data) {
      if (data_is_string(data)) {
        fnc = function_parse(data_tostring(data));
      } else {
        fnc = (function_t *) data_copy(data);
      }
    }
    pos_config -> onnewline = fnc;
  } else {
    ret = NULL;
  }
  return ret;
}

data_t * _pos_config_resolve(pos_config_t *pos_config, char *name) {
  if (!strcmp(name, PARAM_ONNEWLINE)) {
    return (pos_config -> onnewline) ? data_copy((data_t *) pos_config -> onnewline) : data_null();
  } else {
    return NULL;
  }
}

pos_config_t * _pos_config_config(pos_config_t *config, array_t *cfg) {
  if (config -> onnewline) {
    array_push(cfg, nvp_create(str_to_data(PARAM_ONNEWLINE), (data_t *) function_copy(config -> onnewline)));
  }
  return config;
}

token_t * _pos_match(scanner_t *scanner) {
  char          ch;
  token_t      *ret = NULL;
  pos_config_t *pos_config = (pos_config_t *) scanner -> config;
  int           count = 0;
  lexer_t      *lexer = scanner -> lexer;

  for (count = 0, ch = lexer_get_char(scanner -> lexer);
       ch && (count < lexer -> scanned);
       count++, ch = lexer_get_char(scanner -> lexer)) {

    ch = lexer_get_char(scanner -> lexer);
    if ((ch == '\r') || ((scanner -> state != '\r') && (ch == '\n'))) {
      debug(position, "Processing newline %p", pos_config -> onnewline);
      if (pos_config -> onnewline) {
        ((onnewline_t) pos_config -> onnewline->fnc)(lexer);
        debug(position, "Newline processed");
      }
      lexer -> line++;
      lexer -> column = 0;
    } else if ((ch != '\r') && (ch != '\n')) {
      lexer -> column++;
    }
    scanner -> state = ch;
  }
  return ret;
}

__DLL_EXPORT__ typedescr_t * position_register(void) {
  typedescr_t *ret;

  logging_register_category("position", &position_debug);
  PosScannerConfig = typedescr_create_and_register(
      PosScannerConfig, "position", _vtable_posscanner_config, NULL);
  ret = typedescr_get(PosScannerConfig);
  typedescr_set_size(PosScannerConfig, pos_config_t);
  return ret;
}
