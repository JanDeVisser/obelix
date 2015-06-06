/*
 * lexer.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <array.h>
#include <dict.h>
#include <lexer.h>
#include <logging.h>


#ifdef LEXER_DEBUG
  #undef LEXER_DEBUG
#endif

//#ifndef LEXER_DEBUG
//  #define LEXER_DEBUG
//#endif

typedef lexer_t * (*newline_fnc)(lexer_t *, int);

typedef struct _tokenize_ctx {
  data_t  *parser;
  array_t *args;
} tokenize_ctx_t;

typedef struct _kw_matches {
  int               matches;
  int               code;
  list_t           *keywords;
  str_t            *token;
  kw_match_state_t  state;
} kw_matches_t;

static void             _lexer_init(void) __attribute__((constructor(102)));
static void             _dequotify(str_t *);
static int              _is_identifier(str_t *);

static kw_matches_t *   _kw_matches_create();
static void             _kw_matches_free(kw_matches_t *);

static int              _lexer_get_char(lexer_t *);
static void             _lexer_push_back(lexer_t *, int);
static void             _lexer_push_all_back(lexer_t *);
static lexer_t *        _lexer_keyword_match_reducer(token_t *, lexer_t *);
static int              _lexer_keyword_match(lexer_t *);
static token_t *        _lexer_match_token(lexer_t *, int);
static int              _lexer_accept(lexer_t *, token_t *);
static lexer_t *        _lexer_on_newline(lexer_t *, int);

static void             _lexer_free(lexer_t *);
static char *           _lexer_tostring(lexer_t *);
static data_t *         _lexer_resolve(lexer_t *, char *);
static data_t *         _lexer_has_next(lexer_t *);
static data_t *         _lexer_next(lexer_t *);
//static data_t *       _lexer_set(lexer_t *, char *);
static data_t *         _lexer_mth_rollup(lexer_t *, char *, array_t *, dict_t *);
static tokenize_ctx_t * _lexer_tokenize_reducer(token_t *, tokenize_ctx_t *);
static data_t *         _lexer_mth_tokenize(lexer_t *, char *, array_t *, dict_t *);

static code_label_t lexer_state_names[] = {
  { LexerStateFresh,           "LexerStateFresh" },
  { LexerStateInit,            "LexerStateInit" },
  { LexerStateSuccess,         "LexerStateSuccess" },
  { LexerStateWhitespace,      "LexerStateWhitespace" },
  { LexerStateNewLine,         "LexerStateNewLine" },
  { LexerStateIdentifier,      "LexerStateIdentifier" },
  { LexerStateKeyword,         "LexerStateKeyword" },
  { LexerStatePlusMinus,       "LexerStatePlusMinus" },
  { LexerStateZero,            "LexerStateZero" },
  { LexerStateNumber,          "LexerStateNumber" },
  { LexerStateDecimalInteger,  "LexerStateDecimalInteger" },
  { LexerStateHexInteger,      "LexerStateHexInteger" },
  { LexerStateFloat,           "LexerStateFloat" },
  { LexerStateSciFloat,        "LexerStateSciFloat" },
  { LexerStateQuotedStr,       "LexerStateQuotedStr" },
  { LexerStateQuotedStrEscape, "LexerStateQuotedStrEscape" },
  { LexerStateHashPling,       "LexerStateHashPling" },
  { LexerStateSlash,           "LexerStateSlash" },
  { LexerStateBlockComment,    "LexerStateBlockComment" },
  { LexerStateLineComment,     "LexerStateLineComment" },
  { LexerStateStar,            "LexerStateStar" },
  { LexerStateDone,            "LexerStateDone" },
  { -1,                        NULL }
};

static code_label_t matcher_state_names[] = {
  { KMSInit,                           "KMSInit" },
  { KMSPrefixMatched,                  "KMSPrefixMatched" },
  { KMSPrefixesMatched,                "KMSPrefixesMatched" },
  { KMSIdentifierFullMatch,            "KMSIdentifierFullMatch" },
  { KMSIdentifierFullMatchAndPrefixes, "KMSIdentifierFullMatchAndPrefixes" },
  { KMSFullMatch,                      "KMSFullMatch" },
  { KMSFullMatchAndPrefixes,           "KMSFullMatchAndPrefixes" },
  { KMSMatchLost,                      "KMSMatchLost" },
  { KMSNoMatch,                        "KMSNoMatch" },
  { -1,                                NULL }
};

static code_label_t lexer_option_labels[] = {
  { .code = LexerOptionIgnoreWhitespace,    .label = "LexerOptionIgnoreWhitespace" },
  { .code = LexerOptionIgnoreNewLines,      .label = "LexerOptionIgnoreNewLines" },
  { .code = LexerOptionIgnoreAllWhitespace, .label = "LexerOptionIgnoreAllWhitespace" },
  { .code = LexerOptionCaseSensitive,       .label = "LexerOptionCaseSensitive" },
  { .code = LexerOptionHashPling,           .label = "LexerOptionHashPling" },
  { .code = LexerOptionSignedNumbers,       .label = "LexerOptionSignedNumbers" },
  { .code = LexerOptionOnNewLine,           .label = "LexerOptionOnNewLine" },
  { .code = LexerOptionLAST,                .label = NULL }
};

int lexer_debug = 0;
int Lexer = -1;

/* ------------------------------------------------------------------------ */

