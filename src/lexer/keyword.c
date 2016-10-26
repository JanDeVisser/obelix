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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "liblexer.h"
#include <nvp.h>

#define PARAM_KEYWORD   "keyword"

typedef enum _kw_scanner_state {
  KSSInit,
  KSSPrefixMatched,
  KSSPrefixesMatched,
  KSSFullMatch,
  KSSFullMatchAndPrefixes,
  KSSFullMatchLost,
  KSSPrefixMatchLost,
  KSSNoMatch
} kw_scanner_state_t;

typedef struct _kw_config {
  scanner_config_t   _sc;
  int                num_keywords;
  token_t          **keywords;
} kw_config_t;

typedef struct _kw_scanner {
  int                  matchcount;
  token_t            **matches;
  token_t             *token;
  char                *scanned;
} kw_scanner_t;

static code_label_t _scanner_state_names[] = {
  { .code = KSSInit,                 .label = "KSSInit" },
  { .code = KSSPrefixMatched,        .label = "KSSPrefixMatched" },
  { .code = KSSPrefixesMatched,      .label = "KSSPrefixesMatched" },
  { .code = KSSFullMatch,            .label = "KSSFullMatch" },
  { .code = KSSFullMatchAndPrefixes, .label = "KSSFullMatchAndPrefixes" },
  { .code = KSSFullMatchLost,        .label = "KSSFullMatchLost" },
  { .code = KSSPrefixMatchLost,      .label = "KSSPrefixMatchLost" },
  { .code = KSSNoMatch,              .label = "KSSNoMatch" },
  { .code = -1,                      .label = NULL }
};

static kw_config_t *   _kw_config_create(kw_config_t *, va_list);
static void            _kw_config_free(kw_config_t *);
static data_t *        _kw_config_resolve(kw_config_t *, char *);
static kw_config_t *   _kw_config_set(kw_config_t *, char *, data_t *);
static kw_config_t *   _kw_config_configure(kw_config_t *, data_t *);
static kw_config_t *   _kw_config_add_keyword(kw_config_t *, token_t *);
static kw_config_t *   _kw_config_dump(kw_config_t *);

static kw_scanner_t *  _kw_scanner_create(kw_config_t *);
static void            _kw_scanner_free(kw_scanner_t *);
static kw_scanner_t *  _kw_scanner_scan(scanner_t *);

static kw_scanner_t *  _kw_scanner_match(scanner_t *, int ch);
static kw_scanner_t *  _kw_scanner_reset(scanner_t *);
static token_t *       _kw_match(scanner_t *);

static vtable_t _vtable_kwscanner_config[] = {
  { .id = FunctionNew,            .fnc = (void_t) _kw_config_create },
  { .id = FunctionFree,           .fnc = (void_t) _kw_config_free },
  { .id = FunctionResolve,        .fnc = (void_t) _kw_config_resolve },
  { .id = FunctionSet,            .fnc = (void_t) _kw_config_set },
  { .id = FunctionMatch,          .fnc = (void_t) _kw_match },
  { .id = FunctionDestroyScanner, .fnc = (void_t) _kw_scanner_free },
  { .id = FunctionGetConfig,      .fnc = NULL /* (void_t) _kw_config_config */ },
  { .id = FunctionDump,           .fnc = (void_t) _kw_config_dump },
  { .id = FunctionNone,           .fnc = NULL }
};

static int   KWScannerConfig = -1;

/* -- K W _ S C A N N E R _ C O N F I G ----------------------------------- */

kw_config_t * _kw_config_create(kw_config_t *config, va_list args) {
  config -> _sc.priority = 10;
  config -> keywords = NULL;
  config -> num_keywords = 0;
  return config;
}

void _kw_config_free(kw_config_t *config) {
  int ix;

  if (config && config -> keywords) {
    for (ix = 0; ix < config -> num_keywords; ix++) {
      token_free(config -> keywords[ix]);
    }
    free(config -> keywords);
  }
}

kw_config_t * _kw_config_set(kw_config_t *config, char *name, data_t *value) {
  if (!strcmp(PARAM_KEYWORD, name)) {
    return _kw_config_configure(config, value);
  } else {
    return NULL;
  }
}

data_t * _kw_config_resolve(kw_config_t *config, char *name) {
  int ix;

  for (ix = 0; ix < config -> num_keywords; ix++) {
    if (!strcmp(token_token(config -> keywords[ix]), name)) {
      return (data_t *) token_copy(config -> keywords[ix]);
    }
  }
  return NULL;
}

