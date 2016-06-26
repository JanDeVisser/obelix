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

#include <lexer.h>
#include <nvp.h>

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

typedef struct _kw_scanner_config {
  scanner_config_t   _sc;
  int                num_keywords;
  token_t          **keywords;
} kw_scanner_config_t;

typedef struct _kw_scanner {
  kw_scanner_config_t *config;
  int                  matchcount;
  token_t            **matches;
  token_t             *token;
  kw_scanner_state_t   state;
} kw_scanner_t;

static code_label_t scanner_state_names[] = {
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

static kw_scanner_config_t * _kw_scanner_config_create(scanner_config_t *);
static void                  _kw_scanner_config_free(kw_scanner_config_t *);
static kw_scanner_config_t * _kw_scanner_config_configure(kw_scanner_config_t *, data_t *);
static kw_scanner_config_t * _kw_scanner_config_add_keyword(kw_scanner_config_t *, token_t *);

static kw_scanner_t *        _kw_scanner_create(kw_scanner_config_t *, scanner_t *);
static void                  _kw_scanner_free(kw_scanner_t *);
static kw_scanner_t *        _kw_scanner_match(kw_scanner_t *, str_t *);
static kw_scanner_t *        _kw_scanner_reset(kw_scanner_t *);
static kw_scanner_t *        _kw_scanner_scan(kw_scanner_t *, lexer_t *);

static token_t *             _keywords_match(scanner_t *);
static token_t *             _keywords_match_2nd_pass(scanner_t *);

static vtable_t _kw_scanner_config_vtable[] = {
  { .id = FunctionNew,          .fnc = (void_t) _kw_scanner_config_create },
  { .id = FunctionFree,         .fnc = (void_t) _kw_scanner_config_free },
  { .id = FunctionResolve,      .fnc = (void_t) _kw_scanner_config_resolve },
  { .id = FunctionSet,          .fnc = (void_t) _kw_scanner_config_set },
  { .id = FunctionUsr1,         .fnc = (void_t) _kw_scanner_create },
  { .id = FunctionNone,         .fnc = NULL }
};

static int   KWScannerConfig;
static int   KWScanner;

/* -- K W _ S C A N N E R _ C O N F I G ----------------------------------- */

kw_scanner_config_t * _kw_scanner_config_create(scanner_config_t *config) {
  kw_scanner_config_t *ret = NEW(kw_scanner_config_t);

  (void) config;
  ret -> keywords = NULL;
  ret -> num_keywords = 0;
  return ret;
}

void _kw_scanner_config_free(kw_scanner_config_t *config) {
  int ix;

  if (config && config -> keywords) {
    for (ix = 0; ix < config -> num_keywords; ix++) {
      token_free(config -> keywords[ix]);
    }
    free(config -> keywords);
  }
}

kw_scanner_config_t * _kw_scanner_config_configure(kw_scanner_config_t *config, data_t *data) {
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
    _kw_scanner_config_add_keyword(config, token);
    token_free(token);
  }
  return config;
}

kw_scanner_config_t * _kw_scanner_config_add_keyword(kw_scanner_config_t *config, token_t *token) {
  config -> keywords = resize_ptrarray(config -> keywords,
                                       config -> num_keywords,
                                       config -> num_keywords + 1);
  config -> keywords[config -> num_keywords++] = token_copy(token);
  return config;
}

/* -- K W _ S C A N N E R ------------------------------------------------- */

kw_scanner_t * _kw_scanner_create(kw_scanner_config_t *config, scanner_t *scanner) {
  kw_scanner_t *ret = NEW(kw_scanner_t);

  (void) scanner;
  ret -> config = config;
  ret -> matchcount = 0;
  ret -> matches = NULL;
  ret -> state = KSSInit;
  ret -> token = NULL;
  return ret;
}

void _kw_scanner_free(kw_scanner_t *kw_scanner) {
  if (kw_scanner) {
    if (kw_scanner -> matches) {
      free(kw_scanner -> matches);
    }
    free(kw_scanner);
  }
}

