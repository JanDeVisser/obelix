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
#include <stdio.h>
#include <ctype.h>

#include <exception.h>
#include <lexer.h>
#include <nvp.h>

#define PARAM_TOUPPER         "toupper"
#define PARAM_ONLYUPPER       "onlyupper"
#define PARAM_TOLOWER         "tolower"
#define PARAM_ONLYLOWER       "onlylower"
#define PARAM_CASESENSITIVE   "casesensitive"
#define PARAM_FOLD            "fold"
#define PARAM_DIGITS          "digits"
#define PARAM_UNDERSCORE      "underscore"
#define PARAM_TOKENCODE       "tokencode"

typedef enum _id_foldcase {
  IDCaseSensitive = 0,
  IDFoldToLower,
  IDOnlyLower,
  IDFoldToUpper,
  IDOnlyUpper
} id_foldcase_t;

static code_label_t foldcase_labels[] = {
  { IDCaseSensitive, "casesensitive" },
  { IDFoldToLower,   "tolower" },
  { IDOnlyLower,     "onlylower" },
  { IDFoldToUpper,   "toupper" },
  { IDOnlyUpper,     "onlyupper" },
  { -1,              NULL }
};

typedef struct _id_config {
  scanner_config_t  _sc;
  token_code_t      code;
  int               underscore;
  int               digits;
  id_foldcase_t     foldcase;
} id_config_t;

static id_config_t * _id_config_create(id_config_t *config, va_list args);
static data_t *      _id_config_set(id_config_t *, char *, data_t *);
static data_t *      _id_config_resolve(id_config_t *, char *);
static id_config_t * _id_config_config(id_config_t *, array_t *);
static token_t *     _id_match(scanner_t *);

static vtable_t _vtable_idscanner_config[] = {
  { .id = FunctionNew,     .fnc = (void_t) _id_config_create },
  { .id = FunctionResolve, .fnc = (void_t) _id_config_resolve },
  { .id = FunctionSet,     .fnc = (void_t) _id_config_set },
  { .id = FunctionUsr1,    .fnc = (void_t) _id_match },
  { .id = FunctionUsr2,    .fnc = NULL },
  { .id = FunctionUsr4,    .fnc = (void_t) _id_config_config },
  { .id = FunctionNone,    .fnc = NULL }
};

static int IDScannerConfig = -1;

/* -- I D _ C O N F I G --------------------------------------------------- */

id_config_t * _id_config_create(id_config_t *config, va_list args) {
  config -> code = TokenCodeIdentifier;
  config -> underscore = TRUE;
  config -> digits = TRUE;
  config -> foldcase = IDCaseSensitive;
  if (lexer_debug) {
    debug("_id_config_create - match: %p", config -> _sc.match);
  }
  return config;
}

data_t * _id_config_set(id_config_t *id_config, char *name, data_t *value) {
  data_t        *ret = (data_t *) id_config;
  id_foldcase_t  code;

  if (!strcmp(name, PARAM_TOUPPER)) {
    id_config -> foldcase = data_intval(value) ? IDFoldToUpper : IDCaseSensitive;
  } else if (!strcmp(name, PARAM_ONLYUPPER)) {
    id_config -> foldcase = data_intval(value) ? IDOnlyUpper : IDCaseSensitive;
  } else if (!strcmp(name, PARAM_TOLOWER)) {
    id_config -> foldcase = data_intval(value) ? IDFoldToLower : IDCaseSensitive;
  } else if (!strcmp(name, PARAM_ONLYLOWER)) {
    id_config -> foldcase = data_intval(value) ? IDOnlyLower : IDCaseSensitive;
  } else if (!strcmp(name, PARAM_CASESENSITIVE) && data_intval(value)) {
    id_config -> foldcase = IDCaseSensitive;
  } else if (!strcmp(name, PARAM_DIGITS)) {
    id_config -> digits = data_intval(value);
  } else if (!strcmp(name, PARAM_UNDERSCORE)) {
    id_config -> underscore = data_intval(value);
  } else if (!strcmp(name, PARAM_FOLD)) {
    code = code_for_label(foldcase_labels, data_tostring(value));
    if ((int) code < 0) {
      ret = data_exception(ErrorParameterValue, "Invalid foldcase value '%s'",
                           data_tostring(value));
    } else {
      id_config -> foldcase = code;
    }
  } else if (!strcmp(name, PARAM_TOKENCODE)) {
    id_config -> code = data_intval(value);
  } else {
    ret = NULL;
  }
  return ret;
}