kw_config_t * _kw_config_configure(kw_config_t *config, data_t *data) {
  int                  t = data_type(data);
  nvp_t               *nvp;
  token_t             *token = NULL;
  char                *str;
  unsigned int         code;

  if (t == Token) {
    token = token_copy((token_t *) data);
  } else if (t == NVP) {
    nvp = data_as_nvp(data);
    token = token_create((unsigned int) data_intval(nvp -> value), data_tostring(nvp -> name));
  } else {
    str = data_tostring(data);
    if (str && strlen(str)) {
      token = token_parse(str);
      if (!token) {
        code = strhash(str);
        token = token_create(code, str);
      }
    }
  }
  if (token) {
    _kw_config_add_keyword(config, token);
    token_free(token);
  }
  return config;
}

kw_config_t * _kw_config_add_keyword(kw_config_t *config, token_t *token) {
  config -> keywords = resize_ptrarray(config -> keywords,
                                       config -> num_keywords + 1,
                                       config -> num_keywords);
  config -> keywords[config -> num_keywords++] = token_copy(token);
  debug(lexer, "Added keyword '%s' - num_keywords = %d",
                token_tostring(config -> keywords[config -> num_keywords - 1]),
                config -> num_keywords);
  return config;
}

kw_config_t * _kw_config_dump(kw_config_t *config) {
  int      ix;
  token_t *t;

  if (config -> num_keywords > 0) {
    printf("  { /* Configure keyword scanner with keywords */\n");
    printf("    token_t *token\n\n");
    for (ix = 0; ix < config -> num_keywords; ix++) {
      t = config -> keywords[ix];
      printf("    token = token_create(%d, \"%s\");\n", token_code(t), token_token(t));
      printf("    scanner_config_setvalue(scanner_config, \"" PARAM_KEYWORD "\", token);\n");
      printf("    token_free(token);\n\n");
    }
    printf("  }\n");
  }
  return config;
}

/* -- K W _ S C A N N E R ------------------------------------------------- */

kw_scanner_t * _kw_scanner_create(kw_config_t *config) {
  kw_scanner_t *ret = NEW(kw_scanner_t);
  size_t        maxlen = 0;
  int           ix;
  size_t        len;

  ret -> matchcount = 0;
  ret -> matches = NULL;
  ret -> token = NULL;
  for (ix = 0; ix < config -> num_keywords; ix++) {
    len = strlen(token_token(config -> keywords[ix]));
    if (len > maxlen) {
      maxlen = len;
    }
  }
  if (maxlen) {
    ret -> scanned = (char *) _new(maxlen + 1);
  }
  return ret;
}

void _kw_scanner_free(kw_scanner_t *kw_scanner) {
  if (kw_scanner) {
    if (kw_scanner -> matches) {
      free(kw_scanner -> matches);
    }
    free(kw_scanner -> scanned);
    free(kw_scanner);
  }
}

kw_scanner_t * _kw_scanner_match(scanner_t *scanner, int ch) {
  kw_scanner_state_t  state = (kw_scanner_state_t) scanner -> state;
  kw_config_t        *config = (kw_config_t *) scanner -> config;
  kw_scanner_t       *kw_scanner = (kw_scanner_t *) scanner -> data;
  int                 len;
  int                 ix;
  char               *kw;

  if (!config -> num_keywords) {
    scanner -> state = KSSNoMatch;
    return kw_scanner;
  }

  if (!kw_scanner -> matches) {
    kw_scanner -> matches = NEWARR(config -> num_keywords, token_t *);
  }
  if (state == KSSInit) {
    memcpy(kw_scanner -> matches, config -> keywords,
           config -> num_keywords * sizeof(token_t *));
    kw_scanner -> matchcount = config -> num_keywords;
    kw_scanner -> scanned[0] = 0;
  }
  len = strlen(kw_scanner -> scanned);
  kw_scanner -> scanned[len++] = ch;
  kw_scanner -> scanned[len] = 0;

  /*
   * kw_scanner -> matches is an array holding all the matched tokens
   * previously found, or all of the keywords if this is the match attempt.
   * It is initialized as a shallow copy of the entries in config -> keywords.
   * The current token is matched against all non-zero tokens in the array,
   * and when the current token doesn't match the keyword (anymore), the entry
   * is zeroed out and the matchcount is decremented. If the current token
   * (still) matches the entry, and the is exactly equal to the keyword,
   * kw_scanner -> token is set to that entry.
   */
  for (ix = 0; ix < config -> num_keywords; ix++) {
    if (!kw_scanner -> matches[ix]) {
      continue;
    }

    kw = token_token(kw_scanner -> matches[ix]);
    if ((len <= strlen(kw)) && !strncmp(kw_scanner -> scanned, kw, len)) {
      if (len == strlen(kw)) {
        kw_scanner -> token = kw_scanner -> matches[ix];
      }
    } else {
      kw_scanner -> matches[ix] = NULL;
      kw_scanner -> matchcount--;
    }
  }

  /*
   * Determine new state.
   */
  switch (kw_scanner -> matchcount) {
    case 0:
      /*
       * No matches. This means that either there wasn't any match at all, or
       * we lost the match.
       */
      switch (state) {
        case KSSFullMatchAndPrefixes:
        case KSSFullMatch:

          /*
           * We had a full match (and maybe some additional prefix matches to)
           * but now lost it or all of them:
           */
          scanner -> state = KSSFullMatchLost;
          break;

        case KSSPrefixesMatched:
        case KSSPrefixMatched:

          /*
           * We had one or more prefix matches, but lost it or all of them:
           */
          scanner -> state = KSSPrefixMatchLost;
          break;

        default:

          /*
           * No match at all.
           */
          scanner -> state = KSSNoMatch;
          break;
      }
      break;
    case 1:

      /*
       * Only one match. If it's a full match, i.e. the token matches the
       * keyword, we have a full match. Otherwise it's a prefix match.
       */
      scanner -> state = (kw_scanner -> token)
        ? KSSFullMatch
        : KSSPrefixMatched;
      break;

    default: /* kw_scanner -> matchcount > 1 */

      /*
       * More than one match. If one of them is a full match, i.e. the token
       * matches exactly the keyword, it's a full-and-prefix match, otherwise
       * it's a prefixes-match.
       */
      scanner -> state = (kw_scanner -> token)
        ? KSSFullMatchAndPrefixes
        : KSSPrefixesMatched;
      break;

  }
  return kw_scanner;
}