kw_scanner_t * _kw_scanner_match(kw_scanner_t *kw_scanner, str_t *token) {
  kw_scanner_state_t   state = kw_scanner -> state;
  int                  ix;
  int                  matchcount;
  int                  sz;
  char                *kw;
  token_t            **tmp;

  kw_scanner -> token = NULL;
  if (!kw_scanner -> config -> num_keywords || !str_len(token)) {
    kw_scanner -> state = KSSNoMatch;
    return kw_scanner;
  }

  if (!kw_scanner -> matches) {
    kw_scanner -> matches = NEWARR(kw_scanner -> config -> num_keywords, token_t *);
  }
  if (state == KSSInit) {
    memcpy(kw_scanner -> matches, kw_scanner -> config -> keywords,
           kw_scanner -> config -> num_keywords * sizeof(token_t *));
    kw_scanner -> matchcount = kw_scanner -> config -> num_keywords;
  }

  /*
   * kw_scanner -> matches is an array holding all the matched tokens
   * previously found, or all of the keywords if this is the match attempt.
   * It is initialized as a shallow copy of the entries in
   * kw_scanner -> config -> keywords. The current token is matched against
   * all non-zero tokens in the array, and when the current token doesn't match
   * the keyword (anymore), the entry is zeroed out and the matchcount is
   * decremented. If the current token (still) matches the entry, and the
   * is exactly equal to the keyword, kw_scanner -> token is set to that entry.
   */
  for (ix = 0; ix < kw_scanner -> config -> num_keywords; ix++) {
    if (!kw_scanner -> matches[ix]) {
      continue;
    }

    kw = token_token(kw_scanner->matches[ix]);
    if ((str_len(token) <= strlen(kw)) &&
        !str_ncmp_chars(token, kw, str_len(token))) {
      if (str_len(token) == strlen(kw)) {
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
          kw_scanner -> state = KSSFullMatchLost;
          break;

        case KSSPrefixesMatched:
        case KSSPrefixMatched:

          /*
           * We had one or more prefix matches, but lost it or all of them:
           */
          kw_scanner -> state = KSSPrefixMatchLost;
          break;

        default:

          /*
           * No match at all.
           */
          kw_scanner -> state = KSSNoMatch;
          break;
      }
      break;
    case 1:

      /*
       * Only one match. If it's a full match, i.e. the token matches the
       * keyword, we have a full match. Otherwise it's a prefix match.
       */
      kw_scanner -> state = (kw_scanner -> token)
        ? KSSFullMatch
        : KSSPrefixMatched;
      break;

    default: /* kw_scanner -> matchcount > 1 */

      /*
       * More than one match. If one of them is a full match, i.e. the token
       * matches exactly the keyword, it's a full-and-prefix match, otherwise
       * it's a prefixes-match.
       */
      kw_scanner -> state = (kw_scanner -> token)
        ? KSSFullMatchAndPrefixes
        : KSSPrefixesMatched;
      break;

  }
  return kw_scanner;
}

kw_scanner_t * _kw_scanner_reset(kw_scanner_t *kw_scanner) {
  kw_scanner -> state = KSSInit;
  kw_scanner -> matchcount = 0;
  kw_scanner -> token = NULL;
  return kw_scanner;
}

kw_scanner_t * _kw_scanner_scan(kw_scanner_t *kw_scanner, lexer_t *lexer) {
  int carry_on;

  _kw_scanner_reset(kw_scanner);
  if (!lexer_get_char(lexer)) {
    return kw_scanner;
  }
  lexer_push(lexer);
  for (carry_on = 1; carry_on; ) {
    _kw_scanner_match(kw_scanner, lexer -> token);
    carry_on = 0;
    
    switch (kw_scanner -> state) {
      case KSSNoMatch:
        break;
        
      case KSSFullMatch:
      case KSSFullMatchAndPrefixes:
        /* Fall through */
        
      case KSSPrefixesMatched:
      case KSSPrefixMatched:
        /*
         * Carry on. Read the next character and do another match.
         */
        if (lexer_get_char(lexer)) {
          lexer_push(lexer);
          carry_on = 1;
        } else {
          /*
           * End of stream. So no match.
           */
          kw_scanner -> state = KSSNoMatch;
        }
        break;
        
      case KSSPrefixMatchLost:
        /*
         * We lost the match, but there was never a full match.
         */
        kw_scanner -> state = KSSNoMatch;
        break;
        
      case KSSFullMatchLost:
        /*
         * The full match has been lost. We set the state to match lost, and let
         * other handlers decide if the token we've found up to now is for
         * instance the leading part of an identifier. If no handlers want
         * accept the token, we will end up in this scanner again in the 
         * match lost state. We then push back the character causing us to
         * lose the match and everything after it, and return the code of the 
         * keyword.
         */
        break;
    }
  };
  return kw_scanner;
}

token_t * _keywords_match(scanner_t *scanner) {
  token_t      *token = NULL;
  kw_scanner_t *kw_scanner = (kw_scanner_t *) scanner -> data;

  if (!kw_scanner -> config -> num_keywords) {
    return token;
  }
  _kw_scanner_scan(kw_scanner, scanner -> lexer);
  return NULL;
}

token_t * _keywords_match_2nd_pass(scanner_t *scanner) {
  token_t      *token = NULL;
  kw_scanner_t *kw_scanner = (kw_scanner_t *) scanner -> data;

  switch (kw_scanner -> state) {
    case KSSNoMatch:
      /* Nothing */
      break;

    case KSSFullMatchLost:
      token = lexer_get_accept(scanner -> lexer,
                               token_code(kw_scanner -> token),
                               strlen(token_token(kw_scanner -> token)));
      break;
  }
  return token;
}

void keywords_register(void) {
  int          sc;

  if (KWScanner < 0) {
    sc = scanner_config_typeid();
    KWScannerConfig = typedescr_create_and_register(KWScannerConfig, _kw_scanner_config_vtable, NULL);
    typedescr_assign_inheritance(typedescr_get(KWScannerConfig), sc);
    KWScanner = typedescr_create_and_register(KWScannerConfig, _kw_scanner_vtable, NULL);
    typedescr_assign_inheritance(typedescr_get(KWScannerConfig), sc);

    scanner_config_register(typedescr_get(KWScannerConfig));
}