static vtable_t _vtable_lexer[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionFree,     .fnc = (void_t) _lexer_free },
  { .id = FunctionToString, .fnc = (void_t) _lexer_tostring },
  { .id = FunctionResolve,  .fnc = (void_t) _lexer_resolve },
  { .id = FunctionNext,     .fnc = (void_t) _lexer_next },
  { .id = FunctionHasNext,  .fnc = (void_t) _lexer_has_next },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methoddescr_lexer[] = {
  { .type = -1,     .name = "rollup",   .method = (method_t) _lexer_mth_rollup,   .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "tokenize", .method = (method_t) _lexer_mth_tokenize, .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,                .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static typedescr_t _typedescr_lexer = {
  .type = -1,
  .type_name = "lexer",
  .vtable = _vtable_lexer
};

/* ------------------------------------------------------------------------ */

void _lexer_init(void) {
  logging_register_category("lexer", &lexer_debug);
  Lexer = typedescr_register_type(&_typedescr_lexer, _methoddescr_lexer);
}

void _dequotify(str_t *str) {
  int len, ix;

  len = str_len(str);
  if ((len >= 2) && (str_at(str, 0) == str_at(str, len - 1))) {
    switch (len) {
      case 2:
        str_erase(str);
        break;
      default:
        str_lchop(str, 1);
        str_chop(str, 1);
        break;
    }
  }
}

int _is_identifier(str_t *str) {
  char *s = str_chars(str);
  int   ret = TRUE;
  int   ix;
  int   len;
  int   c;
  
  len = str_len(str);
  for (ix = 0; ix < len; ix++) {
    c = s[ix];
    ret = ((!ix) ? isalpha(c) : isalnum(c)) || (c == '_');
    if (!ret) break;
  }
  return ret;
}

/*
 * public utility functions
 */

char * _state_name(code_label_t *str_table, int state) {
  char *ret = label_for_code(str_table, state);

  if (!ret) {
    ret = "[Unknown state]";
  }
  return ret;
}

char * lexer_state_name(lexer_state_t state) {
  return label_for_code(lexer_state_names, state);
}

char * lexer_option_name(lexer_option_t option) {
  return label_for_code(lexer_option_labels, option);
}

/*
 * kw_matches_t - static interface
 */

kw_matches_t * _kw_matches_create(list_t *keywords) {
  kw_matches_t *ret = NEW(kw_matches_t);
  
  ret -> matches = 0;
  ret -> keywords = keywords;
  ret -> code = TokenCodeNone;
  ret -> token = NULL;
  ret -> state = KMSInit;
  return ret;
}

void _kw_matches_free(kw_matches_t *kw_matches) {
  if (kw_matches) {
    str_free(kw_matches -> token);
    free(kw_matches);
  }
}

kw_matches_t * _kw_matches_match_reducer(token_t *kw_token, kw_matches_t *kw_matches) {
  str_t *t = kw_matches -> token;
  char  *kw = token_token(kw_token);

  if ((str_len(t) <= strlen(kw)) &&
      !str_ncmp_chars(t, kw, str_len(t))) {
    kw_matches -> matches++;
    if (str_len(t) == strlen(kw)) {
      kw_matches -> code = token_code(kw_token);
    }
  }
  return kw_matches;
}

kw_matches_t * _kw_matches_match(kw_matches_t *kw_matches, str_t *token) {
  kw_match_state_t state = kw_matches -> state;
  
  kw_matches -> token = str_copy(token);
  kw_matches -> code = TokenCodeNone;
  kw_matches -> matches = 0;
  if (!str_len(kw_matches -> token)) {
    return kw_matches;
  }

  list_reduce(kw_matches -> keywords,
              (reduce_t) _kw_matches_match_reducer, 
              kw_matches);
  switch (kw_matches -> matches) {
    case 0:
      kw_matches -> state = ((state == KMSFullMatchAndPrefixes) ||
                             (state == KMSIdentifierFullMatch))
        ? KMSMatchLost
        : KMSNoMatch;
      break;
    case 1:
      kw_matches -> state = (kw_matches -> code) 
        ? KMSFullMatch
        : KMSPrefixMatched;
      break;
    default: /* kw_matches -> matches > 0 */
      if (state == KMSMatchLost) {
        kw_matches -> state = (kw_matches -> code)
          ? KMSFullMatch
          : KMSNoMatch;
      } else {
        kw_matches -> state = (kw_matches -> code)
          ? KMSFullMatchAndPrefixes
          : KMSPrefixesMatched;
      }
      break;
  }
  if ((kw_matches -> state == KMSFullMatch) && _is_identifier(token)) {
    kw_matches -> state = KMSIdentifierFullMatch;
  }
  return kw_matches;
}

kw_matches_t * _kw_matches_reset(kw_matches_t *kw_matches) {
  kw_matches -> state = KMSInit;
  str_free(kw_matches -> token);
  kw_matches -> token = NULL;
  kw_matches -> matches = 0;
  kw_matches -> code = TokenCodeNone;
  return kw_matches;
}

/* -- L E X  E R  D A T A  F U N C T I O N S ------------------------------ */

void _lexer_free(lexer_t *lexer) {
  if (lexer) {
    token_free(lexer -> last_token);
    list_free(lexer -> keywords);
    str_free(lexer -> token);
    str_free(lexer -> pushed_back);
    str_free(lexer -> buffer);
    free(lexer);
  }
}

char * _lexer_tostring(lexer_t *lexer) {
  if (!lexer -> _d.str) {
    asprintf(&lexer -> _d.str, "Lexer for '%s'", 
             data_tostring(lexer -> reader));
  }
  return NULL;
}

data_t * _lexer_keyword_list(lexer_t *lexer) {
  array_t *kw = data_array_create(list_size(lexer -> keywords));
  data_t  *ret;
  
  for (list_start(lexer -> keywords); list_has_next(lexer -> keywords); ) {
    array_push(kw, data_copy(list_next(lexer -> keywords)));
  }
  ret = data_create_list(kw);
  array_free(kw);
  return ret;
}

data_t * _lexer_resolve_option(lexer_t *lexer, char *name) {
  int opt = code_for_label(lexer_option_labels, name);
  return (opt >= 0) ? data_create(Int, lexer_get_option(lexer, opt)) : NULL;
}

data_t * _lexer_resolve(lexer_t *lexer, char *name) {
  if (!strcmp(name, "reader")) {
    return data_copy(lexer -> reader);
  } else if (!strcmp(name, "statename")) {
    return data_create(String, lexer_state_name(lexer -> state));
  } else if (!strcmp(name, "state")) {
    return data_create(Int, lexer -> state);
  } else if (!strcmp(name, "line")) {
    return data_create(Int, lexer -> line);
  } else if (!strcmp(name, "column")) {
    return data_create(Int, lexer -> column);
  } else if (!strcmp(name, "keywords")) {
    return _lexer_keyword_list(lexer);
  } else {
    return _lexer_resolve_option(lexer, name);
  }
}

data_t * _lexer_has_next(lexer_t *lexer) {
  return data_create(Int, lexer_next_token(lexer) ? TRUE : FALSE);
}

data_t * _lexer_next(lexer_t *lexer) {
  return data_create(Token, lexer -> last_token);
}

data_t * _lexer_mth_rollup(lexer_t *lexer, char *n, array_t *args, dict_t *kwargs) {
  return (data_t *) lexer_rollup_to(lexer, data_intval(data_array_get(args, 0)));
}

tokenize_ctx_t * _lexer_tokenize_reducer(token_t *token, tokenize_ctx_t *ctx) {
  data_t *d;
  
  array_set(ctx -> args, 0, data_create(Token, token));
  d = data_call(ctx -> parser, ctx -> args, NULL);
  if (d != data_array_get(ctx -> args, 0)) {
    array_set(ctx -> args, 0, d);
  }
  if (!data_cmp(d, data_null()) || ((data_type(d) == Bool) && !data_intval(d))) {
    return NULL;
  } else {
    return ctx;
  }
}

data_t * _lexer_mth_tokenize(lexer_t *lexer, char *n, array_t *args, dict_t *kwargs) {
  tokenize_ctx_t  ctx;
  data_t         *ret;
  void           *retval;
  
  ctx.parser = data_copy(data_array_get(args, 0));
  ctx.args = data_array_create(3);
  
  array_set(ctx.args, 0, NULL);
  array_set(ctx.args, 1, data_copy(data_array_get(args, 1)));
  array_set(ctx.args, 2, data_create(Lexer, lexer));
  lexer_tokenize(lexer, (reduce_t) _lexer_tokenize_reducer, (void *) &ctx);
  ret = data_copy(data_array_get(ctx.args, 0));
  array_free(ctx.args);
  data_free(ctx.parser);
  return ret;
}

/* -- L E X E R  S T A T I C  F U N C T I O N S --------------------------- */

int _lexer_get_char(lexer_t *lexer) {
  int   ch;
  int   ret;

  if (lexer -> pushed_back) {
    ch = str_readchar(lexer -> pushed_back);
    if (ch > 0) {
#ifdef LEXER_DEBUG
      debug("_lexer_get_char: read '%c' from pushed_back", ch);
#endif
      return ch;
    } else {
#ifdef LEXER_DEBUG
      debug("_lexer_get_char: pushed_back exhausted");
#endif
      str_free(lexer -> pushed_back);
      lexer -> pushed_back = NULL;
    }
  }

  if (!lexer -> buffer) {
    lexer -> buffer = str_create(LEXER_BUFSIZE);
    if (!lexer -> buffer) {
      return 0;
    }
    ret = str_readinto(lexer -> buffer, lexer -> reader);
#ifdef LEXER_DEBUG
    debug("_lexer_get_char: Created buffer and did initial read: %d", ret);
#endif
    if (ret <= 0) {
      return 0;
    }
  }
  ch = str_readchar(lexer -> buffer);
#ifdef LEXER_DEBUG
    debug("_lexer_get_char: Read character: '%c'", (ch > 0) ? ch : '$');
#endif
  if (ch <= 0) {
    ret = str_readinto(lexer -> buffer, lexer -> reader);
#ifdef LEXER_DEBUG
    debug("_lexer_get_char: Read new chunk into buffer: %d", ret);
#endif
    if (ret <= 0) {
      return 0;
    } else {
      ch = str_readchar(lexer -> buffer);
#ifdef LEXER_DEBUG
      debug("_lexer_get_char: Read character from new chunk: '%c'", (ch > 0) ? ch : '$');
#endif
    }
  }
  return ch;
}

void _lexer_push_back(lexer_t *lexer, int ch) {
  if (ch <= 0) {
#ifdef LEXER_DEBUG
    debug("_lexer_push_back: Not pushing back EOF character");
#endif
    return;
  }
#ifdef LEXER_DEBUG
  debug("_lexer_push_back: in current_token: '%s' current_token_len: %d",
        str_chars(lexer -> token), str_len(lexer -> token));
#endif
  str_chop(lexer -> token, 1);
  str_pushback((lexer -> pushed_back) ? lexer -> pushed_back : lexer -> buffer, 1);
#ifdef LEXER_DEBUG
  debug("_lexer_push_back: out current_token: '%s' current_token_len: %d",
        str_chars(lexer -> token), str_len(lexer -> token));
#endif
}

void _lexer_push_all_back(lexer_t *lexer) {
  if (lexer -> pushed_back) {
    str_append(lexer -> pushed_back, lexer -> token);
  } else {
    lexer -> pushed_back = str_copy(lexer -> token);
  }
#ifdef LEXER_DEBUG
  debug("_lexer_push_all_back: pushed_back: '%s'", 
        str_chars(lexer -> pushed_back));
#endif
  str_erase(lexer -> token);
}

int _lexer_keyword_match(lexer_t *lexer) {
  int   ret = TokenCodeNone;
  
#ifdef LEXER_DEBUG
  debug("_lexer_keyword_match in - lexer state: %s matcher state: %s",
        lexer_state_name(lexer -> state), 
        _state_name(matcher_state_names, lexer -> matches -> state));
#endif
  if (!str_len(lexer -> token)) {
    return ret;
  }
  
  _kw_matches_match(lexer -> matches, lexer -> token);
  switch (lexer -> matches -> state) {
    case KMSNoMatch:
      lexer -> state = LexerStateInit;
      break;
    case KMSFullMatch:
    case KMSIdentifierFullMatch:
      ret = lexer -> matches -> code;
      /* no break */
    default:
      lexer -> state = LexerStateKeyword;
      break;
  }
#ifdef LEXER_DEBUG
  debug("_lexer_keyword_match out - lexer state: %s matcher state: %s",
        lexer_state_name(lexer -> state), 
        _state_name(matcher_state_names, lexer -> matches -> state));
#endif
  return ret;
}

void _lexer_update_location(lexer_t *lexer, int ch) {
  if (strchr("\r\n", ch)) {
    if (!strchr("\r\n", lexer -> prev_char) || (ch != lexer -> prev_char)) {
      lexer -> line++;
      lexer -> column = 0;
    }
  } else {
    lexer -> column++;
  }
}

token_t * _lexer_match_token(lexer_t *lexer, int ch) {
  token_code_t    code;
  token_t        *ret;
  int             ignore_nl;
  list_t         *matches;
  listiterator_t *iter;
  
  ignore_nl = lexer_get_option(lexer, LexerOptionIgnoreNewLines);
  _lexer_update_location(lexer, ch);
  str_append_char(lexer -> token, ch);
  code = TokenCodeNone;
  ret = NULL;
#ifdef LEXER_DEBUG
  debug("_lexer_match_token in - lexer state: %s matcher state: %s ch: '%c' tok: '%s'",
        lexer_state_name(lexer -> state), 
        _state_name(matcher_state_names, lexer -> matches -> state),
        (ch > 0) ? ch : '$',
        str_chars(lexer -> token));
#endif
  switch (lexer -> state) {
    
    case LexerStateInit:
      if (lexer -> matches -> state != KMSNoMatch) {
        code = _lexer_keyword_match(lexer);
      }
      if (lexer -> state != LexerStateKeyword) {
        if (!ignore_nl && ((ch == '\r') || (ch == '\n'))) {
          lexer -> state = LexerStateNewLine;
        } else if (isspace(ch)) {
          lexer -> state = LexerStateWhitespace;
        } else if (isalpha(ch) || (ch == '_')) {
          lexer -> state = LexerStateIdentifier;
        } else if (lexer_get_option(lexer, LexerOptionSignedNumbers) && 
                   ((ch == '-') || (ch == '+'))) {
          lexer -> quote = ch;
          lexer -> state = LexerStatePlusMinus;
        } else if (ch == '0') {
          lexer -> state = LexerStateZero;
        } else if (isdigit(ch)) {
          lexer -> state = LexerStateNumber;
        } else if ((ch == '\'') || (ch == '"') || (ch == '`')) {
          lexer -> state = LexerStateQuotedStr;
          lexer -> quote = ch;
        } else if (ch == '/') {
          lexer -> state = LexerStateSlash;
        } else if ((ch == '#') && ((lexer -> line == 1) &&
                   (lexer -> column == 1)) &&
                   lexer_get_option(lexer, LexerOptionHashPling)) {
          lexer -> state = LexerStateHashPling;
        } else if (ch > 0) {
          code = ch;
        }
      }
      break;
      
    case LexerStateNewLine:
      if ((ch <= 0) || ((ch != '\n') && (ch != '\r'))) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeNewLine;
      }
      break;
    case LexerStateWhitespace:
      if (!isspace(ch) ||
          (!ignore_nl && ((ch == '\r') || (ch == '\n')))) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeWhitespace;
      }
      break;
    case LexerStateIdentifier:
      if (!isalnum(ch) && (ch != '_')) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeIdentifier;
      }
      break;
    case LexerStatePlusMinus:
      if (ch == '0') {
        lexer -> state = LexerStateZero;
      } else if (isdigit(ch) || (ch == '.')) {
        lexer -> state = LexerStateNumber;
      } else {
        _lexer_push_back(lexer, ch);
        code = lexer -> quote;
      }
      break;
    case LexerStateZero:
      /* FIXME: Second leading zero */
      if (isdigit(ch)) {
	/*
	 * We don't want octal numbers. Therefore we strip
         * leading zeroes.
	 */
        str_chop(lexer -> token, 2);
        str_append_char(lexer -> token, ch);
        lexer -> state = LexerStateNumber;
      } else if (ch == '.') {
        lexer -> state = LexerStateFloat;
      } else if ((ch == 'x') || (ch == 'X')) {
	/*
	 * Hexadecimals are returned including the leading
         * 0x. This allows us to send both base-10 and hex ints
         * to strtol and friends.
	 */
        lexer -> state = LexerStateHexInteger;
      } else {
        _lexer_push_back(lexer, ch);
        code = TokenCodeInteger;
      }
      break;
    case LexerStateNumber:
      if (ch == '.') {
        lexer -> state = LexerStateFloat;
      } else if ((ch == 'e') || (ch == 'E')) {
	str_chop(lexer -> token, 1);
        str_append_char(lexer -> token, 'e');
        lexer -> state = LexerStateSciFloat;
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeInteger;
      }
      break;
    case LexerStateFloat:
      if ((ch == 'e') || (ch == 'E')) {
	str_chop(lexer -> token, 1);
        str_append_char(lexer -> token, 'e');
        lexer -> state = LexerStateSciFloat;
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeFloat;
      }
      break;
    case LexerStateSciFloat:
      if (((ch == '+') || (ch == '-')) && (str_at(lexer -> token, -2) == 'e')) {
        // Nothing
      } else if (!isdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeFloat;
      }
      break;
    case LexerStateHexInteger:
      if (!isxdigit(ch)) {
        _lexer_push_back(lexer, ch);
        code = TokenCodeHexNumber;
      }
      break;
    case LexerStateQuotedStr:
      if (ch == lexer -> quote) {
        code = (token_code_t) lexer -> quote;
        _dequotify(lexer -> token);
      } else if (ch == '\\') {
        lexer -> state = LexerStateQuotedStrEscape;
        str_chop(lexer -> token, 1);
      }
      break;
    case LexerStateQuotedStrEscape:
      if (ch == 'n') {
        str_append_char(lexer -> token, '\n');
      } else if (ch == 'r') {
        str_append_char(lexer -> token, '\r');
      } else if (ch == 't') {
        str_append_char(lexer -> token, '\t');
      }
      lexer -> state = LexerStateQuotedStr;
      break;
    case LexerStateHashPling:
      if (ch == '!') {
        str_erase(lexer -> token);
        lexer -> state = LexerStateLineComment;
      } else {
        _lexer_push_back(lexer, ch);
        code = TokenCodeHash;
      }
      break;
    case LexerStateSlash:
      if (ch == '*') {
        str_erase(lexer -> token);
        lexer -> state = LexerStateBlockComment;
      } else if (ch == '/') {
        str_erase(lexer -> token);
        lexer -> state = LexerStateLineComment;
      } else {
        _lexer_push_back(lexer, ch);
        code = TokenCodeSlash;
      }
      break;
    case LexerStateBlockComment:
      str_erase(lexer -> token);
      if (ch == '*') {
        lexer -> state = LexerStateStar;
      }
      break;
    case LexerStateStar:
      str_erase(lexer -> token);
      lexer -> state = (ch == '/') ? LexerStateInit : LexerStateBlockComment;
      break;
    case LexerStateLineComment:
      str_erase(lexer -> token);
      if ((ch == '\r') || (ch == '\n') || (ch <= 0)) {
        lexer -> state = LexerStateInit;
      }
      break;

    case LexerStateKeyword:
      if (lexer -> matches -> state == KMSIdentifierFullMatch) {
        if (isalnum(ch) || (ch == '_')) {
          lexer -> state = LexerStateIdentifier;
        } else {
          _lexer_push_back(lexer, ch);
          code = lexer -> matches -> code;
        }
      } else {
        _lexer_keyword_match(lexer);
        switch (lexer -> matches -> state) {
          case KMSFullMatch:
            code = lexer -> matches -> code;
            break;
          case KMSMatchLost:
            if (_is_identifier(lexer -> token)) {
              lexer -> state = LexerStateIdentifier;
            } else {
              _lexer_push_back(lexer, ch);
              code = _lexer_keyword_match(lexer);
            }
            break;
          case KMSNoMatch:
            _lexer_push_all_back(lexer);
            break;
        }
      }
      break;
  }
  if (code != TokenCodeNone) {
    ret = token_create(code, str_chars(lexer -> token));
    lexer -> state = LexerStateSuccess;
  }
  if (ch <= 0) {
    if (!ret) {
      switch (lexer -> state) {
        case LexerStateBlockComment:
        case LexerStateSlash:
          ret = token_create(TokenCodeError, "Unterminated block comment");
          break;
        case LexerStateQuotedStr:
        case LexerStateQuotedStrEscape:
          ret = token_create(TokenCodeError, "Unterminated string");
          break;
      }
      lexer -> state = LexerStateDone;
    }
  }
  if (ret) {
    ret -> line = lexer -> line;
    ret -> column = lexer -> column;
  }
