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
#include <ctype.h>

#include <lexer.h>

#define MAX_MARKERS    16

typedef enum {
  CommentNone,
  CommentStartMarker,
  CommentText,
  CommentEndMarker,
  CommentEnd
} comment_state_t;

typedef struct _comment_marker {
  int   hashpling;
  char *start;
  char *end;
  int   matched;
} comment_marker_t;

typedef struct _comment_scanner {
  comment_marker_t  markers[MAX_MARKERS];
  int               num_markers;
  int               num_matches;
  comment_marker_t *match;
  comment_state_t   state;
} comment_scanner_t;

static         comment_scanner_t * _comment_create(void);
static         void                _comment_free(comment_scanner_t *);
static         comment_scanner_t * _comment_add_marker(comment_scanner_t *, char *);
static         int                 _comment_match_start(comment_scanner_t *, lexer_t *);
static         void                _comment_find_eol(comment_scanner_t *, lexer_t *);
static         void                _comment_find_endmarker(comment_scanner_t *, lexer_t *);
static         void                _comment_find_end(comment_scanner_t *, lexer_t *);
static         comment_scanner_t * _comment_set(comment_scanner_t *, char *, void *);
__DLL_EXPORT__ token_code_t        scanner_comment(lexer_t *, scanner_t *);

/*
 * ---------------------------------------------------------------------------
 * C O M M E N T  S C A N N E R  F U N C T I O N S
 * ---------------------------------------------------------------------------
 */

comment_scanner_t * _comment_create(void) {
  comment_scanner_t *ret;
 
  ret = NEW(comment_scanner_t);
  memset(ret -> markers, 0, sizeof(ret -> markers));
  ret -> num_markers = 0;
  ret -> state = CommentNone;
  return ret;
}

void _comment_free(comment_scanner_t *c_scanner) {
  int ix;
  
  if (c_scanner) {
    for (ix = 0; ix < c_scanner -> num_markers; ix++) {
      free(c_scanner -> markers[ix].start);
      free(c_scanner -> markers[ix].end);
    }
    free(c_scanner);
  }
}

comment_scanner_t * _comment_add_marker(comment_scanner_t *c_scanner, char *marker) {
  char *start = marker;
  char *end;
  char *ptr;
  char *start_endptr = NULL, end_endptr = NULL;
  int   ch1 = 0, ch2 = 0;
  
  if (c_scanner -> num_markers >= MAX_MARKERS) {
    return NULL;
  }
  
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
    c_scanner -> markers[c_scanner -> num_markers].hashpling = 1;
    for (start++; *start && isspace(*start); start++);
    
    if (!*start) {
      /* No non-whitespace found. Bail: */
      return NULL;
    }  
  } else {
    c_scanner -> markers[c_scanner -> num_markers].hashpling = 0;
  }
    
  /* Find the end of the start marker: */
  for (start_endptr = start + 1; *start_endptr && !isspace(*start_endptr); start_endptr++);
  
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
      for (end_endptr = end + 1; *end_endptr && !isspace(*end_endptr); end_endptr++);
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
  c_scanner -> markers[c_scanner -> num_markers].start = strdup(start);
  c_scanner -> markers[c_scanner -> num_markers].end = (end) 
    ? strdup(end) : NULL;
  c_scanner -> num_markers++;
  
  /* Reset temporary end-of-string markers: */
  if (start_endptr) {
    *start_endptr = ch1;
  }
  if (end_endptr) {
    *end_endptr = ch2;
  }
  
  return c_scanner;
}

