/*
 * /obelix/src/lexer.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __LEXER_H__
#define __LEXER_H__

#include <array.h>
#include <data.h>
#include <dict.h>
#include <logging.h>
#include <str.h>
#include <token.h>

#define LEXER_BUFSIZE             16384
#define LEXER_INIT_TOKEN_SZ       256
#define LEXER_MAX_SCANNERS        32
#define SCANNER_CONFIG_SEPARATORS " \t,.;"

typedef enum _lexer_state {
  LexerStateNoState,
  LexerStateFresh,
  LexerStateParameter,
  LexerStateInit,
  LexerStateFirstPass,
  LexerStateSecondPass,
  LexerStateSuccess,
  LexerStateWhitespace,
  LexerStateNewLine,
  LexerStateIdentifier,
  LexerStateURIComponent,
  LexerStateURIComponentPercent,
  LexerStateKeyword,
  LexerStateMatchLost,
  LexerStatePlusMinus,
  LexerStateZero,
  LexerStateNumber,
  LexerStateDecimalInteger,
  LexerStateHexInteger,
  LexerStateFloat,
  LexerStateSciFloat,
  LexerStateQuotedStr,
  LexerStateQuotedStrEscape,
  LexerStateHashPling,
  LexerStateSlash,
  LexerStateBlockComment,
  LexerStateLineComment,
  LexerStateStar,
  LexerStateDone,
  LexerStateStale,
  LexerStateLAST
} lexer_state_t;

struct _lexer;
struct _lexer_config;
struct _scanner;
struct _scanner_config;

typedef token_t * (*matcher_t)(struct _scanner *);

typedef struct _scanner_config {
  data_t                   _d;
  struct _scanner_config  *prev;
  struct _scanner_config  *next;
  struct _lexer_config    *lexer_config;
  matcher_t                match;
  matcher_t                match_2nd_pass;
  dict_t                  *config;
} scanner_config_t;

typedef struct _scanner {
  data_t            _d;
  struct _scanner  *next;
  struct _scanner  *prev;
  scanner_config_t *config;
  struct _lexer    *lexer;
  int               state;
  void             *data;
} scanner_t;

typedef struct _lexer_config {
  data_t            _d;
  int               num_scanners;
  scanner_config_t *scanners;
  int               bufsize;
} lexer_config_t;

typedef struct _lexer {
  data_t           _d;
  lexer_config_t  *config;
  scanner_t       *scanners;
  data_t          *reader;
  str_t           *buffer;
  str_t           *lookahead;
  int              lookahead_live;
  str_t           *token;
  lexer_state_t    state;
  token_t         *last_token;
  char             quote;
  int              current;
  int              prev_char;
  int              line;
  int              column;
} lexer_t;

OBLLEXER_IMPEXP char *           lexer_state_name(lexer_state_t);

/*
 * ---------------------------------------------------------------------------
 * S C A N N E R  C O N F I G
 * ---------------------------------------------------------------------------
 */

OBLLEXER_IMPEXP int                scanner_config_typeid(void);
OBLLEXER_IMPEXP typedescr_t *      scanner_config_register(typedescr_t *);
OBLLEXER_IMPEXP typedescr_t *      scanner_config_get(char *);
OBLLEXER_IMPEXP scanner_config_t * scanner_config_create(char *, lexer_config_t *);
OBLLEXER_IMPEXP scanner_t *        scanner_config_instantiate(scanner_config_t *, lexer_t *);
OBLLEXER_IMPEXP scanner_config_t * scanner_config_configure(scanner_config_t *, data_t *);

/*
 * ---------------------------------------------------------------------------
 * S C A N N E R
 * ---------------------------------------------------------------------------
 */

OBLLEXER_IMPEXP scanner_t *        scanner_create(scanner_config_t *, lexer_t *);
OBLLEXER_IMPEXP token_code_t       scanner_match(scanner_t *);

/*
 * ---------------------------------------------------------------------------
 * L E X E R  C O N F I G
 * ---------------------------------------------------------------------------
 */

