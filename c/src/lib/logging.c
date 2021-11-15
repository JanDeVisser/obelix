/*
 * /obelix/src/parser/logging.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "libcore.h"
#include <dict.h>
#include <timer.h>

typedef struct _logcategory {
  char        *name;
  int          enabled;
  int         *flag;
  log_level_t  level;
  char        *str;
} logcategory_t;

static void            _logging_lock();
static void            _logging_unlock();

static logcategory_t * _logcategory_create(const char *, int *);
static char *          _logcategory_tostring(logcategory_t *);
static void            _logcategory_free(logcategory_t *);
static int             _logcategory_cmp(logcategory_t *, logcategory_t *);
static int             _logcategory_hash(logcategory_t *);
static logcategory_t * _logcategory_set(logcategory_t *, int);

static void            _logging_set(const char *, int);
static char *          _log_level_str(log_level_t lvl);

static char *          _logfile = NULL;
static FILE *          _destination = NULL;
static dict_t *        _categories = NULL;
static log_level_t     _log_level = LogLevelWarning;
       int             core_debug = 0;
extern int             mutex_debug;

static code_label_t _log_level_labels[] = {
  { .code = LogLevelNone,    .label = "     " },
  { .code = LogLevelDebug,   .label = "DEBUG" },
  { .code = LogLevelInfo,    .label = "INFO" },
  { .code = LogLevelWarning, .label = "WARN" },
  { .code = LogLevelError,   .label = "ERROR" },
  { .code = LogLevelFatal,   .label = "FATAL" }
};

static type_t _type_logcategory = {
  .hash     = (hash_t) _logcategory_hash,
  .tostring = (tostring_t) _logcategory_tostring,
  .copy     = NULL,
  .free     = (free_t) _logcategory_free,
  .cmp      = (cmp_t) _logcategory_cmp
};

static int             _initialized = 0;
static pthread_mutex_t _mutex;

/* ------------------------------------------------------------------------ */

void _logging_lock() {
  errno = pthread_mutex_lock(&_mutex);
  if (errno) {
    fprintf(stderr, "Error locking logging mutex: %s\n", strerror(errno));
    abort();
  }
}

void _logging_unlock() {
  errno = pthread_mutex_unlock(&_mutex);
  if (errno) {
    fprintf(stderr, "Error locking logging mutex: %s\n", strerror(errno));
    abort();
  }
}

/* ------------------------------------------------------------------------ */

logcategory_t * _logcategory_create_nolock(const char *name, int *flag) {
  logcategory_t *ret = NEW(logcategory_t);

  // fprintf(stderr, "Creating log category '%s'\n", name);
  assert(!dict_has_key(_categories, name));
  ret -> name = strdup(name);
  ret -> flag = flag;
  ret -> level = _log_level;
  if (flag) {
    *flag = 0;
  }
  ret -> enabled = 0;
  dict_put(_categories, strdup(name), ret);
  // assert(dict_has_key(_categories, name));
  return ret;
}

logcategory_t * _logcategory_create(const char *name, int *flag) {
  logcategory_t *ret;

  logging_init();
  _logging_lock();
  ret = _logcategory_create_nolock(name, flag);
  _logging_unlock();
  return ret;
}

logcategory_t * _logcategory_get(const char *name) {
  logcategory_t *ret;

  logging_init();
  ret = (logcategory_t *) dict_get(_categories, name);
  return ret;
}

char * _logcategory_tostring(logcategory_t *cat) {
  asprintf(&cat -> str, "%s:%d", cat -> name, *(cat -> flag));
  return cat -> str;
}

int _logcategory_hash(logcategory_t *cat) {
  return strhash(cat -> name);
}

int _logcategory_cmp(logcategory_t *cat1, logcategory_t *cat2) {
  return strcmp(cat1 -> name, cat2 -> name);
}

void _logcategory_free(logcategory_t *cat) {
  if (cat) {
    free(cat -> name);
    free(cat -> str);
    free(cat);
  }
}

logcategory_t * _logcategory_set(logcategory_t *cat, int value) {
  if (value) {
    debug(core, "Enabling %s logging", cat -> name);
  }
  if (cat -> flag) {
    *(cat -> flag) = value;
  }
  cat -> enabled = value;
  return cat;
}

int * _logging_set_reducer(logcategory_t *cat, int *value) {
  _logcategory_set(cat, *value);
  return value;
}

void _logging_set_nolock(const char *category, int value) {
  logcategory_t *cat;

  if (strcmp(category, "all")) {
    cat = dict_get(_categories, category);
    if (cat) {
      _logcategory_set(cat, value);
    } else {
      cat = _logcategory_create_nolock(category, NULL);
      cat -> enabled = value;
    }
  } else {
    dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
  }
}

void _logging_set(const char *category, int value) {
  logging_init();
  _logging_lock();
  _logging_set_nolock(category, value);
  _logging_unlock();
}

char * _log_level_str(log_level_t lvl) {
  assert(_log_level_labels[lvl - LogLevelNone].code == lvl);
  return _log_level_labels[lvl - LogLevelNone].label;
}

/* ------------------------------------------------------------------------ */

void _logging_open_logfile(void) {
  char *logfile = getenv("OBL_LOGFILE");

  if (logfile) {
    _logfile = strdup(logfile);
  }
}