kw_scanner_t * _kw_scanner_reset(scanner_t *scanner) {
  kw_scanner_t *kw_scanner = (kw_scanner_t *) scanner -> data;

  scanner -> state = KSSInit;
  kw_scanner -> matchcount = 0;
  kw_scanner -> token = NULL;
  return kw_scanner;
}

kw_scanner_t * _kw_scanner_scan(scanner_t *scanner) {
  kw_scanner_t *kw_scanner = (kw_scanner_t *) scanner -> data;
  int           carry_on;
  int           ch;

  _kw_scanner_reset(scanner);
  carry_on = 1;
  for (ch = lexer_get_char(scanner -> lexer);
       ch && carry_on;
       ch = lexer_get_char(scanner -> lexer)) {
    _kw_scanner_match(scanner, ch);

    carry_on = 0;
    switch (scanner -> state) {
      case KSSNoMatch:
        break;

      case KSSFullMatch:
      case KSSFullMatchAndPrefixes:
      case KSSPrefixesMatched:
      case KSSPrefixMatched:
        carry_on = 1;
        break;

      case KSSPrefixMatchLost:
        /*
         * We lost the match, but there was never a full match.
         */
        scanner -> state = KSSNoMatch;
        break;

      case KSSFullMatchLost:
        break;
    }
    if (carry_on) {
      lexer_push(scanner -> lexer);
    }
  };
  return kw_scanner;
}

token_t * _kw_match(scanner_t *scanner) {
  kw_config_t  *config = (kw_config_t *) scanner -> config;
  kw_scanner_t *kw_scanner = (kw_scanner_t *) scanner -> data;

  if (!config -> num_keywords) {
    debug(lexer, "No keywords...");
    return NULL;
  }
  if (!kw_scanner) {
    kw_scanner = _kw_scanner_create(config);
    scanner -> data = kw_scanner;
  }
  _kw_scanner_scan(scanner);
  debug(lexer, "_kw_match returns '%s' (%d)",
                label_for_code((code_label_t *) _scanner_state_names, scanner -> state),
                scanner -> state);
  if ((scanner -> state == KSSFullMatchLost) || (scanner -> state == KSSFullMatch)) {
    lexer_accept(scanner -> lexer, token_code(kw_scanner -> token));
    return token_copy(kw_scanner -> token);
  } else {
    return NULL;
  }
}

typedescr_t * keyword_register(void) {
  typedescr_t *ret;

  debug(lexer, "keyword_register");
  KWScannerConfig = typedescr_create_and_register(
      KWScannerConfig, "keyword", _vtable_kwscanner_config, NULL);
  typedescr_set_size(KWScannerConfig, kw_config_t);
  ret = typedescr_get(KWScannerConfig);
  debug(lexer, "keyword_register: %d %d", KWScannerConfig, ret -> type);
  return ret;
}
