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

#define PARAM_KEYWORD      "keyword"
#define PARAM_KEYWORDS     "keywords"
#define PARAM_NUM_KEYWORDS "num_keywords"

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
  int       num_keywords;
  int       size;
  size_t    maxlen;
  token_t **keywords;
} kw_config_t;

typedef struct _kw_scanner {
  int      matchcount;
  int      match_min;
  int      match_max;
  token_t *token;
  char    *scanned;
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
static kw_config_t *  _kw_config_create(kw_config_t *, va_list);
static void           _kw_config_free(kw_config_t *);
static data_t *       _kw_config_resolve(kw_config_t *, char *);
static kw_config_t *  _kw_config_set(kw_config_t *, char *, data_t *);
static kw_config_t *  _kw_config_mth_dump(kw_config_t *, char *, array_t *, dict_t *);

static kw_config_t *  _kw_config_configure(kw_config_t *, data_t *);
static kw_config_t *  _kw_config_add_keyword(kw_config_t *, token_t *);
static kw_config_t *  _kw_config_dump(kw_config_t *);
static kw_config_t *  _kw_config_dump_tostream(kw_config_t *, FILE *);

static kw_scanner_t * _kw_scanner_create(kw_config_t *);
static void           _kw_scanner_free(kw_scanner_t *);
static kw_scanner_t * _kw_scanner_scan(scanner_t *);

static kw_scanner_t * _kw_scanner_match(scanner_t *, int ch);
static kw_scanner_t * _kw_scanner_reset(scanner_t *);
static token_t *      _kw_match(scanner_t *);

static vtable_t _vtable_kwscanner_config[] = {
  { .id = FunctionNew,            .fnc = (void_t) _kw_config_create },
  { .id = FunctionFree,           .fnc = (void_t) _kw_config_free },
  { .id = FunctionResolve,        .fnc = (void_t) _kw_config_resolve },
  { .id = FunctionSet,            .fnc = (void_t) _kw_config_set },
  { .id = FunctionMatch,          .fnc = (void_t) _kw_match },
  { .id = FunctionDestroyScanner, .fnc = (void_t) _kw_scanner_free },
  { .id = FunctionDump,           .fnc = (void_t) _kw_config_dump },
  { .id = FunctionGetConfig,      .fnc = NULL /* (void_t) _kw_config_config */ },
  { .id = FunctionNone,           .fnc = NULL }
};

static methoddescr_t _methoddescr_kwscanner_config[] = {
  { .type = String, .name = "dump", .method = (method_t ) _kw_config_mth_dump, .argtypes = { NoType, NoType, NoType}, .minargs = 0, .varargs = 0},
  { .type = NoType, .name = NULL, .method = NULL, .argtypes = { NoType, NoType, NoType}, .minargs = 0, .varargs = 0}
};

static int   KWScannerConfig = -1;

/* -- K W _ S C A N N E R _ C O N F I G ----------------------------------- */

kw_config_t * _kw_config_create(kw_config_t *config, va_list args) {
  config -> _sc.priority = 10;
  config -> keywords = NULL;
  config -> num_keywords = 0;
  config -> size = 0;
  config -> maxlen = 0;
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
  int     ix;
  data_t *keywords;

  if (!strcmp(name, PARAM_NUM_KEYWORDS)) {
    return int_to_data(config -> num_keywords);
  } else if (!strcmp(name, PARAM_KEYWORDS)) {
    keywords = data_create_list(NULL);
    for (ix = 0; ix < config -> num_keywords; ix++) {
      data_list_push(keywords, (data_t *) token_copy(config -> keywords[ix]));
    }
    return keywords;
  } else {
    for (ix = 0; ix < config -> num_keywords; ix++) {
      if (!strcmp(token_token(config -> keywords[ix]), name)) {
        return (data_t *) token_copy(config -> keywords[ix]);
      }
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

  debug(lexer, "_kw_config_configure('%s')", data_encode(data));
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
  int newsz;
  int slot;

  if (config -> num_keywords == config -> size) {
    newsz = (config -> size) ? (2 * config -> size) : 4;
    config -> keywords = resize_ptrarray(config -> keywords,
                                         newsz,
                                         config -> size);
    config -> size = newsz;
  }
  debug(lexer, "Adding keyword '%s', num_keywords: %d", token_token(token), config -> num_keywords);
  for (slot = 0;
       (slot < config -> num_keywords) &&
         (strcmp(token_token(config -> keywords[slot]), token_token(token)) < 0);
       slot++) {
    debug(lexer, "slot: %d keyword: '%s'", slot, token_token(config -> keywords[slot]));
  }
  debug(lexer, "Going to use slot %d", slot);
  if (slot < config -> num_keywords) {
    debug(lexer, "Currently occupied by '%s' - %d", token_token(config -> keywords[slot]));
    if (!strcmp(token_token(config -> keywords[slot]), token_token(token))) {
      debug(lexer, "Duplicate keyword '%s'", token_token(token));
      if (token_code(config -> keywords[slot]) != token_code(token)) {
        error("Attempt to register duplicate keyword '%s' with conflicting codes",
              token_token(token));
        config = NULL;
      }
      return config;
    }
    memmove(config -> keywords + (slot + 1), config -> keywords + slot,
            (config -> num_keywords - slot) * sizeof(token_t *));
  }
  config -> keywords[slot] = token_copy(token);
  if (strlen(token_token(token)) > config -> maxlen) {
    config -> maxlen = strlen(token_token(token));
  }
  config -> num_keywords++;
  debug(lexer, "Added keyword '%s' - slot: %d num_keywords: %d size: %d",
                token_tostring(config -> keywords[slot]),
                slot, config -> num_keywords, config -> size);
  return config;
}

kw_config_t * _kw_config_mth_dump(kw_config_t *self, char *name, array_t *args, dict_t *kwargs) {
  return _kw_config_dump_tostream(self, stderr);
}

kw_config_t * _kw_config_dump(kw_config_t *config) {
  return _kw_config_dump_tostream(config, stdout);
}

kw_config_t * _kw_config_dump_tostream(kw_config_t *config, FILE *stream) {
  int      ix;
  token_t *t;
  char    *escaped;

  debug(lexer, "_kw_config_dump_tostream");
  if (config -> num_keywords > 0) {
    fprintf(stream, "  { /* Configure keyword scanner with keywords */\n");
    fprintf(stream, "    token_t *token\n\n");
    for (ix = 0; ix < config -> num_keywords; ix++) {
      t = config -> keywords[ix];
      escaped = c_escape(token_token(t));
      fprintf(stream, "    token = token_create(%d, \"%s\");\n", token_code(t), escaped);
      free(escaped);
      fprintf(stream, "    scanner_config_setvalue(scanner_config, \"" PARAM_KEYWORD "\", token);\n");
      fprintf(stream, "    token_free(token);\n");
    }
    fprintf(stream, "  }\n");
  }
  return config;
}

/* -- K W _ S C A N N E R ------------------------------------------------- */

kw_scanner_t * _kw_scanner_create(kw_config_t *config) {
  kw_scanner_t *ret = NEW(kw_scanner_t);

  ret -> matchcount = ret -> match_min = ret -> match_max = 0;
  ret -> token = NULL;
  if (config -> maxlen) {
    ret -> scanned = stralloc(config -> maxlen + 2);
  }
  return ret;
}

void _kw_scanner_free(kw_scanner_t *kw_scanner) {
  if (kw_scanner) {
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
  int                 cmp;
  char               *kw;

  if (!config -> num_keywords) {
    scanner -> state = KSSNoMatch;
    return kw_scanner;
  }

  if (state == KSSInit) {
    kw_scanner -> match_min = 0;
    kw_scanner -> match_max = config -> num_keywords;
    kw_scanner -> scanned[0] = 0;
  }
  len = strlen(kw_scanner -> scanned);
  kw_scanner -> scanned[len++] = ch;
  kw_scanner -> scanned[len] = 0;

  for (ix = kw_scanner -> match_min; ix < kw_scanner -> match_max; ix++) {
    kw = token_token(config -> keywords[ix]);
    cmp = strcmp(kw, kw_scanner -> scanned);
    if (cmp < 0) {
      kw_scanner -> match_min = ix + 1;
    } else if (cmp == 0) {
      kw_scanner -> token = config -> keywords[ix];
    } else { /* cmp > 0 */
      if (strncmp(kw_scanner -> scanned, kw, len)) {
        kw_scanner -> match_max = ix;
        break;
      }
    }
  }
  kw_scanner -> matchcount = kw_scanner -> match_max - kw_scanner -> match_min;
  if (kw_scanner -> matchcount < 0) {
    kw_scanner -> matchcount = 0;
  }
  debug(lexer, "_kw_scanner_match: scanned: %s matchcount: %d match_min: %d, match_max: %d",
               kw_scanner -> scanned, kw_scanner -> matchcount, kw_scanner -> match_min,
               kw_scanner -> match_max);

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

  KWScannerConfig = typedescr_create_and_register(
      KWScannerConfig, "keyword", _vtable_kwscanner_config, _methoddescr_kwscanner_config);
  typedescr_set_size(KWScannerConfig, kw_config_t);
  ret = typedescr_get(KWScannerConfig);
  return ret;
}
