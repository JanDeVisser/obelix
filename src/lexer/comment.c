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

#include <ctype.h>

#include <lexer.h>

typedef enum {
  CommentNone,
  CommentStartMarker,
  CommentText,
  CommentEndMarker,
  CommentEnd,
  CommentUnterminated
} comment_state_t;

typedef struct _comment_marker {
  struct _comment_marker *next;
  int                     hashpling;
  char                   *start;
  char                   *end;
} comment_marker_t;

typedef struct _comment_config {
  data_t            _d;
  comment_marker_t *markers;
  int               num_markers;
} comment_config_t;

typedef struct _comment_scanner {
  int               num_matches;
  comment_marker_t *match;
  int               matched[0];
} comment_scanner_t;

static comment_config_t * _comment_config_create(comment_config_t *, va_list);
static data_t *           _comment_config_resolve(comment_config_t *, char *);
static ws_config_t *      _comment_config_set(comment_config_t *, char *, data_t *);
static token_t *          _comment_match(scanner_t *);

static vtable_t _vtable_comment_config[] = {
    { .id = FunctionNew,     .fnc = (void_t) _comment_config_create },
    { .id = FunctionResolve, .fnc = (void_t) _comment_config_resolve },
    { .id = FunctionSet,     .fnc = (void_t) _comment_config_set },
    { .id = FunctionUsr1,    .fnc = (void_t) _comment_match },
    { .id = FunctionUsr2,    .fnc = NULL },
    { .id = FunctionNone,    .fnc = NULL }
};

static int CommentScannerConfig = -1;

/*
 * ---------------------------------------------------------------------------
 * C O M M E N T  S C A N N E R  F U N C T I O N S
 * ---------------------------------------------------------------------------
 */

comment_config_t *_comment_config_create(comment_config_t *config,
                                         va_list args) {
  config -> markers = NULL;
  config -> num_markers = 0;
  return config;
}

void _comment_config_free(comment_config_t *config) {
  comment_marker_t *marker;
  comment_marker_t *last;

  if (config) {
    for (marker = config -> markers; marker; marker = last -> next) {
      last = marker;
      free(marker -> start);
      free(marker -> end);
      free(marker);
    }
  }
}

comment_config_t *_comment_config_add_marker(comment_config_t *config,
                                             char *marker) {
  char             *start = marker;
  char             *end;
  char             *ptr;
  char             *start_endptr = NULL
  char             *end_endptr = NULL;
  int               ch1 = 0;
  int               ch2 = 0;
  int               hashpling = 0;
  comment_marker_t *comment_marker = NULL;

  /*
   * Marker should be a string consisting of start marker and end maker 
   * separated by whitespace. If there is no end marker the comment is a 
   * line comment, i.e. ends at the end of the line (c.f. //, #).
   * 
   * So valid marker strings are for example "[[ ]]" for a block comment marked 
   * by double square brackets or "#" for line comments marked by a hash sign.
   * 
   * Additionally, the string can be preceded by a '^' sign to indicate that 
   * the sequence only marks a comment at the beginning of the text. This is
   * to accommodate shell hashplings (#!): the string for allowing hashplings 
   * but not # as an end of line comment marker in the rest of the text would 
   * be "^#!".
   */

  /* Find the first non-whitespace character: */
  for (start = marker; *start && isspace(*start); start++);

  if (!*start) {
    /* No non-whitespace found. Bail: */
    return NULL;
  }

  if (*start == '^') {
    /* 
     * First non-whitespace character is the start-of-text anchor. Set the
     * hashpling attribute in the marker structure and try to find next 
     * non-whitespace char:
     */
    if (!*start) {
      /* No non-whitespace found. Bail: */
      return NULL;
    }
    hashpling = 1;
  }
  comment_marker = NEW(comment_marker_t);
  comment_marker -> hashpling = hashpling;

  /* Find the end of the start marker: */
  for (start_endptr = start + 1;
       *start_endptr && !isspace(*start_endptr);
       start_endptr++);

  if (*start_endptr) {
    /* 
     * There is whitespace after the start marker. Temporarily end the string,
     * and find the end of the whitespace sequence:
     */
    ch1 = *start_endptr;
    *start_endptr = 0;
    for (end = start_endptr + 1; *end && isspace(*end); end++);

    if (*end) {
      /* 
       * There is non-whitespace after the whitespace following the start 
       * marker. This is the start of the end marker. Find the end of it.
       */
      for (end_endptr = end + 1;
           *end_endptr && !isspace(*end_endptr); end_endptr++);
      if (*end_endptr) {
        /* There is trailing whitespace. Temporarily trim it: */
        ch2 = *end_endptr;
        *end_endptr = 0;
      }
    } else {
      /*
       * There is no non-whitespace after the start marker. Treat this as a line
       * comment.
       */
      end = NULL;
    }
  } else {
    /* There was nothing after the start marker. This is a line comment. */
    end = NULL;
  }

  /* Set up the marker structure: */
  comment_marker -> start = strdup(start);
  comment_marker -> end = (end) ? strdup(end) : NULL;
  comment_marker -> next = config -> markers;
  config -> markers = comment_marker;
  config -> num_markers++;

  /* Reset temporary end-of-string markers: */
  if (start_endptr) {
    *start_endptr = ch1;
  }
  if (end_endptr) {
    *end_endptr = ch2;
  }

  return c_scanner;
}

