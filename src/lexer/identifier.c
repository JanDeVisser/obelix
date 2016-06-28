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

#define PARAM_TOUPPER         "toupper"
#define PARAM_TOLOWER         "tolower"
#define PARAM_CASESENSITIVE   "casesensitive"
#define PARAM_DIGITS          "digits"
#define PARAM_UNDERSCORE      "underscore"

typedef enum _id_foldcase {
  IDCaseSensitive = 0,
  IDFoldToLower,
  IDFoldToUpper
} id_foldcase_t;

typedef struct _id_config {
  scanner_config_t  _sc;
  int               underscore;
  int               digits;
  id_foldcase_t     foldcase;
} id_config_t;

static id_config_t *      _id_config_create(void);
static id_config_t *      _id_config_set(id_config_t *, char *, data_t *);
static data_t *           _id_config_resolve(id_config_t *, char *);
static token_t *          _id_match(scanner_t *);

static vtable_t _vtable_idscanner_config[] = {
  { .id = FunctionNew,     .fnc = (void_t ) _id_config_create },
  { .id = FunctionResolve, .fnc = (void_t ) _id_config_resolve },
  { .id = FunctionSet,     .fnc = (void_t ) _id_config_set },
  { .id = FunctionUsr1,    .fnc = (void_t ) _id_match },
  { .id = FunctionUsr2,    .fnc = NULL },
  { .id = FunctionNone,    .fnc = NULL }
};

static int IDScannerConfig = -1;

/* -- I D _ C O N F I G --------------------------------------------------- */

id_config_t * _id_config_create(void) {
  id_config_t *ret;
  
  ret = NEW(id_config_t);
  ret -> underscore = TRUE;
  ret -> digits = TRUE;
  ret -> foldcase = IDCaseSensitive;
  return ret;
}

id_config_t * _id_config_set(id_config_t *id_config, char *name, data_t *value) {
  id_config_t *ret = id_config;

  if (!strcmp(name, PARAM_TOUPPER)) {
    id_config -> foldcase = data_intval(value) ? IDFoldToUpper : IDCaseSensitive;
  } else if (!strcmp(name, PARAM_TOLOWER)) {
    id_config -> foldcase = data_intval(value) ? IDFoldToLower : IDCaseSensitive;
  } else if (!strcmp(name, PARAM_CASESENSITIVE) && data_intval(value)) {
    id_config -> foldcase = IDCaseSensitive;
  } else if (!strcmp(name, PARAM_DIGITS)) {
    id_config -> digits = data_intval(value);
  } else if (!strcmp(name, PARAM_UNDERSCORE)) {
    id_config -> underscore = data_intval(value);
  } else {
    ret = NULL;
  }
  return ret;
}

data_t * _id_config_resolve(id_config_t *id_config, char *name) {
  if (!strcmp(name, PARAM_TOUPPER)) {
    return (data_t *) bool_get(id_config -> foldcase == IDFoldToLower);
  } else if (!strcmp(name, PARAM_TOLOWER)) {
    return (data_t *) bool_get(id_config -> foldcase == IDFoldToLower);
  } else if (!strcmp(name, PARAM_CASESENSITIVE)) {
    return (data_t *) bool_get(id_config -> foldcase == IDCaseSensitive);
  } else if (!strcmp(name, PARAM_DIGITS)) {
    return (data_t *) bool_get(id_config -> digits);
  } else if (!strcmp(name, PARAM_UNDERSCORE)) {
    return (data_t *) bool_get(id_config -> underscore);
  } else {
    return NULL;
  }
}

token_t * _id_match(scanner_t *scanner) {
  int          ch;
  id_config_t *config = (id_config_t *) scanner -> config;
  
  for (ch = lexer_get_char(scanner -> lexer);
       ch && (isalpha(ch) ||
              (config -> underscore && (ch == '_')) ||
              (config -> digits && isdigit(ch)));
       ch = lexer_get_char(scanner -> lexer)) {
    lexer_push(scanner -> lexer);
  }
  if (scanner -> lexer -> token &&
      str_len(scanner -> lexer -> token) &&
      config -> foldcase) {
      str_forcecase(scanner -> lexer -> token, config -> foldcase == IDFoldToUpper);
  }
  return lexer_accept(scanner -> lexer, TokenCodeIdentifier);
}

/* -- I D E N T I F I E R  S C A N N E R ---------------------------------- */

void identifier_register(void) {
  lexer_init();
  IDScannerConfig = typedescr_create_and_register(IDScannerConfig,
                                                  "identifier",
                                                  _vtable_idscanner_config,
                                                  NULL);
  typedescr_assign_inheritance(typedescr_get(IDScannerConfig), ScannerConfig);
  scanner_config_register(typedescr_get(IDScannerConfig));
}
