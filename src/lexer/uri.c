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

#include <ctype.h>

#include "liblexer.h"
#include <exception.h>
#include <nvp.h>

typedef struct _uri_scanner_config {
  scanner_config_t  _sc;
} uri_config_t;

static uri_config_t *  _uri_config_new(uri_config_t *config, va_list args);
static token_t *       _uri_match(scanner_t *);

static vtable_t _vtable_URIScannerConfig[] = {
  { .id = FunctionNew,       .fnc = (void_t) _uri_config_new },
  { .id = FunctionMatch,     .fnc = (void_t) _uri_match },
  { .id = FunctionNone,      .fnc = NULL }
};

static int URIScannerConfig = -1;

/* -- I D _ C O N F I G --------------------------------------------------- */

uri_config_t * _uri_config_new(uri_config_t *config, va_list args) {
  debug(lexer, "_uri_config_create");
  return config;
}

/*
 * TODO: Percent encoding.
 */
token_t * _uri_match(scanner_t *scanner) {
  int          ch;

  debug(lexer, "_uri_match");
  for (ch = lexer_get_char(scanner -> lexer);
       ch && (isalnum(ch) || strchr("-_~", ch));
       ch = lexer_get_char(scanner -> lexer)) {
    lexer_push(scanner -> lexer);
  }
  return lexer_accept(scanner -> lexer, 'u');
}

/* -- I D E N T I F I E R  S C A N N E R ---------------------------------- */

__DLL_EXPORT__ typedescr_t * uri_register(void) {
  URIScannerConfig = typedescr_create_and_register(
      URIScannerConfig, "uri", _vtable_URIScannerConfig, NULL);
  typedescr_set_size(URIScannerConfig, uri_config_t);
  return typedescr_get(URIScannerConfig);;
}