#ifdef LEXER_DEBUG
    if (ret) {
      debug("_lexer_match_token out - state: %s token: %s",
            lexer_state_name(lexer -> state), token_code_name(ret -> code));
    } else {
      debug("_lexer_match_token out - state: %s", lexer_state_name(lexer -> state));
    }
#endif
  lexer -> prev_char = ch;
  return ret;
}

int _lexer_accept(lexer_t *lexer, token_t *token) {
  long ignore_all_ws = lexer_get_option(lexer, LexerOptionIgnoreAllWhitespace);
  long ignore_ws = ignore_all_ws || lexer_get_option(lexer, LexerOptionIgnoreWhitespace);
  long ignore_nl = ignore_all_ws || lexer_get_option(lexer, LexerOptionIgnoreNewLines);
  
  if (!token) {
    return 1;
    /* FIXME Counter-intuitive but lexer_next_token deals with a NULL token */
  }
  if (token_code(token) == TokenCodeNewLine) {
    return !ignore_nl;
  } else if (token_iswhitespace(token)) {
    return !ignore_ws;
  } else {
    return 1;
  }
}

lexer_t * _lexer_on_newline(lexer_t *lexer, int line) {
  function_t  *fnc;
  newline_fnc  on_newline;
  
  if (fnc = (function_t *) lexer_get_option(lexer, LexerOptionOnNewLine)) {
    on_newline = (newline_fnc) fnc -> fnc;
    return on_newline(lexer, line);
  } else {
    return lexer;
  }
}

