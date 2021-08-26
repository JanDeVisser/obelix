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
#include <stdio.h>

#include "liblexer.h"

/*
 * Initial size of the marker token buffer. I've never seen a comment marker
 * longer than 3 characters ('REM') so make it 4 just to be safe. One would
 * think that by taking the length of the longest registered marker we don't
 * have to juggle at all, but the advantage of this is that we can allocate
 * a buffer in the scanning struct without having to dynamically allocate it.
 */
#define SCANNER_INIT_BUFSZ    4
#define PARAM_MARKER          "marker"

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
  char                   *str;
} comment_marker_t;

typedef struct _comment_config {
  scanner_config_t  _sc;
  comment_marker_t *markers;
  size_t            longest_marker;
  int               num_markers;
} comment_config_t;

typedef struct _comment_scanner {
  int               num_matches;
  char             *token;
  char              token_buf[SCANNER_INIT_BUFSZ];
  size_t            bufsz;
  char             *newbuf;
  size_t            len;
  comment_marker_t *match;
  int               matched[0];
} comment_scanner_t;

static comment_marker_t * _comment_marker_create(void);
static void               _comment_marker_free(comment_marker_t *);
static char *             _comment_marker_tostring(comment_marker_t *);
static comment_config_t * _comment_config_create(comment_config_t *, va_list);

static data_t *           _comment_config_resolve(comment_config_t *, char *);
static void               _comment_config_free(comment_config_t *);
static comment_config_t * _comment_config_set(comment_config_t *, char *, data_t *);
static comment_config_t * _comment_config_config(comment_config_t *, array_t *);
static comment_config_t * _comment_config_add_marker(comment_config_t *, char *);

static token_t *          _comment_match(scanner_t *);
static void               _comment_free_scanner(comment_scanner_t *);

static vtable_t _vtable_CommentScannerConfig[] = {
    { .id = FunctionNew,            .fnc = (void_t) _comment_config_create },
    { .id = FunctionFree,           .fnc = (void_t) _comment_config_free },
    { .id = FunctionResolve,        .fnc = (void_t) _comment_config_resolve },
    { .id = FunctionSet,            .fnc = (void_t) _comment_config_set },
    { .id = FunctionMatch,          .fnc = (void_t) _comment_match },
    { .id = FunctionDestroyScanner, .fnc = (void_t) _comment_free_scanner },
    { .id = FunctionGetConfig,      .fnc = (void_t) _comment_config_config },
    { .id = FunctionNone,           .fnc = NULL }
};

static int CommentScannerConfig = -1;

/*
 * ---------------------------------------------------------------------------
 * C O M M E N T  S C A N N E R  F U N C T I O N S
 * ---------------------------------------------------------------------------
 */

comment_marker_t * _comment_marker_create(void) {
  comment_marker_t *ret;

  ret = NEW(comment_marker_t);
  ret -> hashpling = 0;
  ret -> start = NULL;
  ret -> end = NULL;
  ret -> str = NULL;
  return ret;
}

void _comment_marker_free(comment_marker_t *marker) {
  if (marker) {
    free(marker -> start);
    free(marker -> end);
    free(marker -> str);
    free(marker);
  }
}

char * _comment_marker_tostring(comment_marker_t *marker) {
  if (!marker -> str) {
    asprintf(&marker -> str, "%s%s%s%s", marker -> hashpling ? "^" : "",
                             marker -> start,
                             (marker -> end) ? " " : "",
                             (marker -> end) ? marker -> end : "");
  }
  return marker -> str;
}

comment_config_t *_comment_config_create(comment_config_t *config,
                                         va_list args) {
  config -> _sc.priority = 20;
  config -> markers = NULL;
  config -> longest_marker = 0;
  config -> num_markers = 0;
  return config;
}

void _comment_config_free(comment_config_t *config) {
  comment_marker_t *marker;
  comment_marker_t *next;

  if (config) {
    for (marker = config -> markers; marker; marker = next) {
      next = marker->next;
      _comment_marker_free(marker);
    }
  }
}

comment_config_t * _comment_config_set(comment_config_t *config,
                                       char *name, data_t *value) {
  if (!strcmp(PARAM_MARKER, name)) {
    return _comment_config_add_marker(config, data_tostring(value));
  } else {
    return NULL;
  }
}

