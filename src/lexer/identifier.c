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

#include "liblexer.h"
#include <exception.h>
#include <nvp.h>

#define PARAM_STARTWITH       "startwith"
#define PARAM_FILTER          "filter"
#define PARAM_TOKENCODE       "tokencode"

typedef enum _id_charclass {
  IDCaseSensitive = 'X',
  IDFoldToLower = 'l',
  IDOnlyLower = 'a',
  IDFoldToUpper = 'U',
  IDOnlyUpper = 'A',
  IDNoAlpha = 'Q',
  IDDigits = '9'
} id_charclass_t;

#define ALL_ALPHA_CLASSES  "XlUAaQ"

static code_label_t charclass_labels[] = {
  { IDCaseSensitive, "casesensitive" },
  { IDFoldToLower,   "tolower" },
  { IDOnlyLower,     "onlylower" },
  { IDFoldToUpper,   "toupper" },
  { IDOnlyUpper,     "onlyupper" },
  { IDNoAlpha,       "noalpha" },
  { IDDigits,        "digits"},
  { -1,              NULL }
};

typedef struct _id_config {
  scanner_config_t  _sc;
  token_code_t      code;
  char             *filter;
  char             *startwith;
  int               alpha;
  int               digits;
  int               startwith_alpha;
  int               startwith_digits;
} id_config_t;

static id_config_t * _id_config_create(id_config_t *config, va_list args);
static data_t *      _id_config_set(id_config_t *, char *, data_t *);
static data_t *      _id_config_resolve(id_config_t *, char *);
static id_config_t * _id_config_config(id_config_t *, array_t *);
static int           _id_config_filter(id_config_t *, str_t *, int);
static token_t *     _id_match(scanner_t *);

static vtable_t _vtable_idscanner_config[] = {
  { .id = FunctionNew,       .fnc = (void_t) _id_config_create },
  { .id = FunctionResolve,   .fnc = (void_t) _id_config_resolve },
  { .id = FunctionSet,       .fnc = (void_t) _id_config_set },
  { .id = FunctionMatch,     .fnc = (void_t) _id_match },
  { .id = FunctionGetConfig, .fnc = (void_t) _id_config_config },
  { .id = FunctionNone,      .fnc = NULL }
};

static int IDScannerConfig = -1;

/* -- I D _ C O N F I G --------------------------------------------------- */

id_config_t * _id_config_create(id_config_t *config, va_list args) {
  config -> code = TokenCodeIdentifier;
  config -> startwith = strdup("X_");
  config -> filter = strdup("X9_");
  config -> alpha = IDCaseSensitive;
  config -> startwith_alpha = IDCaseSensitive;
  config -> digits = TRUE;
  config -> startwith_digits = FALSE;
  debug(lexer, "_id_config_create - match: %p", config -> _sc.match);
  return config;
}

data_t * _id_config_set(id_config_t *id_config, char *name, data_t *value) {
  data_t *ret = (data_t *) id_config;
  char   *ptr;
  int     tokencode;

  if (!strcmp(name, PARAM_STARTWITH)) {
    id_config -> startwith = strdup(data_tostring(value));
    if (!*id_config -> startwith) {
      free(id_config -> startwith);
      id_config -> startwith = NULL;
      id_config -> startwith_alpha = IDCaseSensitive;
      id_config -> startwith_digits = TRUE;
    } else {
      ptr = strpbrk(id_config -> startwith, ALL_ALPHA_CLASSES);
      if (ptr) {
        id_config -> startwith_alpha = *ptr;
      } else {
        id_config -> startwith_alpha = IDNoAlpha;
      }
      id_config -> startwith_digits = strchr(id_config -> startwith, '9') != NULL;
    }
  } else if (!strcmp(name, PARAM_FILTER)) {
    id_config -> filter = strdup(data_tostring(value));
    if (!*id_config -> filter) {
      free(id_config -> filter);
      id_config -> filter = NULL;
      id_config -> alpha = IDCaseSensitive;
      id_config -> digits = TRUE;
    } else {
      ptr = strpbrk(id_config -> filter, ALL_ALPHA_CLASSES);
      if (ptr) {
        id_config -> alpha = *ptr;
      } else {
        id_config -> alpha = IDNoAlpha;
      }
      id_config -> digits = strchr(id_config -> filter, '9') != NULL;
    }
  } else if (!strcmp(name, PARAM_TOKENCODE)) {
    tokencode = data_intval(value);
    if (!tokencode && (strlen(data_tostring(value)) == 1)) {
      tokencode = *(data_tostring(value));
    }
    if (tokencode) {
      id_config -> code = tokencode;
    } else {
      ret = data_exception(ErrorParameterValue, "Invalid tokencode value '%s'",
                           data_tostring(value));
    }
  } else {
    ret = NULL;
  }
  return ret;
}

data_t * _id_config_resolve(id_config_t *id_config, char *name) {
  if (!strcmp(name, PARAM_STARTWITH)) {
    return (data_t *) str_copy_chars(id_config -> startwith ? id_config -> startwith : "");
  } else if (!strcmp(name, PARAM_FILTER)) {
    return (data_t *) str_copy_chars(id_config -> filter ? id_config -> filter : "");
  } else if (!strcmp(name, PARAM_TOKENCODE)) {
    return (data_t *) int_to_data(id_config -> code);
  } else {
    return NULL;
  }
}

id_config_t * _id_config_config(id_config_t *config, array_t *cfg) {
  array_push(cfg, nvp_create(
    str_to_data(PARAM_FILTER),
    (data_t *) str_copy_chars(config -> filter ? config -> filter : "")));
  array_push(cfg, nvp_create(
    str_to_data(PARAM_STARTWITH),
    (data_t *) str_copy_chars(config -> startwith ? config -> startwith : "")));
  array_push(cfg, nvp_create(str_to_data(PARAM_TOKENCODE), int_to_data(config -> code)));
  return config;
}

int _id_config_filter_against(char *filter, int alpha, int digits, int ch) {
  if (isalpha(ch)) {
    switch (alpha) {
      case IDNoAlpha:
        return FALSE;
      case IDOnlyLower:
        return islower(ch);
      case IDOnlyUpper:
        return isupper(ch);
    }
  } else if (isdigit(ch)) {
    return digits;
  } else if (filter) {
    return strchr(filter, ch) != NULL;
  }
  return TRUE;
}

int _id_config_filter(id_config_t *config, str_t *token, int ch) {
  int ret;

  if (!ch) {
    return FALSE;
  }
  ret = _id_config_filter_against(
    config -> filter, config -> alpha, config -> digits, ch);
  if (ret && !str_len(token)) {
    ret = _id_config_filter_against(
      config -> startwith, config -> startwith_alpha,
      config -> startwith_digits, ch);
  }
  return ret;
}

token_t * _id_match(scanner_t *scanner) {
  int          ch;
  id_config_t *config = (id_config_t *) scanner -> config;

  debug(lexer, "_id_match");
  for (ch = lexer_get_char(scanner -> lexer);
       _id_config_filter(config, scanner -> lexer -> token, ch);
       ch = lexer_get_char(scanner -> lexer)) {
    debug(lexer, "_id_match(%c)", ch);
    switch (config -> alpha) {
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

__DLL_EXPORT__ typedescr_t * identifier_register(void) {
  typedescr_t *ret;

  IDScannerConfig = typedescr_create_and_register(IDScannerConfig,
                                                  "identifier",
                                                  _vtable_idscanner_config,
                                                  NULL);
  typedescr_set_size(IDScannerConfig, id_config_t);
  ret = typedescr_get(IDScannerConfig);
  return ret;
}
