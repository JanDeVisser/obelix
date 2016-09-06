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

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <core.h>
#include <dict.h>
#include <logging.h>

typedef struct _logcategory {
  char        *name;
  int          enabled;
  int         *flag;
  log_level_t  level;
  char        *str;
} logcategory_t;

static void            __logging_init(void);

static logcategory_t * _logcategory_create(char *, int *);
static char *          _logcategory_tostring(logcategory_t *);
static void            _logcategory_free(logcategory_t *);
static int             _logcategory_cmp(logcategory_t *, logcategory_t *);
static int             _logcategory_hash(logcategory_t *);
static logcategory_t * _logcategory_set(logcategory_t *, int);

static void            _logging_set(char *, int);
static char *          _log_level_str(log_level_t lvl);

static dict_t *        _categories = NULL;
static log_level_t     _log_level = LogLevelError;

static code_label_t _log_level_labels[] = {
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

static pthread_once_t  _logging_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t _logging_mutex;

#define _logging_init() pthread_once(&_logging_once, __logging_init)

/* ------------------------------------------------------------------------ */

logcategory_t * _logcategory_create_nolock(char *name, int *flag) {
  logcategory_t *ret = NEW(logcategory_t);

  assert(!dict_has_key(_categories, name));
  ret -> name = strdup(name);
  ret -> flag = flag;
  ret -> level = _log_level;
  if (flag) {
    *flag = 0;
  }
  ret -> enabled = 0;
  pthread_mutex_lock(&_logging_mutex);
  dict_put(_categories, strdup(name), ret);
  pthread_mutex_unlock(&_logging_mutex);
  return ret;
}

logcategory_t * _logcategory_create(char *name, int *flag) {
  logcategory_t *ret;

  _logging_init();
  pthread_mutex_lock(&_logging_mutex);
  ret = _logcategory_create_nolock(name, flag);
  pthread_mutex_unlock(&_logging_mutex);
  return ret;
}

logcategory_t * _logcategory_get(char *name) {
  logcategory_t *ret;

  _logging_init();
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
    debug("Enabling %s logging", cat -> name);
  }
  if (cat -> flag) {
    *(cat -> flag) = value;
  }
  cat -> enabled = value;
  return cat;
}

logcategory_t  * _logcategory_logmsg(logcategory_t *cat, log_level_t lvl, char *file, int line, char *msg, ...) {
  va_list args;

  if (lvl >= cat -> level) {
    va_start(args, msg);
    fprintf(stderr, "%-12.12s:%4d:%-5.5s:", file, line, _log_level_str(lvl));
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
  return cat;
}

int * _logging_set_reducer(logcategory_t *cat, int *value) {
  _logcategory_set(cat, *value);
  return value;
}

void _logging_set_nolock(char *category, int value) {
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

void _logging_set(char *category, int value) {
  _logging_init();
  pthread_mutex_lock(&_logging_mutex);
  _logging_set_nolock(category, value);
  pthread_mutex_unlock(&_logging_mutex);
}

char * _log_level_str(log_level_t lvl) {
  assert(_log_level_labels[lvl].code == lvl);
  return _log_level_labels[lvl].label;
}

/* ------------------------------------------------------------------------ */

void __logging_init(void) {
  char                *cats;
  char                *ptr;
  char                *sepptr;
  pthread_mutexattr_t  attr;
  char                *lvl;
  long                 level;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&_logging_mutex, &attr);
  pthread_mutexattr_destroy(&attr);

  _categories = strvoid_dict_create();
  dict_set_data_type(_categories, &_type_logcategory);

  lvl = getenv("OBL_LOGLEVEL");
  if (lvl && *lvl) {
    level = -1;
    if (strtoint(lvl, &level)) {
      level = code_for_label(_log_level_labels, lvl);
    }
    if ((level >= LogLevelDebug) && (level <= LogLevelFatal)) {
      logging_set_level((log_level_t) level);
    }
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
      ptr = sepptr + strlen(sepptr) + 1;
    }
    if (ptr && *ptr) {
      _logging_set_nolock(ptr, 1);
    }
    free(cats);
  }
}

void logging_init(void) {
  _logging_init();
}

void logging_register_category(char *name, int *flag) {
  logcategory_t *cat;

  _logging_init();
  pthread_mutex_lock(&_logging_mutex);
  if (cat = dict_get(_categories, name)) {
    cat -> flag = flag;
    *flag = cat -> enabled;
  } else {
    _logcategory_create_nolock(name, flag);
  }
  pthread_mutex_unlock(&_logging_mutex);
}

void logging_reset(void) {
  int value = 0;

  _logging_init();
  pthread_mutex_lock(&_logging_mutex);
  dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
  pthread_mutex_unlock(&_logging_mutex);
}

void logging_enable(char *category) {
  _logging_set(category, 1);
}

void logging_disable(char *category) {
  _logging_set(category, 0);
}

void _logmsg(log_level_t lvl, const char *file, int line, const char *caller, const char *msg, ...) {
  va_list args;

  if (lvl >= _log_level) {
    va_start(args, msg);
    fprintf(stderr, "%-12.12s:%4d:%-20.20s:%-5.5s:", file, line, caller, _log_level_str(lvl));
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
}

int logging_status(char *category) {
  logcategory_t *cat;

  _logging_init();
  pthread_mutex_lock(&_logging_mutex);
  cat = dict_get(_categories, category);
  if (!cat) {
    cat = _logcategory_create(category, NULL);
    cat -> enabled = FALSE;
  }
  pthread_mutex_unlock(&_logging_mutex);
  return cat -> enabled;
}

int logging_level(void) {
  return _log_level;
}

int logging_set_level(log_level_t log_level) {
  _log_level = log_level;
  return _log_level;
}