OBLLEXER_IMPEXP lexer_config_t *   lexer_config_create(void);
OBLLEXER_IMPEXP scanner_config_t * lexer_config_add_scanner(lexer_config_t *, char *);
OBLLEXER_IMPEXP lexer_config_t *   lexer_config_set_bufsize(lexer_config_t *, int);
OBLLEXER_IMPEXP int                lexer_config_get_bufsize(lexer_config_t *);
OBLLEXER_IMPEXP lexer_config_t *   lexer_config_tokenize(lexer_config_t *, reduce_t, data_t *);

/*
 * ---------------------------------------------------------------------------
 * L E X E R
 * ---------------------------------------------------------------------------
 */

OBLLEXER_IMPEXP lexer_t *        lexer_create(lexer_config_t *, data_t *);
OBLLEXER_IMPEXP token_t *        lexer_next_token(lexer_t *);
OBLLEXER_IMPEXP int              lexer_get_char(lexer_t *);
OBLLEXER_IMPEXP void             lexer_pushback(lexer_t *);
OBLLEXER_IMPEXP void             lexer_clear(lexer_t *);
OBLLEXER_IMPEXP void             lexer_flush(lexer_t *);
OBLLEXER_IMPEXP lexer_t *        lexer_reset(lexer_t *);
OBLLEXER_IMPEXP lexer_t *        lexer_rewind(lexer_t *);
OBLLEXER_IMPEXP token_t *        lexer_accept(lexer_t *, token_code_t);
OBLLEXER_IMPEXP token_t *        lexer_get_accept(lexer_t *, token_code_t, int);
OBLLEXER_IMPEXP lexer_t *        lexer_push(lexer_t *);
OBLLEXER_IMPEXP void *           _lexer_tokenize(lexer_t *, reduce_t, void *);
OBLLEXER_IMPEXP token_t *        lexer_rollup_to(lexer_t *, int);

OBLLEXER_IMPEXP void             lexer_init(void);

OBLLEXER_IMPEXP void             keywords_register(void);
OBLLEXER_IMPEXP void             whitespace_register(void);
OBLLEXER_IMPEXP void             identifier_register(void);

OBLLEXER_IMPEXP int LexerConfig;
OBLLEXER_IMPEXP int Lexer;
OBLLEXER_IMPEXP int ScannerConfig;
OBLLEXER_IMPEXP int Scanner;
OBLLEXER_IMPEXP int lexer_debug;

#define lexer_tokenize(l, r, d)    _lexer_tokenize((l), (reduce_t) (r), (d))

#define data_is_lexer_config(d)    ((d) && data_hastype((d), LexerConfig))
#define data_as_lexer_config(d)    (data_is_lexer_config((d)) ? ((lexer_config_t *) (d)) : NULL)
#define lexer_config_copy(l)       ((lexer_config_t *) data_copy((data_t *) (l)))
#define lexer_config_free(l)       (data_free((data_t *) (l)))
#define lexer_config_tostring(l)   (data_tostring((data_t *) (l)))

#define data_is_lexer(d)           ((d) && data_hastype((d), Lexer))
#define data_as_lexer(d)           (data_is_lexer((d)) ? ((lexer_t *) (d)) : NULL)
#define lexer_copy(l)              ((lexer_t *) data_copy((data_t *) (l)))
#define lexer_free(l)              (data_free((data_t *) (l)))
#define lexer_tostring(l)          (data_tostring((data_t *) (l)))

#define data_is_scanner_config(d)  ((d) && data_hastype((d), ScannerConfig))
#define data_as_scanner_config(d)  (data_is_scanner_config((d)) ? ((scanner_config_t *) (d)) : NULL)
#define scanner_config_copy(l)     ((scanner_config_t *) data_copy((data_t *) (l)))
#define scanner_config_free(l)     (data_free((data_t *) (l)))
#define scanner_config_tostring(l) (data_tostring((data_t *) (l)))

#define data_is_scanner(d)         ((d) && data_hastype((d), Scanner))
#define data_as_scanner(d)         (data_is_scanner((d)) ? ((scanner_t *) (d)) : NULL)
#define scanner_copy(l)            ((scanner_t *) data_copy((data_t *) (l)))
#define scanner_free(l)            (data_free((data_t *) (l)))
#define scanner_tostring(l)        (data_tostring((data_t *) (l)))

#endif /* __LEXER_H__ */
