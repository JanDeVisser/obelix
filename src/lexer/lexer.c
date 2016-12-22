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
#include <string.h>

#include "liblexer.h"
#include <array.h>
#include <data.h>

#define LEXER_DEBUG

typedef struct _tokenize_ctx {
  data_t  *parser;
  array_t *args;
} tokenize_ctx_t;

static lexer_t *        _lexer_new(lexer_t *, va_list);
static void             _lexer_free(lexer_t *);
static char *           _lexer_allocstring(lexer_t *);
static data_t *         _lexer_resolve(lexer_t *, char *);
static data_t *         _lexer_has_next(lexer_t *);
static data_t *         _lexer_next(lexer_t *);
static data_t *         _lexer_call(lexer_t *, array_t *, dict_t *);
static tokenize_ctx_t * _lexer_tokenize_reducer(token_t *, tokenize_ctx_t *);
static data_t *         _lexer_mth_tokenize(lexer_t *, char *, array_t *, dict_t *);
static token_t *        _lexer_match_token(lexer_t *);

static code_label_t lexer_state_names[] = {
  { .code = LexerStateNoState,            .label = "LexerStateNoState" },
  { .code = LexerStateFresh,              .label = "LexerStateFresh" },
  { .code = LexerStateInit,               .label = "LexerStateInit" },
  { .code = LexerStateSuccess,            .label = "LexerStateSuccess" },
  { .code = LexerStateDone,               .label = "LexerStateDone" },
  { .code = LexerStateStale,              .label = "LexerStateStale" },
  { .code = -1,                           .label = NULL }
};