/* -- L E X E R  P U B L I C  I N T E R F A C E --------------------------- */

lexer_t * lexer_create(data_t *reader) {
  lexer_t *ret;
  int      ix;
  
  ret = NEW(lexer_t);
  data_settype(&ret -> _d, Token);
  ret -> reader = reader;
  ret -> pushed_back = NULL;
  ret -> buffer = NULL;
  ret -> state = LexerStateFresh;
  ret -> last_token = NULL;
  ret -> line = 1;
  ret -> column = 0;
  ret -> prev_char = 0;
  for (ix = 0; ix < (int) LexerOptionLAST; ix++) {
    lexer_set_option(ret, ix, 0L);
  }
  ret -> keywords = list_create();
  list_set_free(ret -> keywords, (free_t) data_free);
  ret -> token = str_create(LEXER_INIT_TOKEN_SZ);
  return ret;
}

lexer_t * lexer_set_option(lexer_t *lexer, lexer_option_t option, long value) {
  lexer -> options[(int) option] = value;
  return lexer;
}

long lexer_get_option(lexer_t *lexer, lexer_option_t option) {
  return lexer -> options[(int) option];
}

lexer_t * lexer_add_keyword(lexer_t *lexer, int code, char *token) {
  list_append(lexer -> keywords, token_create(code, token));
  return lexer;
}