int _comment_match_start(comment_scanner_t *c_scanner, lexer_t *lexer) {
  int               ix;
  int               len;
  char             *token;
  comment_marker_t *marker;
  int               at_top;
  
  at_top = (lexer -> line == 1) && (lexer -> column == 1);
  for (ix = 0; ix < c_scanner -> num_markers; ix++) {
    c_scanner -> markers[ix].matched = 1;
  }
  do {
    token = str_chars(lexer -> token);
    len = strlen(token);
    c_scanner -> num_matches = 0;
    for (ix = 0; ix < c_scanner -> num_markers; ix++) {
      marker = c_scanner -> markers + ix;
      if (marker -> hashpling && !at_top) {
        continue;
      }
      if (marker -> matched) {
        marker -> matched = !strncmp(token, marker -> start, len);
        if (marker -> matched) {
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
      c_scanner -> state = CommentText;
    } else if (c_scanner -> num_matches > 0) {
      c_scanner -> state = CommentStartMarker;
      lexer_get_char(lexer);
    } else {
      c_scanner -> state = CommentNone;
    }
  } while (c_scanner -> state == CommentStartMarker);
}

void _comment_find_eol(comment_scanner_t *c_scanner, lexer_t *lexer) {
  switch (c_scanner -> state) {
    case CommentText:
      if ((lexer -> current == '\r') || (lexer -> current == '\n')) {
        c_scanner -> state = CommentEndMarker;
      }
      break;
      
    case CommentEndMarker:
      if ((lexer -> current != '\r') && (lexer -> current != '\n')) {
        c_scanner -> state = CommentNone;
        lexer_clear(lexer);
        lexer_push_back(lexer);
      }
      break;
  }
}

void _comment_find_endmarker(comment_scanner_t *c_scanner, lexer_t *lexer) {
  int               len;
  char             *token;
  comment_marker_t *marker = c_scanner -> match;

  switch (c_scanner -> state) {
    case CommentText:
      if (lexer -> current == *(marker -> end)) {
        lexer_clear(lexer);
        str_append_char(lexer -> token, *(marker -> end));
        lexer -> current = *(marker -> end);
        c_scanner -> state = CommentEndMarker;
      }
      break;
      
    case CommentEndMarker:
      token = str_chars(lexer -> token);
      len = strlen(token);
      if (strncmp(token, marker -> end, len)) {
        /*
         * The match of the end marker was lost. Reset the state. It's possible
         * though that this is the start of a new end marker match, so
         * feed it through this function again.
         */
        c_scanner -> state = CommentText;
        _comment_find_endmarker(c_scanner, lexer);
      } else if (len == strlen(marker -> end)) {
        /*
         * We matched the full end marker. Set the state of the scanner and
         * erase the current token.
         */
        c_scanner -> state = CommentNone;
        lexer_clear(lexer);
      }
      break;
  }
}

comment_scanner_t * _comment_find_end(comment_scanner_t *c_scanner, lexer_t *lexer) {
  comment_marker_t  *marker = c_scanner -> match;
  void             (*endfinder)(comment_scanner_t *, lexer_t *); 
  
  endfinder = (marker -> end) ? _comment_find_endmarker : _comment_find_eol;
  do {
    if (lexer -> current < 0) {
      return NULL;
    }
    endfinder(c_scanner, lexer);
    if (c_scanner -> state != CommentNone) {
      lexer_get_char(lexer);
    }
  } while (c_scanner -> state != CommentNone);
  return c_scanner;
}

comment_scanner_t * _comment_set(comment_scanner_t *c_scanner, char *parameter, void *value) {
  return _comment_add_marker(c_scanner, (char *) value);
}

__DLL_EXPORT__ token_code_t scanner_comment(lexer_t *lexer, scanner_t *scanner) {
  token_code_t       code = TokenCodeNone;
  comment_scanner_t *c_scanner = (comment_scanner_t *) scanner -> data;
  int                ptr;
  
  switch (lexer -> state) {
    case LexerStateFresh:
      scanner -> data = _comment_create();
      break;
    
    case LexerStateParameter:
      _comment_set(c_scanner, scanner -> parameter, scanner -> value);
      break;
      
    case LexerStateStale:
      _comment_free(c_scanner);
      scanner -> data = NULL;
      break;
      
    case LexerStateInit:
      ptr = lexer_get_pointer(lexer);
      _comment_match_start(c_scanner, lexer);
      switch (c_scanner -> state) {
        case CommentNone:
          lexer_push_back_to(lexer, ptr);
          break;
        case CommentText:
          if (!_comment_find_end(c_scanner, lexer)) {
            lexer -> error = str_wrap("Unterminated comment");
            code = TokenCodeError;
          }
          break;
      }
      break;
  }
  return code;
}

