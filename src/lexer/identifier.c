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

static         int          _is_identifier(str_t *);

typedef enum _id_foldcase {
  IDCaseSensitive,
  IDFoldToLower,
  IDFoldToUpper
} id_foldcase_t;

typedef struct _id_config {
  int            underscore;
  int            digits;
  id_foldcase_t  foldcase;
} id_config_t;

typedef struct _id_scanner {
  id_config_t   *config;
} id_scanner_t;

static id_config_t *      _id_config_create(void);
static void               _id_config_free(id_config_t *);
static id_config_t *      _id_config_set(id_config_t *, char *);

static id_scanner_t *     _id_scanner_create(id_config_t *);
static void               _id_scanner_free(id_scanner_t *);
static token_t *          _id_scanner_scan(id_scanner_t *, lexer_t *);
static int                _id_scanner_is_identifier(id_scanner_t *, str_t *);

static scanner_config_t * identifier_configure(scanner_config_t *, data_t *);
static scanner_config_t * identifier_unconfigure(scanner_config_t *);
static scanner_t *        identifier_initialize(scanner_t *);
static scanner_t *        identifier_destroy(scanner_t *);
static token_t *          identifier_match(scanner_t *);

static scanner_funcs_t _identifier_funcs = {
  .config         = identifier_configure,
  .unconfig       = identifier_unconfigure,
  .initialize     = identifier_initialize,
  .destroy        = identifier_destroy,
  .match          = identifier_match,
  .match_2nd_pass = NULL
};

/* -- I D _ C O N F I G --------------------------------------------------- */

id_config_t * _id_config_create(void) {
  id_config_t *ret;
  
  ret = NEW(id_config_t);
  ret -> underscore = TRUE;
  ret -> digits = TRUE;
  ret -> foldcase = IDCaseSensitive;
  return ret;
}

void _id_config_free(id_config_t *id_config) {
  if (id_config) {
    free(id_config);
  }
}

id_config_t * _id_config_set(id_config_t *id_config, char *params) {
  char *saveptr;
  char *param;
  
  for (param = strtok_r(params, SCANNER_CONFIG_SEPARATORS, &saveptr); 
       param; 
       param = strtok_r(NULL, SCANNER_CONFIG_SEPARATORS, &saveptr)) {
    if (!strcmp(param, "toupper")) {
      id_config -> foldcase = IDFoldToUpper;
    } else if (!strcmp(param, "tolower")) {
      id_config -> foldcase = IDFoldToLower;
    } else if (!strcmp(param, "nodigits")) {
      id_config -> digits = FALSE;
    } else if (!strcmp(param, "nounderscore")) {
      id_config -> underscore = FALSE;
    }
  }
  return id_config;
}

/* -- I D _ S C A N N E R ------------------------------------------------- */

id_scanner_t * _id_scanner_create(id_config_t *config) {
  id_scanner_t *ret;

  ret = NEW(id_scanner_t);
  ret -> config = config;
  return ret;
}

void _id_scanner_free(id_scanner_t *id_scanner) {
  if (id_scanner) {
    free(id_scanner);
  }
}

token_t * _id_scanner_scan(id_scanner_t *id_scanner, lexer_t *lexer) {
  token_t *ret;
  int      ch;
  
  for (ch = lexer_get_char(lexer);
       ch && (isalpha(ch) ||
              (id_scanner -> underscore && (ch == '_')) ||
              (id_scanner -> digits && isdigit(ch)));
       ch = lexer_get_char(lexer)) {
    lexer_push(lexer);
  }
  if (str_len(lexer -> token)) {
    switch (id_scanner -> foldcase) {
      case IDFoldToLower:
        str_tolower(lexer -> token);
        break;
      case IDFoldToUpper:
          str_toupper(lexer -> token);
        break;
    }
    ret = lexer_accept(lexer, TokenCodeIdentifier);
  }
  return ret;
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

/* -- I D E N T I F I E R  S C A N N E R ---------------------------------- */

scanner_config_t * identifier_configure(scanner_config_t *config, data_t *data) {
  id_config_t *id_config = (id_config_t *) config -> config;
  char        *value;

  if (!config -> config) {
    id_config = _id_config_create();
    config -> config = id_config;
  }
  value = (data) ? data_tostring(data) : "";
  _id_config_set(id_config, value);

  return config;
}

scanner_config_t * identifier_unconfigure(scanner_config_t *config) {
  id_config_t *id_config = (id_config_t *) config -> config;

  _id_config_free(id_config);
  config -> config = NULL;
  return config;
}

scanner_t * identifier_initialize(scanner_t *scanner) {
  id_scanner_t *id_scanner;

  id_scanner = _id_scanner_create((id_config_t *) scanner -> config -> config);
  scanner -> data = id_scanner;
  return scanner;
}

scanner_t * identifier_destroy(scanner_t *scanner) {
  id_scanner_t *id_scanner = (id_scanner_t *) scanner -> data;

  _id_scanner_free(id_scanner);
  scanner -> data = NULL;
  return scanner;
}

token_t * identifier_match(scanner_t *scanner) {
  id_scanner_t *id_scanner = (id_scanner_t *) scanner -> data;

  return _id_scanner_scan(id_scanner, scanner -> lexer);
}

void identifier_register(void) {
  scanner_def_register("id", &_identifier_funcs);
}