data_t * _id_config_resolve(id_config_t *id_config, char *name) {
  if (!strcmp(name, PARAM_TOUPPER)) {
    return (data_t *) bool_get(id_config -> foldcase == IDFoldToUpper);
  } else if (!strcmp(name, PARAM_ONLYUPPER)) {
    return (data_t *) bool_get(id_config -> foldcase == IDOnlyUpper);
  } else if (!strcmp(name, PARAM_TOLOWER)) {
    return (data_t *) bool_get(id_config -> foldcase == IDFoldToLower);
  } else if (!strcmp(name, PARAM_ONLYLOWER)) {
    return (data_t *) bool_get(id_config -> foldcase == IDOnlyLower);
  } else if (!strcmp(name, PARAM_CASESENSITIVE)) {
    return (data_t *) bool_get(id_config -> foldcase == IDCaseSensitive);
  } else if (!strcmp(name, PARAM_DIGITS)) {
    return (data_t *) bool_get(id_config -> digits);
  } else if (!strcmp(name, PARAM_UNDERSCORE)) {
    return (data_t *) bool_get(id_config -> underscore);
  } else if (!strcmp(name, PARAM_FOLD)) {
    return (data_t *) str_wrap(label_for_code(foldcase_labels, id_config -> foldcase));
  } else if (!strcmp(name, PARAM_TOKENCODE)) {
    return (data_t *) int_to_data(id_config -> code);
  } else {
    return NULL;
  }
}

id_config_t * _id_config_config(id_config_t *config, array_t *cfg) {
  array_push(cfg, nvp_create(str_to_data(PARAM_FOLD),
                             str_to_data(label_for_code(foldcase_labels, config -> foldcase))));
  array_push(cfg, nvp_create(str_to_data(PARAM_DIGITS), (data_t *) bool_get(config -> digits)));
  array_push(cfg, nvp_create(str_to_data(PARAM_UNDERSCORE), (data_t *) bool_get(config -> underscore)));
  array_push(cfg, nvp_create(str_to_data(PARAM_TOKENCODE), int_to_data(config -> code)));
  return config;
}

token_t * _id_match(scanner_t *scanner) {
  int          ch;
  id_config_t *config = (id_config_t *) scanner -> config;

  mdebug(lexer, "_id_match");
  for (ch = lexer_get_char(scanner -> lexer);
       ch && ((isalpha(ch) &&
                ((config -> foldcase != IDOnlyUpper) || isupper(ch)) &&
                ((config -> foldcase != IDOnlyLower) || islower(ch))) ||
              (config -> underscore && (ch == '_')) ||
              (config -> digits && isdigit(ch) && str_len(scanner -> lexer -> token)));
       ch = lexer_get_char(scanner -> lexer)) {
    mdebug(lexer, "_id_match(%c)", ch);
    switch (config -> foldcase) {
      case IDCaseSensitive:
      case IDOnlyLower:
      case IDOnlyUpper:
        lexer_push(scanner -> lexer);
        break;
      case IDFoldToUpper:
        lexer_push_as(scanner -> lexer, toupper(ch));
        break;
      case IDFoldToLower:
        lexer_push_as(scanner -> lexer, tolower(ch));
        break;
    }
  }
  return lexer_accept(scanner -> lexer, config -> code);
}

/* -- I D E N T I F I E R  S C A N N E R ---------------------------------- */

typedescr_t * identifier_register(void) {
  typedescr_t *ret;

  IDScannerConfig = typedescr_create_and_register(IDScannerConfig,
                                                  "identifier",
                                                  _vtable_idscanner_config,
                                                  NULL);
  ret = typedescr_get(IDScannerConfig);
  typedescr_set_size(IDScannerConfig, id_config_t);
  return ret;
}
