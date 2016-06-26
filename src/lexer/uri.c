/*
 * uri.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <lexer.h>

/* ------------------------------------------------------------------------ */

token_code_t _lexer_state_uricomponent_handler(lexer_t *lexer, int ch) {
  token_code_t code = TokenCodeNone;

  switch (lexer -> state) {
    case LexerStateInit:
      if (isalnum(ch)) {
        lexer -> state = LexerStateURIComponent;
      } else if (ch == '%') {
        lexer -> state = LexerStateURIComponentPercent;
      }
      break;
    case LexerStateURIComponent:
      if (isalnum(ch)) {
        // Nothing
      } else if (ch == '%') {
        lexer -> state = LexerStateURIComponentPercent;
      } else {
        lexer_push_back(lexer);
        code = LexerStateURIComponent;
      }
      break;
  }
  return code;
}

