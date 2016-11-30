/*
 * /obelix/src/lib/strutils.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libcore.h"

static int          rand_initialized = 0;
static char         charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY";
static char *       _false[] = { "f", "false","F", "FALSE", "False", NULL };
static unsigned int my_seed = 3425674;

static inline void _initialize_random(void);

/* ------------------------------------------------------------------------ */

void _initialize_random(void) {
  if (!rand_initialized++) {
    srand((unsigned int) time(NULL) + my_seed);
  }
}

/* ------------------------------------------------------------------------ */

unsigned int strhash(const char *str) {
  return hash(str, strlen(str));
}

/**
 * Convert the given string to a boolean int. As usual, 0 (zero) is false,
 * and any int != 0 is true.
 *
 * If:
 * - str is NULL, then false is returned,
 * - str can be parsed as a valid integer, then true is returned if this integer
 *   is != 0, and false it == 0,
 * - str is 'f', 'false', 'F', 'FALSE', 'False', then false is returned.
 * Else any string except the empty string is true.
 */
int atob(const char *str) {
  long val;
  int  ix;

  if (str) {
    if (!strtoint(str, &val)) {
      return (val != 0);
    }
    for (ix = 0; _false[ix]; ix++) {
      if (!strcmp(str, _false[ix])) {
        return FALSE;
      }
    }
    return (str[0] != 0);
  }
  return FALSE;
}

char * btoa(long b) {
  return (b) ? "true" : "false";
}

char * chars(const void *str) {
  return (char *) str;
}

int strtoint(const char *str, long *val) {
  char *endptr;
  char *ptr;

  /*
   * Using strtod here instead of strtol. strtol parses a number with a
   * leading '0' as octal, which nobody wants these days. Also less chance
   * of overflows.
   *
   * After a successful parse we check that the number is within bounds
   * and that it's actually an long, i.e. that it doesn't have a decimal
   * point or exponent.
   */
  *val = strtol(str, &endptr, 0);
  if ((*endptr == 0) || (isspace(*endptr))) {
    if ((*val < LONG_MIN) || (*val > LONG_MAX)) {
      *val = 0;
      return -1;
    }
    ptr = strpbrk(str, ".eE");
    if (ptr && (ptr < endptr)) {
      *val = 0;
      return -1;
    } else {
      return 0;
    }
  } else {
    *val = 0;
    return -1;
  }
}

#ifdef itoa
  #undef itoa
#endif
char * oblcore_itoa(long i) {
  static char buf[20];
#ifdef HAVE__ITOA
  _itoa(i, buf, 10);
#elif defined(HAVE_ITOA)
  itoa(i, buf, 10);
#else
  sprintf(buf, "%ld", i);
#endif
  return buf;
}

char * oblcore_dtoa(double d) {
  static char buf[20];
  snprintf(buf, 20, "%f", d);
  return buf;
}

int oblcore_strcasecmp(const char *s1, const char *s2) {
  while (*s1 && (toupper(*s1) && toupper(*s2))) {
    s1++;
    s2++;
  }
  return toupper(*s1) - toupper(*s2);
}

int oblcore_strncasecmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && (toupper(*s1) && toupper(*s2))) {
    n--;
    s1++;
    s2++;
  }
  return toupper(*s1) - toupper(*s2);
}

char * strrand(char *buf, size_t numchars) {
  size_t n;
  int key;

  _initialize_random();
  if (numchars) {
    if (!buf) {
      buf = (char *) _new(numchars + 1);
    }
    for (n = 0; n < numchars; n++) {
      key = rand() % (int) (sizeof(charset) - 1);
      buf[n] = charset[key];
    }
    buf[numchars] = 0;
  }
  return buf;
}

char * strltrim(char *str) {
  char *ret = str;

  if (ret && *ret) {
    while (*ret && isspace(*ret)) {
      ret++;
    }
  }
  return ret;
}

char * strrtrim(char *str) {
  char *ptr;

  if (str && *str) {
    for (ptr = str + (strlen(str) - 1); (ptr != str) && isspace(*ptr); ptr--) {
      *ptr = 0;
    }
  }
  return str;
}

char * strtrim(char *str) {
  char *ret = str;

  if (ret && *ret) {
    ret = strltrim(ret);
    if (*ret) {
      ret = strrtrim(ret);
    }
  }
  return ret;
}

char * escape(char *str, const char *escapeable, char escape_char) {
  int   count;
  char *escape;
  char *escaped;

  /*
   * By default, escape quotes and backslashes:
   */
  if (!escapeable) {
    escapeable = "\"\\";
  }

  if (!escape_char) {
    escape_char = '\\';
  }

  /*
   * Start by counting the escape characters so we can expand the buffer:
   */
  for (count = 0, escape = strpbrk(str, escapeable);
       escape;
       escape = strpbrk(escape + 1, escapeable)) {
    count++;
  }

  /* If there are characters to be escaped, escape them: */
  if (count) {
    /* Expand the buffer by the number to be escaped characters: */
    escaped = stralloc(strlen(str) + count + 1);
    strcpy(escaped, str);

    /*
     * Find escapeable characters. Shift the rest of the string one
     * to the right and paste a backslash into the hole.
     *
     * The number of characters to shift is the length of the remaining string
     * plus one for the zero.
     */
    for (escape = strpbrk(escaped, escapeable);
         escape;
         escape = strpbrk(escape + 2, escapeable)) {
      memmove(escape + 1, escape, strlen(escape) + 1);
      *escape = escape_char;
    }
  } else {
    escaped = strdup(str);
  }
  return escaped;
}

/**
 * Removes escape characters (typically '\', backslash) from a string.
 *
 * Because the result string will the same length or shorter than the input
 * string, this operation will take place in-place, i.e. the input buffer will
 * be modified and the return value is the same pointer as the input string
 * pointer.
 *
 * If the <tt>escape_char</tt> is non-null, every occurance of this character
 * is removed, except when escape_char is preceded by another escape_char
 * (i.e. "a\\a" is transformed into "a\a" when <tt>escape_char</tt< is '\'). If
 * <tt>escape_char</tt> is zero, the value of '\' is used.
 */
char * unescape(char *str, char escape_char) {
  char *escape;

  if (!escape_char) {
    escape_char = '\\';
  }

  for (escape = strchr(str, escape_char);
       escape && *(escape + 1);
       escape = strchr(escape + 1, escape_char)) {
    memmove(escape, escape + 1, strlen(escape));
  }
  return str;
}