void * _lexer_tokenize(lexer_t *lexer, reduce_t parser, void *data) {
  while (lexer_next_token(lexer)) {
    data = parser(lexer -> last_token, data);
    if (!data) {
      break;
    }
  }
  return data;
}

token_t * lexer_next_token(lexer_t *lexer) {
  int          ch;
  token_t     *ret;
  int          first;
  
  if (!lexer -> last_token) {
    _lexer_on_newline(lexer, 1);
    lexer -> matches = _kw_matches_create(lexer -> keywords);
  } else if (token_code(lexer -> last_token) == TokenCodeEnd) {
    token_free(lexer -> last_token);
    lexer -> last_token = token_create(TokenCodeExhausted, "$$$$");
    return NULL;
  } else if (token_code(lexer -> last_token) == TokenCodeExhausted) {
    return NULL;
  } else {
    token_free(lexer -> last_token);
    lexer -> last_token = NULL;
  }
  
  first = lexer -> state == LexerStateFresh;
  ret = NULL;
  
  do {
    lexer -> state = LexerStateInit;
    str_erase(lexer -> token);
    _kw_matches_reset(lexer -> matches);

    do {
      if (ret) {
        token_free(ret);
        ret = NULL;
      }
      ch = _lexer_get_char(lexer);
      ret = _lexer_match_token(lexer, ch);
    } while ((lexer -> state != LexerStateDone) && (lexer -> state != LexerStateSuccess));
#ifdef LEXER_DEBUG
    if (ret) {
      debug("lexer_next_token: matched token: %s [%s]", token_code_name(ret -> code), ret -> token);
    }
#endif
    if (!ret && (lexer -> state == LexerStateDone)) {
      ret = token_create(TokenCodeEnd, "$$");
      ret -> line = lexer -> line;
      ret -> column = lexer -> column;
    }
    if (ret && (token_code(ret) == TokenCodeNewLine)) {
      _lexer_on_newline(lexer, ret -> line);
    }
  } while (!_lexer_accept(lexer, ret));

  lexer -> last_token = ret;
  if (token_code(ret) == TokenCodeEnd) {
    _kw_matches_free(lexer -> matches);
    lexer -> matches = NULL;
  }
  
  if (ret && lexer_debug) {
    debug("lexer_next_token out: token: %s [%s], state %s",
          token_code_name(ret -> code), ret -> token, lexer_state_name(lexer -> state));
  }
  return ret;
}

token_t * lexer_rollup_to(lexer_t *lexer, int marker) {
  token_t *ret;
  str_t   *str;
  int      ch;
  char    *msgbuf;
  
  str = str_create(10);
  for (ch = _lexer_get_char(lexer); 
       ch && (ch != marker); 
       ch = _lexer_get_char(lexer)) {
    if (ch == '\\') {
      ch = _lexer_get_char(lexer);
      if (!ch) {
        break;
      }
    }
    str_append_char(str, ch);
  }
  if (ch == marker) {
    ret = token_create(TokenCodeRawString, str_chars(str));
  } else {
    asprintf(&msgbuf, "Unterminated '%c'", marker);
    ret = token_create(TokenCodeError, msgbuf);
  }
  str_free(str);
  return ret;
}
