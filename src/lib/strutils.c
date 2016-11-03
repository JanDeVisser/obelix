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
static unsigned int my_seed = 3425674;

void _initialize_random(void) {
  if (!rand_initialized++) {
    srand((unsigned int) time(NULL) + my_seed);
  }
}

/* ------------------------------------------------------------------------ */

unsigned int strhash(char *str) {
  return hash(str, strlen(str));
}

/*
 * Note that we mirror python here: any string except the empty
 * string is true.
 */
int atob(char *str) {
  return str && str[0];
}

char * btoa(long b) {
  return (b) ? "true" : "false";
}

char * chars(void *str) {
  return (char *) str;
}

int strtoint(char *str, long *val) {
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

int oblcore_strcasecmp(char *s1, char *s2) {
  while (*s1 && (toupper(*s1) && toupper(*s2))) {
    s1++;
    s2++;
  }
  return toupper(*s1) - toupper(*s2);
}

int oblcore_strncasecmp(char *s1, char *s2, size_t n) {
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