comment_config_t *_comment_config_add_marker(comment_config_t *config,
                                             char *marker) {
  char             *start;
  char             *end;
  char             *start_endptr = NULL;
  char             *end_endptr = NULL;
  int               hashpling = 0;
  comment_marker_t *comment_marker = NULL;

  debug(lexer, "Parsing comment marker '%s'", marker);
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

  if (!marker) {
    return config;
  }
  marker = strdup(marker);

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
    for (start++; *start && isspace(*start); start++);
    if (!*start) {
      /* No non-whitespace found. Bail: */
      return NULL;
    }
    hashpling = 1;
  }
  /* Find the end of the start marker: */
  for (start_endptr = start + 1;
       *start_endptr && !isspace(*start_endptr);
       start_endptr++);

  if (*start_endptr) {
    /*
     * There is whitespace after the start marker. Find the end of the
     * whitespace sequence:
     */
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
  comment_marker = _comment_marker_create();
  comment_marker -> hashpling = hashpling;
  comment_marker -> start = strdup(start);
  comment_marker -> end = (end) ? strdup(end) : NULL;
  comment_marker -> next = config -> markers;
  debug(lexer, "Created comment marker [%s]", _comment_marker_tostring(comment_marker));
  config -> markers = comment_marker;
  config -> num_markers++;
  if (strlen(start) > config -> longest_marker) {
    config -> longest_marker = strlen(start);
  }
  if (end && (strlen(start) > config -> longest_marker)) {
    config -> longest_marker = strlen(end);
  }
  free(marker);
  return config;
}

data_t * _comment_config_resolve(comment_config_t *config, char *name) {
  datalist_t       *markers;
  comment_marker_t *marker;

  if (!strcmp(name, PARAM_MARKER)) {
    markers = datalist_create(NULL);
    for (marker = config -> markers; marker; marker = marker -> next) {
      datalist_push(markers, (data_t *) str(_comment_marker_tostring(marker)));
    }
    return (data_t *) markers;
  } else {
    return NULL;
  }
}

comment_config_t * _comment_config_config(comment_config_t *config, array_t *cfg) {
  comment_marker_t *marker;

  for (marker = config -> markers; marker; marker = marker -> next) {
    array_push(cfg, nvp_create(str_to_data(PARAM_MARKER),
                               str_to_data(_comment_marker_tostring(marker))));
  }
  return config;
}

/* ---------------------------------------------------------------------- - */

token_t * _comment_find_eol(scanner_t *scanner) {
  token_t *ret = NULL;
  int      ch;

  debug(lexer, "_comment_find_eol");
  for (ch = lexer_get_char(scanner -> lexer); scanner -> state == CommentText;) {
    if (!ch || (ch == '\r') || (ch == '\n')) {
      /* Do not discard - this is part of the next token. */
      scanner -> state = CommentNone;
      lexer_skip(scanner -> lexer);
    } else {
      lexer_discard(scanner -> lexer);
      ch = lexer_get_char(scanner -> lexer);
    }
  }
  return ret;
}

token_t * _comment_find_endmarker(scanner_t *scanner) {
  token_t           *ret = NULL;
  comment_scanner_t *c_scanner = (comment_scanner_t *) scanner -> data;
  comment_marker_t  *marker = c_scanner -> match;
  lexer_t           *lexer = scanner -> lexer;
  int                ch;

  debug(lexer, "_comment_find_endmarker: %s", marker -> end);
  for (ch = lexer_get_char(lexer); ch && (scanner -> state != CommentNone);) {

    lexer_discard(scanner -> lexer);

    switch (scanner -> state) {
      case CommentText:
        if (ch == *(marker -> end)) {
          scanner -> state = CommentEndMarker;
          memset(c_scanner -> token, 0, c_scanner -> bufsz);
          c_scanner -> token[0] = (char) ch;
          c_scanner -> len = 1;
        }
        ch = lexer_get_char(scanner -> lexer);
        break;

      case CommentEndMarker:
        /*
         * If our token buffer is running out expand it:
         */
        if (c_scanner -> len >= c_scanner -> bufsz - 1) {
          c_scanner -> bufsz *= 2;
          c_scanner -> newbuf = (char *) _new(c_scanner -> bufsz);
          strcpy(c_scanner -> newbuf, c_scanner -> token);
          c_scanner -> token = c_scanner -> newbuf;
        }

        /*
         * Append current character to our token:
         */
        c_scanner -> token[c_scanner -> len++] = (char) ch;

        if (strncmp(c_scanner -> token, marker -> end, c_scanner -> len)) {
          /*
           * The match of the end marker was lost. Reset the state. It's possible
           * though that this is the start of a new end marker match though.
           */
          scanner -> state = CommentText;
          ch = lexer_get_char(scanner -> lexer);
        } else if (c_scanner -> len == strlen(marker -> end)) {
          /*
           * We matched the full end marker. Set the state of the scanner.
           */
          scanner -> state = CommentNone;
          lexer_accept_token(lexer, NULL);
        } else {
          /*
           * Still matching the end marker. Read next character:
           */
          ch = lexer_get_char(scanner -> lexer);
        }
        break;
      default:
        break;
    }
  }
  if (!ch) {
    ret = token_create(TokenCodeError, "Unterminated comment");
    lexer_accept_token(scanner -> lexer, ret);
  }
  return ret;
}

