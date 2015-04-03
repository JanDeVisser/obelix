/*
 * core.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <core.h>
#include <resolve.h>

static void    __init(void) __attribute__((constructor));
static void    _outofmemory(int, siginfo_t *, void *);

#ifdef NDEBUG
log_level_t log_level = LogLevelInfo;
#else /* ! NDEBUG */
log_level_t log_level = LogLevelDebug;
#endif

static void _outofmemory(int sig, siginfo_t *siginfo, void *context) {
  error("Out of Memory. Terminating...");
  exit(1);
}

void __init(void) {
  struct sigaction act;

  memset (&act, 0, sizeof(act));

  /* Use the sa_sigaction field because the handles has two additional parameters */
  act.sa_sigaction = &_outofmemory;

  /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
  act.sa_flags = SA_SIGINFO;

  if (sigaction(SIGUSR1, &act, NULL) < 0) {
    error("Could not install SIGUSR signal handler. Bailing...");
    exit(1);
  }
}


void * new(int sz) {
  void *ret;

  ret = malloc(sz);
  if (sz && !ret) {
    kill(0, SIGUSR1);
  } else {
    memset(ret, 0, sz);
  }
  return ret;
}

void * new_array(int num, int sz) {
  void * ret = calloc(num, sz);

  if (num && !ret) {
    kill(0, SIGUSR1);
  }
  return ret;
}

void * new_ptrarray(int num) {
  return new_array(num, sizeof(void *));
}

void * resize_block(void *block, int newsz, int oldsz) {
  void *ret;

  if (block) {
    ret = realloc(block, newsz);
    if (newsz && !ret) {
      kill(0, SIGUSR1);
    } else if (oldsz) {
      memset(ret + oldsz, 0, newsz - oldsz);
    }
  } else {
    ret = new(newsz);
  }
  return ret;
}

void * resize_ptrarray(void *array, int newsz, int oldsz) {
  return resize_block(array, newsz * sizeof(void *), oldsz * sizeof(void *));
}

#ifndef HAVE_ASPRINTF
int asprintf(char **strp, const char *fmt, ...) {
  va_list args;
  int     ret;
  
  va_start(args, fmt);
  ret = vasprintf(strp, fmt, args);
  va_end(args);
  return ret;
}
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char **strp, const char *fmt, va_list args) {
  va_list  copy;
  int      ret;
  char    *str;
  
  va_copy(copy, args);
  ret = vsnprintf(NULL, 0, fmt, copy)
  va_end(copy);
  str = (char *) new(ret + 1);
  if (!str) {
    ret = -1;
  } else {
    vsnprintf(str, ret, fmt, args);
    *strp = str;
  }
  return ret;
}
#endif 

unsigned int hash(void *buf, size_t size) {
  int hash = 5381;
  int i;
  int c;
  unsigned char *data = (unsigned char *) buf;

  for (i = 0; i < size; i++) {
    c = *data++;
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

unsigned int hashptr(void *ptr) {
  return hash(&ptr, sizeof(void *));
}

unsigned int hashlong(long val) {
  return hash(&val, sizeof(long));
}

unsigned int hashdouble(double val) {
  return hash(&val, sizeof(double));
}

unsigned int hashblend(unsigned int h1, unsigned int h2) {
  return (h1 << 1) + h1 + h2;
}

static code_label_t _log_level_labels[] = {
  { .code = LogLevelDebug,   .label = "DEBUG" },
  { .code = LogLevelInfo,    .label = "INFO" },
  { .code = LogLevelWarning, .label = "WARN" },
  { .code = LogLevelError,   .label = "ERROR" },
  { .code = LogLevelFatal,   .label = "FATAL" }
};

char * _log_level_str(log_level_t lvl) {
  assert(_log_level_labels[lvl].code == lvl);
  return _log_level_labels[lvl].label;
}

void _logmsg(log_level_t lvl, char *file, int line, char *msg, ...) {
  va_list args;

  if (lvl >= log_level) {
    va_start(args, msg);
    fprintf(stderr, "%-12.12s:%4d:%-5.5s:", file, line, _log_level_str(lvl));
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
}

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

char * itoa(long i) {
  static char buf[20];
  sprintf(buf, "%ld", i);
  return buf;
}

char * dtoa(double d) {
  static char buf[20];
  sprintf(buf, "%f", d);
  return buf;
}

static int rand_initialized = 0;
static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
static int my_seed = 3425674;

void initialize_random(void) {
  if (!rand_initialized++) {
    srand(time(NULL) + my_seed);
  }
}

char * strrand(char *buf, size_t numchars) {
  size_t n;
  int key;
  
  initialize_random();
  if (numchars) {
    for (n = 0; n < numchars; n++) {
      key = rand() % (int) (sizeof charset - 1);
      buf[n] = charset[key];
    }
    buf[numchars] = '\0';
  }
  return buf;
}

/*
 * code_label_t public functions
 */

char * label_for_code(code_label_t *table, int code) {
  assert(table);
  while (table -> label) {
    if (table -> code == code) {
      return table -> label;
    } else {
      table++;
    }
  }
  return NULL;
}

int code_for_label(code_label_t *table, char *label) {
  assert(table);
  assert(label);
  while (table -> label) {
    if (!strcmp(table -> label, label)) {
      return table -> code;
    } else {
      table++;
    }
  }
  return -1;
}


/*
 * function_t public functions
 */

function_t * function_create(char *name, voidptr_t fnc) {
  function_t *ret;

  ret = NEW(function_t);
  ret -> name = strdup(name);
  if (fnc) {
    ret -> fnc = fnc;
  } else {
    function_resolve(ret);
  }
  ret -> refs = 1;
  return ret;
}

function_t * function_copy(function_t *src) {
  src -> refs++;
  return src;
}

void function_free(function_t *fnc) {
  if (fnc) {
    fnc -> refs--;
    if (fnc -> refs <= 0) {
      free(fnc -> name);
      free(fnc -> str);
      free(fnc);
    }
  }
}

int function_cmp(function_t *fnc1, function_t *fnc2) {
  return strcmp(fnc1 -> name, fnc2 -> name);
}

function_t * function_resolve(function_t *fnc) {
  fnc -> fnc = (voidptr_t) resolve_function(fnc -> name);
  return (fnc -> fnc) ? fnc : NULL;
}

char * function_tostring(function_t *fnc) {
  if (!fnc -> str) {
    asprintf(&fnc -> str, "%s()", fnc -> name);
  }
  return fnc -> str;
}

unsigned int function_hash(function_t *fnc) {
  return hashblend(strhash(fnc -> name), hashptr(fnc -> fnc));
}

/*
 * reduce_ctx public functions
 */


reduce_ctx * reduce_ctx_create(void *user, void *data, void_t fnc) {
  reduce_ctx *ctx;

  ctx = NEW(reduce_ctx);
  if (ctx) {
    ctx -> data = data;
    ctx -> fnc = fnc;
    ctx -> user = user;
  }
  return ctx;
}

reduce_ctx * collection_hash_reducer(void *elem, reduce_ctx *ctx) {
  hash_t hash = (hash_t) ctx -> fnc;
  ctx -> longdata += hash(elem);
  return ctx;
}

reduce_ctx * collection_add_all_reducer(void *data, reduce_ctx *ctx) {
  ((reduce_t) ctx -> fnc)(ctx -> obj, data);
  return ctx;
}
visit_t collection_visitor(void *data, visit_t visitor) {
  visitor(data);
  return visitor;
}

