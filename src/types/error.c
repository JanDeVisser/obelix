/*
 * /obelix/src/error.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <string.h>

#include <core.h>
#include <error.h>

typedef struct _errordescr {
  int   code;
  char *str;
} errordescr_t;

static errordescr_t  builtin_errors[];
static int           num_errors;
static errordescr_t *errors;

static errordescr_t builtin_errors[] = {
    { code: ErrorArgCount,    str: "ErrorArgCount" },
    { code: ErrorType,        str: "ErrorType" },
    { code: ErrorName,        str: "ErrorName" },
    { code: ErrorNotCallable, str: "ErrorNotCallable" }
};

static int           num_errors = sizeof(builtin_errors) / sizeof(errordescr_t);
static errordescr_t *errors = builtin_errors;

/*
 * error_t functions
 */

int error_register(char *str) {
  errordescr_t *new_errors;
  errordescr_t  descr;
  int           newsz;
  int           cursz;

  descr.code = num_errors;
  descr.str = str;
  newsz = (num_errors + 1) * sizeof(errordescr_t);
  cursz = num_errors * sizeof(errordescr_t);
  if (errors == builtin_errors) {
    new_errors = (errordescr_t *) new(newsz);
    memcpy(new_errors, errors, cursz);
  } else {
    new_errors = (errordescr_t *) resize_block(errors, newsz, cursz);
  }
  errors = new_errors;
  num_errors++;
  memcpy(errors + descr.code, &descr, sizeof(errordescr_t));
  return descr.code;
}

error_t * error_create(int code, ...) {
  error_t *ret;
  va_list  args;

  va_start(args, code);
  ret = error_vcreate(code, args);
  va_end(args);
  return ret;
}

error_t * error_vcreate(int code, va_list args) {
  int      size;
  char    *msg;
  error_t *ret;
  va_list  args_copy;

  ret = NEW(error_t);
  ret -> code = code;
  msg = va_arg(args, char *);
  va_copy(args_copy, args);
  size = vsnprintf(NULL, 0, msg, args);
  va_end(args_copy);
  ret -> msg = (char *) new(size + 1);
  vsprintf(ret -> msg, msg, args);
  return ret;
}

error_t * error_copy(error_t *src) {
  error_t *ret;

  ret = NEW(error_t);
  ret -> code = src -> code;
  ret -> msg = strdup(src -> msg);
  return ret;
}

void error_free(error_t *error) {
  if (error) {
    free(error -> msg);
    free(error -> str);
    free(error);
  }
}

unsigned int error_hash(error_t *error) {
  return hashptr(error);
}

int error_cmp(error_t *e1, error_t *e2) {
  return (e1 -> code != e2 -> code)
    ? e1 -> code - e2 -> code
    : strcmp(e1 -> msg, e2 -> msg);
}

char * error_tostring(error_t *error) {
  int sz;

  if (!error -> str) {
    error -> str = (char *) new(snprintf(NULL, 0, "Error %s (%d): %s",
                                         errors[error -> code].str,
                                         error -> code,
                                         error -> msg));
    sprintf(error -> str, "Error %s (%d): %s",
            errors[error -> code].str,
            error -> code,
            error -> msg);
  }
  return error -> str;
}

void error_report(error_t *e) {
  error(error_tostring(e));
}