token_t *_comment_find_eol(scanner_t *c_scanner) {
  token_t           *ret = NULL;
  comment_config_t  *config = (comment_config_t *) scanner -> config;
  comment_scanner_t *c_scanner = (comment_scanner_t *) scanner -> data;
  int ch;

  for (ch = lexer_get_char(
      scanner -> lexer), scanner -> state = CommentStartMarker;
       ch && (scanner -> state == CommentText);) {
    if ((ch == '\r') || (ch == '\n')) {
      lexer_reset(scanner -> lexer);
      scanner -> state = CommentEnd;
    } else {
      lexer_discard(scanner -> lexer);
      ch = lexer_get_char(scanner -> lexer);
    }
  }
  return ret;
}

token_t *_comment_find_endmarker(scanner_t *scanner) {
  token_t           *ret = NULL;
  comment_config_t  *config = (comment_config_t *) scanner -> config;
  comment_scanner_t *c_scanner = (comment_scanner_t *) scanner -> data;
  comment_marker_t  *marker = c_scanner -> match;
  int                ch;
  int                len;
  char              *token;

  for (ch = lexer_get_char(scanner -> lexer), scanner -> state = CommentStartMarker;
       ch && (scanner -> state != CommentEnd);) {

    switch (scanner -> state) {
      case CommentText:
        if (lexer -> current == *(marker -> end)) {
          lexer_reset(scanner -> lexer);
          scanner -> state = CommentEndMarker;
        } else {
          lexer_discard(scanner -> lexer);
          ch = lexer_get_char(scanner -> lexer);
        }
        break;

      case CommentEndMarker:
        str_append_char(lexer -> token, ch);
        token = str_chars(lexer -> token);
        len = strlen(token);
        if (strncmp(token, marker -> end, len)) {
          /*
           * The match of the end marker was lost. Reset the state. It's possible
           * though that this is the start of a new end marker match though.
           */
          c_scanner -> state = CommentText;
          lexer_reset(scanner -> lexer);
        } else if (len == strlen(marker -> end)) {
          /*
           * We matched the full end marker. Set the state of the scanner and
           * erase the current token.
           */
          c_scanner -> state = CommentEnd;
        }
        break;
    }
  }
  if (!ch) {
    ret = token_create(TokenCodeError, "Unterminated comment");
  }
  return ret;
}

token_t *_comment_match_start(scanner_t *scanner) {
  token_t            *ret = NULL;
  int                 ix;
  int                 ch;
  comment_config_t   *config = (comment_config_t *) scanner -> config;
  comment_scanner_t  *c_scanner = (comment_scanner_t *) scanner -> data;
  comment_marker_t   *marker;
  int                 at_top;

  at_top = lexer_at_top(lexer);
  for (ix = 0; ix < config -> num_markers; ix++) {
    c_scanner -> matched[ix] = 1;
  }
  for (ch = lexer_get_char(scanner -> lexer), c_scanner -> num_matches = config -> num_markers;
       ch && c_scanner -> num_matches > 0;
      ) {

    token = str_chars(lexer -> token);
    len = strlen(token);
    c_scanner -> num_matches = 0;
    marker = config -> markers;
    for (ix = 0; ix < c_scanner -> num_markers; ix++, marker = marker -> next) {
      if (marker -> hashpling && !at_top) {
        continue;
      }
      if (c_scanner -> matched[ix]) {
        // Intentional assignment!
        if (c_scanner -> matched[ix] = !strncmp(token, marker -> start, len)) {
          c_scanner -> num_matches++;
          c_scanner -> match = marker;
        }
      }
    }
    if (c_scanner -> num_matches != 1) {
      c_scanner -> match = NULL;
    }
    if ((c_scanner -> num_matches == 1) &&
        !strcmp(token, c_scanner -> match -> start)) {
      scanner -> state = CommentText;
      lexer_reset(scanner -> lexer);
    } else if (c_scanner -> num_matches > 0) {
      scanner -> state = CommentStartMarker;
      lexer_push(scanner -> lexer);
      ch = lexer_get_char(scanner -> lexer);
    } else {
      scanner -> state = CommentNone;
    }
  }
  if (scanner -> state == CommentText) {
    ret = (c_scanner -> match -> end)
            ? _commment_find_endmarker(scanner)
            : _comment_find_eol(scanner);
  }
  return ret;
}

comment_scanner_t *_comment_set(comment_scanner_t *c_scanner, char *parameter, void *value) {
  return _comment_add_marker(c_scanner, (char *) value);
}

token_t *_comment_match(scanner_t *scanner) {
  token_t *ret = NULL;
  comment_config_t *config = (comment_config_t *) scanner -> config;

  mdebug(lexer, "_comment_match");
  scanner -> state = CommentNone;
  if (!scanner -> data) {
    scanner -> data = NEWDYNARR(comment_scanner_t, config -> num_markers, int);
  }
  return _comment_match_start(scanner);
}

typedescr_t *comment_register(void) {
  typedescr_t *ret;

  CommentScannerConfig = typedescr_create_and_register(
      CommentScannerConfig, "comment", _vtable_comment_config, NULL);
  ret = typedescr_get(CommentScannerConfig);
  typedescr_set_size(CommentScannerConfig, comment_config_t);
  return ret;
}
