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

#include <stdio.h>

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

static void            _logging_init(void) __attribute__((constructor(101)));

static logcategory_t * _logcategory_create(char *, int *);
static char *          _logcategory_tostring(logcategory_t *);
static void            _logcategory_free(logcategory_t *);
static logcategory_t * _logcategory_set(logcategory_t *, int);

static void            _logging_set(char *, int);
static char *          _log_level_str(log_level_t lvl);

static dict_t *        _categories = NULL;
  
static code_label_t _log_level_labels[] = {
  { .code = LogLevelDebug,   .label = "DEBUG" },
  { .code = LogLevelInfo,    .label = "INFO" },
  { .code = LogLevelWarning, .label = "WARN" },
  { .code = LogLevelError,   .label = "ERROR" },
  { .code = LogLevelFatal,   .label = "FATAL" }
};

/* ------------------------------------------------------------------------ */

void _logging_init(void) {
  if (!_categories) {
    _categories = strvoid_dict_create();
    dict_set_tostring_data(_categories, (tostring_t) _logcategory_tostring);
    dict_set_free_data(_categories, (free_t) _logcategory_free);
  }
}

logcategory_t * _logcategory_create(char *name, int *flag) {
  logcategory_t *cat = NEW(logcategory_t);

  if (!_categories) {
    _logging_init();
  }  
  cat -> name = strdup(name);
  cat -> flag = flag;
  cat -> level = log_level;
  if (flag) {
    *flag = 0;
  }
  cat -> enabled = 0;
  dict_put(_categories, strdup(name), cat);
  return cat;
}

logcategory_t * _logcategory_get(char *name) {
  if (!_categories) {
    _logging_init();
  }
  return (logcategory_t *) dict_get(_categories, name);
}

char * _logcategory_tostring(logcategory_t *cat) {
  asprintf(&cat -> str, "%s:%d", cat -> name, *(cat -> flag));
  return cat -> str;
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

void _logging_set(char *category, int value) {
  logcategory_t *cat;
  
  if (strcmp(category, "all")) {
    cat = dict_get(_categories, category);
    if (cat) {
      _logcategory_set(cat, value);
    } else {
      cat = _logcategory_create(category, NULL);
      cat -> enabled = value;
    }
  } else {
    dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
  }
}

char * _log_level_str(log_level_t lvl) {
  assert(_log_level_labels[lvl].code == lvl);
  return _log_level_labels[lvl].label;
}

/* ------------------------------------------------------------------------ */

void logging_register_category(char *name, int *flag) {
  logcategory_t *cat = dict_get(_categories, name);
  
  if (cat) {
    cat -> flag = flag;
    *flag = cat -> enabled;
  } else {
    cat = _logcategory_create(name, flag);
  }
}

void logging_reset(void) {
  int value = 0;
  dict_reduce_values(_categories, (reduce_t) _logging_set_reducer, &value);
}

void logging_enable(char *category) {
  _logging_set(category, 1);
}

void logging_disable(char *category) {
  _logging_set(category, 0);
}

void _logmsg(log_level_t lvl, const char *file, int line, const char *caller, const char *msg, ...) {
  va_list args;
  
  if (lvl >= log_level) {
    va_start(args, msg);
    fprintf(stderr, "%-12.12s:%4d:%-20.20s:%-5.5s:", file, line, caller, _log_level_str(lvl));
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
}

int logging_status(char *category) {
  logcategory_t *cat;
  int            ret;
  
  cat = dict_get(_categories, category);
  if (!cat) {
    cat = _logcategory_create(category, NULL);
    cat -> enabled = FALSE;
  }
  return cat -> enabled;  
}
