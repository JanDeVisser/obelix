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
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <core.h>

static void    __init(void);
static void    _outofmemory(int, siginfo_t *, void *);

static int _initialized = 0;

log_level_t log_level = LogLevelInfo;

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
  _initialized = 1;
}


void * new(int sz) {
  void *ret;

  if (!_initialized) {
    __init();
  }

  ret = malloc(sz);
  if (sz && !ret) {
    kill(0, SIGUSR1);
  } else {
    memset(ret, 0, sz);
  }
  return ret;
}

void * new_ptrarray(int sz) {
  if (!_initialized) {
    __init();
  }
  void * ret = calloc(sz, sizeof(void *));
  if (sz && !ret) {
    kill(0, SIGUSR1);
  }
  return ret;
}

void * resize_block(void *block, int newsz, int oldsz) {
  void *ret;

  if (!_initialized) {
    __init();
  }
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

char * _log_level_str(log_level_t lvl) {
  switch (lvl) {
    case LogLevelDebug:
      return "DEBUG";
    case LogLevelInfo:
      return "INFO ";
    case LogLevelWarning:
      return "WARN ";
    case LogLevelError:
      return "ERROR";
    case LogLevelFatal:
      return "FATAL";
  }
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

int atob(char *str) {
  int ret;

  if (!strcasecmp(str, "true")) {
    return 1;
  }
  if (!strcasecmp(str, "false")) {
    return 0;
  }
  return atoi(str);
}

char * btoa(long b) {
  return (b) ? "true" : "false";
}

char * chars(void *str) {
  return (char *) str;
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

function_ptr_t no_func_ptr = { void_fnc: (void_t) NULL };

/*
 * function_t public functions
 */

function_t * function_create(char *name, voidptr_t fnc) {
  function_t *ret;

  ret = NEW(function_t);
  ret -> name = strdup(name);
  ret -> fnc = fnc;
  return ret;
}

function_t * function_copy(function_t *src) {
  return function_create(src -> name, src -> fnc);
}

void function_free(function_t *fnc) {
  if (fnc) {
    free(fnc -> name);
    free(fnc);
  }
}

char * function_tostring(function_t *fnc) {
  /* FIXME: Not Threadsafe */
  static char buf[100];
  snprintf(buf, 100, "%s()", fnc -> name);
  return buf;
}


/*
 * reduce_ctx public functions
 */


reduce_ctx * reduce_ctx_create(void *user, void *data, function_ptr_t fnc) {
  reduce_ctx *ctx;

  ctx = NEW(reduce_ctx);
  if (ctx) {
    ctx -> data = data;
    ctx -> fnc = fnc;
    ctx -> user = user;
  }
  return ctx;
}