void logging_init(void) {
  pthread_mutexattr_t  attr;
  char                *cats;
  char                *ptr;
  char                *sepptr;
  char                *lvl;

  if (_initialized) {
    return;
  }

  // fprintf(stderr, "Initializing logging...\n");
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  if ((errno = pthread_mutex_init(&_mutex, &attr))) {
    fprintf(stderr, "Error initializing logging mutex: %s\n", strerror(errno));
    abort();
  }
  pthread_mutexattr_destroy(&attr);

  _categories = strvoid_dict_create();
  dict_set_data_type(_categories, &_type_logcategory);

  _logging_open_logfile();
  lvl = getenv("OBL_LOGLEVEL");
  if (lvl && *lvl) {
    logging_set_level(lvl);
  }

  cats = getenv("OBL_DEBUG");
  if (!cats) {
    cats = getenv("DEBUG");
  }
  if (cats && *cats) {
    cats = strdup(cats);
    ptr = cats;
    for (sepptr = strpbrk(ptr, ";,:"); sepptr; sepptr = strpbrk(ptr, ";,:")) {
      *sepptr = 0;
      _logging_set_nolock(ptr, 1);
      ptr = sepptr + 1;
    }
    if (ptr && *ptr) {
      _logging_set_nolock(ptr, 1);
    }
    free(cats);
  }
  _logcategory_create_nolock("core", &core_debug);
  _logcategory_create_nolock("mutex", &mutex_debug);
  _initialized = 1;
}

extern void logging_register_category(const char *name, int *flag) {
  logcategory_t *cat;

  logging_init();
  _logging_lock();
  if ((cat = dict_get(_categories, name))) {
    cat -> flag = flag;
    // fprintf(stderr, "logging_register_category('%s') - Turning flag %s\n",
    //   name, (cat -> enabled) ? "ON" : "OFF");
    *flag = cat -> enabled;
  } else {
    cat = _logcategory_create_nolock(name, flag);
    assert(*flag == cat -> enabled);
    // fprintf(stderr, "logging_register_category('%s') - Creating category, setting flag %s\n",
    //   name, (cat -> enabled) ? "ON" : "OFF");
  }
  _logging_unlock();
}

extern void logging_reset(void) {
  int value = 0;

  logging_init();
  _logging_lock();
  dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
  _logging_unlock();
}

extern void logging_enable(const char *category) {
  _logging_set(category, 1);
}

extern void logging_disable(const char *category) {
  _logging_set(category, 0);
}

extern void _vlogmsg_no_nl(log_level_t lvl, const char *file, int line,
                                   const char *caller, const char *msg, va_list args) {
  char *f = NULL;

  if (!_destination) {
    if (_logfile) {
      _destination = fopen(_logfile, "w");
      if (!_destination) {
        fprintf(stderr, "Could not open logfile '%s': %s\n", _logfile, strerror(errno));
        fprintf(stderr, "Falling back to stderr\n");
      }
    }
    if (!_destination) {
      _destination = stderr;
    }
  }
  if ((lvl <= 0) || (lvl >= _log_level)) {
    if (*file == '/') {
      f = strrchr(file, '/');
      if (f) {
        f = f + 1;
      }
    }
    fprintf(_destination, "%-12.12s:%4d:%-20.20s:%-5.5s:", (f) ? f : file, line, caller, _log_level_str(lvl));
    vfprintf(_destination, msg, args);
  }
}

extern void _vlogmsg(log_level_t lvl, const char *file, int line,
                             const char *caller, const char *msg, va_list args) {
  if ((lvl <= 0) || (lvl >= _log_level)) {
    _vlogmsg_no_nl(lvl, file, line, caller, msg, args);
    fprintf(_destination, "\n");
  }
}


extern void _logmsg(log_level_t lvl, const char *file, int line, const char *caller, const char *msg, ...) {
  va_list args;

  if ((lvl <= 0) || (lvl >= _log_level)) {
    va_start(args, msg);
    _vlogmsg(lvl, file, line, caller, msg, args);
    va_end(args);
  }
}

extern int logging_status(const char *category) {
  logcategory_t *cat;

  logging_init();
  _logging_lock();
  cat = dict_get(_categories, category);
  if (!cat) {
    cat = _logcategory_create(category, NULL);
    cat -> enabled = FALSE;
  }
  _logging_unlock();
  return cat -> enabled;
}

extern int logging_level(void) {
  return _log_level;
}

extern int logging_set_level(const char *log_level) {
  long level = -1;

  if (log_level) {
    if (strtoint(log_level, &level)) {
      level = code_for_label(_log_level_labels, log_level);
    }
    if ((level >= LogLevelDebug) && (level <= LogLevelFatal)) {
      _log_level = level;
    }
  }
  return _log_level;
}

extern int logging_set_file(const char *logfile) {
  logging_init();
  _logging_lock();
  if (_destination && (_destination != stderr)) {
    fclose(_destination);
  }
  _destination = NULL;
  free(_logfile);
  _logfile = (_logfile) ? strdup(logfile) : NULL;
  _logging_unlock();
  return 0;
}

log_timestamp_t * _log_timestamp_start(void) {
  return (log_timestamp_t *) timer_start();
}

void _log_timestamp_end(log_timestamp_t *ts, const char *file, int line, const char *caller, const char *msg, ...) {
  va_list      args;
  timestamp_t *t;

  t = timer_end((timestamp_t *) ts);
  va_start(args, msg);
  _vlogmsg_no_nl(LogLevelDebug, file, line, caller, msg, args);
  fprintf(_destination, "%ld.%06ld sec\n", t -> seconds, t -> microseconds);
  va_end(args);
  free(t);
}