static vtable_t _vtable_Lexer[] = {
  { .id = FunctionNew,         .fnc = (void_t) _lexer_new },
  { .id = FunctionFree,        .fnc = (void_t) _lexer_free },
  { .id = FunctionAllocString, .fnc = (void_t) _lexer_allocstring },
  { .id = FunctionCall,        .fnc = (void_t) _lexer_call },
  { .id = FunctionResolve,     .fnc = (void_t) _lexer_resolve },
  { .id = FunctionNext,        .fnc = (void_t) _lexer_next },
  { .id = FunctionHasNext,     .fnc = (void_t) _lexer_has_next },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Lexer[] = {
  { .type = -1,     .name = "tokenize", .method = (method_t) _lexer_mth_tokenize, .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,                .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

int lexer_debug = 0;
int Lexer = -1;

/* ------------------------------------------------------------------------ */

extern void _lexer_config_init(void);
extern void _scanner_config_init(void);
extern void _scanner_init(void);

void lexer_init(void) {
  if (Lexer < 0) {
    _lexer_config_init();
    _scanner_config_init();
    _scanner_init();
    logging_register_category("lexer", &lexer_debug);
    typedescr_register_with_methods(Lexer, lexer_t);
  }
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

/* -- L E X E R  D A T A  F U N C T I O N S ------------------------------ */

lexer_t * _lexer_new(lexer_t *ret, va_list args) {
  lexer_config_t   *config = va_arg(args, lexer_config_t *);
  data_t           *reader = va_arg(args, data_t *);
  scanner_config_t *scanner_config;

  ret -> config = lexer_config_copy(config);
  ret -> reader = data_copy(reader);

  ret -> buffer = NULL;
  ret -> state = LexerStateFresh;
  ret -> where = LexerWhereBegin;
  ret -> last_token = NULL;
  ret -> count = 0;
  ret -> scan_count = 0;
  ret -> line = 1;
  ret -> column = 1;
  ret -> prev_char = 0;
  ret -> token = str_create(LEXER_INIT_TOKEN_SZ);

  ret -> scanners = NULL;
  for (scanner_config = config -> scanners; scanner_config; scanner_config = scanner_config -> next) {
    scanner_config_instantiate(scanner_config, ret);
  }
  return ret;
}

void _lexer_free(lexer_t *lexer) {
  scanner_t *scanner;

  if (lexer) {
    token_free(lexer -> last_token);
    for (scanner = lexer -> scanners; scanner; scanner = scanner -> next) {
      if (scanner -> next) {
        scanner -> next -> prev = NULL;
      }
      lexer -> scanners = scanner -> next;
      scanner_free(scanner);
    }
    str_free(lexer -> token);
    if ((data_t *) lexer -> buffer != lexer -> reader) {
      str_free(lexer -> buffer);
    }
  }
}

char * _lexer_allocstring(lexer_t *lexer) {
  char *buf;

  asprintf(&buf, "Lexer for '%s'",
           data_tostring(lexer -> reader));
  return buf;
}

data_t * _lexer_resolve(lexer_t *lexer, char *name) {
  if (!strcmp(name, "reader")) {
    return data_copy(lexer -> reader);
  } else if (!strcmp(name, "statename")) {
    return str_to_data(lexer_state_name(lexer -> state));
  } else if (!strcmp(name, "state")) {
    return int_to_data(lexer -> state);
  } else if (!strcmp(name, "line")) {
    return int_to_data(lexer -> line);
  } else if (!strcmp(name, "column")) {
    return int_to_data(lexer -> column);
  } else {
    return NULL;
  }
}

data_t * _lexer_has_next(lexer_t *lexer) {
  return int_to_data(lexer_next_token(lexer) ? TRUE : FALSE);
}

data_t * _lexer_next(lexer_t *lexer) {
  return (data_t *) token_copy(lexer -> last_token);
}

tokenize_ctx_t * _lexer_tokenize_reducer(token_t *token, tokenize_ctx_t *ctx) {
  data_t *d;

  array_set(ctx -> args, 0, (data_t *) token_copy(token));
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

data_t * _lexer_call(lexer_t *lexer, array_t *args, dict_t *kwargs) {
  tokenize_ctx_t  ctx;
  data_t         *ret;

  ctx.parser = data_copy(data_array_get(args, 0));
  ctx.args = data_array_create(3);

  array_set(ctx.args, 0, NULL);
  array_set(ctx.args, 1,
            ((array_size(args) > 1) ? data_copy(data_array_get(args, 1)) : data_null()));
  array_set(ctx.args, 2, (data_t *) lexer_copy(lexer));
  lexer_tokenize(lexer, (reduce_t) _lexer_tokenize_reducer, (void *) &ctx);
  ret = data_copy(data_array_get(ctx.args, 0));
  array_free(ctx.args);
  data_free(ctx.parser);
  return ret;
}

data_t * _lexer_mth_tokenize(lexer_t *lexer, char *n, array_t *args, dict_t *kwargs) {
  return _lexer_call(lexer, args, kwargs);
}

/* -- L E X E R  S T A T I C  F U N C T I O N S --------------------------- */

lexer_t * _lexer_init_buffer(lexer_t *lexer) {
  if (data_is_string(lexer -> reader)) {
    lexer -> buffer = data_as_string(lexer -> reader);
  } else {
    lexer -> buffer = str_create(lexer_config_get_bufsize(lexer -> config));
    if (!lexer -> buffer) {
      return NULL;
    }
  }
  return lexer;
}

token_t * _lexer_match_token(lexer_t *lexer) {
  token_t   *ret;
  scanner_t *scanner;
  int        ch;
  int        line = lexer -> line;
  int        column = lexer -> column;

  debug(lexer, "_lexer_match_token", NULL);
  lexer -> state = LexerStateInit;
  lexer -> scanned = 0;
  for (scanner = lexer -> scanners;
       scanner;
       scanner = scanner -> next) {
    lexer -> scan_count = 0;
    if (scanner -> config -> match) {
      lexer_rewind(lexer);
      scanner -> config -> match(scanner);
      debug(lexer, "First pass with scanner '%s' = %s",
             data_typename(scanner -> config), token_tostring(lexer -> last_token));
    }
  }
  if (!lexer -> last_token && (lexer -> state != LexerStateSuccess)) {
    for (scanner = lexer -> scanners;
         scanner;
         scanner = scanner -> next) {
      lexer -> scan_count = 0;
      if (scanner -> config -> match_2nd_pass) {
        lexer_rewind(lexer);
        scanner -> config -> match_2nd_pass(scanner);
        debug(lexer, "Second pass with scanner '%s' = %s",
               data_typename(scanner -> config), token_tostring(lexer -> last_token));
      }
    }
  }

  /*
   * If no scanners accept whatever is coming, we grab one character and pass
   * that as a token. This maybe should be a separate catchall scanner.
   */
  ret = NULL;
  if (lexer -> last_token) {
    ret = lexer -> last_token;
  } else {
    if (lexer -> state != LexerStateSuccess) {
      debug(lexer, "Scanners found no token");
      lexer_rewind(lexer);
      ch = lexer_get_char(lexer);
      if (ch > 0) {
        lexer_push(lexer);
        ret = lexer_accept(lexer, ch);
      } else {
        debug(lexer, "End-of-file. Returning LexerStateDone");
        lexer -> state = LexerStateDone;
      }
    } else {
      oassert(lexer -> scanned,
        "_lexer_match_token found no token and doesn't skip anything");
      debug(lexer, "_lexer_match_token out - skipping %d characters", lexer -> scanned);
      lexer -> state = LexerStateSuccess;
    }
  }

  if (ret) {
    ret -> line = line;
    ret -> column = column;
    lexer -> state = LexerStateSuccess;
    debug(lexer, "_lexer_match_token out - state: %s token: %s",
               lexer_state_name(lexer -> state), token_code_name(ret -> code));
  }
  lexer_reset(lexer);
  return ret;
}

/* -- L E X E R  P U B L I C  I N T E R F A C E --------------------------- */

lexer_t * lexer_create(lexer_config_t *config, data_t *reader) {
  lexer_init();
  return (lexer_t *) data_create(Lexer, config, reader);
}

void * _lexer_tokenize(lexer_t *lexer, reduce_t parser, void *data) {
  while (lexer_next_token(lexer)) {
    if (!(data = parser(lexer -> last_token, data))) {
      break;
    }
  }
  return data;
}

token_t * lexer_next_token(lexer_t *lexer) {
  if (!lexer -> token) {
    lexer -> token = str_create(16);
  }
  if (lexer -> last_token) {
    if (token_code(lexer -> last_token) == TokenCodeEnd) {
      token_free(lexer -> last_token);
      lexer -> last_token = token_create(TokenCodeExhausted, "$$$$");
      lexer -> last_token -> line = lexer -> line;
      lexer -> last_token -> column = lexer -> column;
      return lexer -> last_token;
    } else if (token_code(lexer -> last_token) == TokenCodeExhausted) {
      return NULL;
    } else {
      token_free(lexer -> last_token);
      lexer -> last_token = NULL;
    }
  }
  do {
    _lexer_match_token(lexer);
    if (lexer -> state == LexerStateDone) {
      lexer -> last_token = token_create(TokenCodeEnd, "$$");
      lexer -> last_token -> line = lexer -> line;
      lexer -> last_token -> column = lexer -> column;
      return lexer -> last_token;
    }
  } while (!lexer -> last_token);

  if (lexer -> last_token) {
    debug(lexer, "lexer_next_token out: token: %s [%s], state %s",
                token_code_name(token_code(lexer -> last_token)),
                token_token(lexer -> last_token),
                lexer_state_name(lexer -> state));
  }
  return token_copy(lexer -> last_token);
}

int lexer_at_top(lexer_t *lexer) {
  return lexer -> count == 0;
}

int lexer_at_end(lexer_t *lexer) {
  return lexer -> where == LexerWhereEnd;
}

/**
 * Rewind the lexer to the point just after the last token was identified.
 */
lexer_t * lexer_rewind(lexer_t *lexer) {
  debug(lexer, "Rewinding lexer");
  if (lexer -> token) {
    debug(lexer, "Erasing scanned token '%s'", str_chars(lexer -> token));
    str_erase(lexer -> token);
  }
  if (lexer -> buffer) {
    debug(lexer, "Rewinding buffer '%s'", str_chars(lexer -> buffer));
    str_rewind(lexer -> buffer);
  }
  if ((lexer -> scan_count > 0) || lexer -> current) {
    lexer -> where = (lexer -> count) ? LexerWhereMiddle : LexerWhereBegin;
  }
  lexer -> scan_count = 0;
  return lexer;
}

/**
 * Mark the current point, discarding everything that came before it.
 */
lexer_t * lexer_reset(lexer_t *lexer) {
  debug(lexer, "Resetting lexer")
  if (lexer -> buffer) {
    str_rewind(lexer -> buffer);
    str_skip(lexer -> buffer, lexer -> scanned);
    str_reset(lexer -> buffer);
    debug(lexer, "buffer: %s", str_chars(lexer -> buffer));
  }
  lexer -> current = 0;
  if (lexer -> token) {
    str_erase(lexer -> token);
  }
  lexer -> count += lexer -> scanned;
  lexer -> scanned = 0;
  lexer -> scan_count = 0;
  return lexer;
}

token_t * lexer_accept(lexer_t *lexer, token_code_t code) {
  token_t *token;

  debug(lexer, "lexer_accept: token: %s code: %d", str_chars(lexer -> token), code);
  token = token_create(code, str_chars(lexer -> token));
  lexer_accept_token(lexer, token);
  token_free(token);
  return lexer -> last_token;
}

token_t * lexer_accept_token(lexer_t *lexer, token_t *token) {
  if (lexer -> scan_count && (!lexer -> scanned || (lexer -> scan_count > lexer -> scanned))) {
    token_free(lexer -> last_token);
    lexer -> last_token = token_copy(token);
    lexer_skip(lexer);
    debug(lexer, "lexer_accept_token(%s)", token_tostring(lexer -> last_token));
  }
  return lexer -> last_token;
}

void lexer_skip(lexer_t *lexer) {
  if (!lexer -> scanned || (lexer -> scan_count > lexer -> scanned)) {
    debug(lexer, "Setting skip to %d characters", lexer -> scan_count);
    lexer -> scanned = lexer -> scan_count;
    lexer_rewind(lexer);
    lexer -> state = LexerStateSuccess;
  }
}

/**
 * Rewinds the buffer and reads <code>num</code> characters from the buffer,
 * returning the read string as a token with code <code>code</code>.
 */
token_t * lexer_get_accept(lexer_t *lexer, token_code_t code, int num) {
  int i;

  lexer_rewind(lexer);
  for (i = 0; i < num; i++) {
    lexer_get_char(lexer);
    lexer_push(lexer);
  }
  return lexer_accept(lexer, code);
}

lexer_t * lexer_push(lexer_t *lexer) {
  return lexer_push_as(lexer, lexer -> current);
}

lexer_t * lexer_push_as(lexer_t *lexer, int ch) {
  if (ch) {
    str_append_char(lexer -> token, ch);
  }
  // debug(lexer, "lexer_push_as(%c));
  lexer -> prev_char = lexer -> current;
  lexer -> current = 0;
  lexer -> scan_count++;
  return lexer;
}

lexer_t * lexer_discard(lexer_t *lexer) {
  return lexer_push_as(lexer, 0);
}

int lexer_get_char(lexer_t *lexer) {
  int   ch;
  int   ret;

  lexer -> where = LexerWhereEnd;
  if (!lexer -> buffer) {
    if (!_lexer_init_buffer(lexer)) {
      return 0;
    }
    if (lexer -> buffer != (str_t *) lexer -> reader) {
      ret = str_readinto(lexer -> buffer, lexer -> reader);
      debug(lexer, "lexer_get_char: Created buffer and did initial read: %d", ret);
      if (ret <= 0) {
        return 0;
      }
    }
  }
  ch = str_readchar(lexer -> buffer);
  if (ch <= 0 && (lexer -> reader != (data_t *) lexer -> buffer)) {
    ret = str_replenish(lexer -> buffer, lexer -> reader);
    debug(lexer, "lexer_get_char: Read new chunk into buffer: %d", ret);
    if (ret <= 0) {
      lexer -> current = 0;
      return 0;
    } else {
      ch = str_readchar(lexer -> buffer);
      debug(lexer, "lexer_get_char: Read character from new chunk: '%c'", (ch > 0) ? ch : '$');
    }
  }
  lexer -> current = ch;
  if (lexer -> current) {
    lexer -> where = LexerWhereMiddle;
  }
  debug(lexer, "lexer_get_char(%c)", ch);
  return ch;
}

scanner_t * lexer_get_scanner(lexer_t *lexer, char *code) {
  scanner_t *scanner;

  for (scanner = lexer -> scanners; scanner; scanner = scanner -> next) {
    if (!strcmp(code, data_typename((data_t *) scanner -> config))) {
      return scanner;
    }
  }
  return NULL;
}

lexer_t * lexer_reconfigure_scanner(lexer_t *lexer, char *code, char *name, data_t *value) {
  scanner_t *scanner = lexer_get_scanner(lexer, code);

  return (scanner)
    ? ((scanner_reconfigure(scanner, name, value)) ? lexer : NULL)
    : NULL;
}