token_t *_comment_match(scanner_t *scanner) {
  token_t            *ret = NULL;
  int                 ix;
  int                 ch;
  comment_config_t   *config = (comment_config_t *) scanner -> config;
  comment_scanner_t  *c_scanner = (comment_scanner_t *) scanner -> data;
  comment_marker_t   *marker;
  lexer_t            *lexer = scanner -> lexer;
  int                 at_top;

  debug(lexer, "_comment_match markers: %s",
                data_tostring(data_get_attribute((data_t *) config, "marker")));

  if (!c_scanner) {
    c_scanner = NEWDYNARR(comment_scanner_t, config -> num_markers, int);
    scanner -> data = c_scanner;
    c_scanner -> bufsz = SCANNER_INIT_BUFSZ;
    c_scanner -> token = c_scanner -> token_buf;
    c_scanner -> newbuf = NULL;
  }
  memset(c_scanner -> token, 0, c_scanner -> bufsz);
  c_scanner -> len = 0;

  scanner -> state = CommentNone;
  at_top = lexer_at_top(lexer);
  for (ix = 0; ix < config -> num_markers; ix++) {
    c_scanner -> matched[ix] = 1;
  }
  for (scanner -> state = CommentStartMarker,
         ch = lexer_get_char(scanner -> lexer),
         c_scanner -> num_matches = config -> num_markers;
       ch && scanner -> state != CommentNone;
      ) {

    /*
     * Whatever happens we're not going to need the character anymore:
     */
    lexer_discard(lexer);

    /*
     * If our token buffer is running out expand it:
     */
    if (c_scanner -> len >= c_scanner -> bufsz - 1) {
      c_scanner -> bufsz *= 2;
      c_scanner -> newbuf = (char *) _new(c_scanner -> bufsz);
      strcpy(c_scanner -> newbuf, c_scanner -> token);
      c_scanner -> token = c_scanner -> newbuf;
    }

    /*
     * Append current character to our token:
     */
    c_scanner -> token[c_scanner -> len++] = (char) ch;

    for (ix = 0, marker = config -> markers, c_scanner -> num_matches = 0;
         ix < config -> num_markers;
         ix++, marker = marker -> next) {
      if (marker -> hashpling && !at_top) {
        continue;
      }
      if (c_scanner -> matched[ix]) {
        // Intentional assignment!
        if ((c_scanner -> matched[ix] = !strncmp(c_scanner -> token,
                                                 marker -> start,
                                                 c_scanner -> len))) {
          c_scanner -> num_matches++;
          c_scanner -> match = marker;
        }
      }
    }
    if (c_scanner -> num_matches != 1) {
      c_scanner -> match = NULL;
    }
    if ((c_scanner -> num_matches == 1) &&
        !strcmp(c_scanner -> token, c_scanner -> match -> start)) {
      debug(lexer, "Full match of comment start marker '%s'", c_scanner -> match -> start);
      scanner -> state = CommentText;
      ret = (c_scanner -> match -> end)
              ? _comment_find_endmarker(scanner)
              : _comment_find_eol(scanner);
    } else if (c_scanner -> num_matches > 0) {
      debug(lexer, "Matching '%d' comment start markers", c_scanner -> num_matches);
      ch = lexer_get_char(scanner -> lexer);
    } else {
      scanner -> state = CommentNone;
    }
  }
  return ret;
}

void _comment_free_scanner(comment_scanner_t *c_scanner) {
  free(c_scanner -> newbuf);
}

extern typedescr_t *comment_register(void) {
  typedescr_register_with_name(CommentScannerConfig, "comment", comment_config_t);
  return typedescr_get(CommentScannerConfig);
}
