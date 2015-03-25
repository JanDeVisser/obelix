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
#include <data.h>
#include <exception.h>

static code_label_t  builtin_errors[];
static int           num_errors;
static code_label_t *errors;

static code_label_t builtin_errors[] = {
  { .code = ErrorSyntax,      .label = "ErrorSyntax" },
  { .code = ErrorArgCount,    .label = "ErrorArgCount" },
  { .code = ErrorType,        .label = "ErrorType" },
  { .code = ErrorName,        .label = "ErrorName" },
  { .code = ErrorNotCallable, .label = "ErrorNotCallable" },
  { .code = ErrorRange,       .label = "ErrorRange" },
  { .code = ErrorIOError,     .label = "ErrorIOError" },
  { .code = ErrorSysError,    .label = "ErrorSysError" },
  { .code = ErrorNotIterable, .label = "ErrorNotIterable" },
  { .code = ErrorExhausted,   .label = "ErrorExhausted" },
  { .code = ErrorNotIterator, .label = "ErrorNotIterator" },
};

static int           num_errors = sizeof(builtin_errors) / sizeof(code_label_t);
static code_label_t *errors = builtin_errors;

static void          _error_init(void) __attribute__((constructor));
static data_t *      _error_new(data_t *, va_list);
static unsigned int  _error_hash(data_t *);
static data_t *      _error_copy(data_t *, data_t *);
static int           _error_cmp(data_t *, data_t *);
static char *        _error_tostring(data_t *);

static data_t *      _error_create(data_t *, char *, array_t *, dict_t *);


static vtable_t _vtable_error[] = {
  { .id = FunctionNew,      .fnc = (void_t) _error_new },
  { .id = FunctionCopy,     .fnc = (void_t) _error_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _error_cmp },
  { .id = FunctionFree,     .fnc = (void_t) error_free },
  { .id = FunctionToString, .fnc = (void_t) _error_tostring },
  { .id = FunctionNone,     .fnc = NULL }
};


static typedescr_t _typedescr_error = {
  .type      = Error,
  .type_name = "error",
  .vtable    = _vtable_error
};

/*
 * error_t functions
 */

int error_register(char *str) {
  code_label_t *new_errors;
  code_label_t  descr;
  int           newsz;
  int           cursz;

  descr.code = num_errors;
  descr.label = str;
  newsz = (num_errors + 1) * sizeof(code_label_t);
  cursz = num_errors * sizeof(code_label_t);
  if (errors == builtin_errors) {
    new_errors = (code_label_t *) new(newsz);
    memcpy(new_errors, errors, cursz);
  } else {
    new_errors = (code_label_t *) resize_block(errors, newsz, cursz);
  }
  errors = new_errors;
  num_errors++;
  memcpy(errors + descr.code, &descr, sizeof(code_label_t));
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

  ret = NEW(error_t);
  ret -> code = code;
  msg = va_arg(args, char *);
  vasprintf(&ret -> msg, msg, args);
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
  if (!error -> str) {
    asprintf(&error -> str, "Error %s (%d): %s",
            errors[error -> code].label,
            error -> code,
            error -> msg);
  }
  return error -> str;
}

void error_report(error_t *e) {
  error(error_tostring(e));
}

/*
 * --------------------------------------------------------------------------
 * Error datatype functions
 * --------------------------------------------------------------------------
 */

void _error_init(void) {
  typedescr_register(&_typedescr_error);
}

data_t * _error_new(data_t *target, va_list args) {
  int      code;
  error_t *e;

  code = va_arg(args, int);
  e = error_vcreate(code, args);
  target -> ptrval = e;
  return target;
}

unsigned int _error_hash(data_t *data) {
  return error_hash((error_t *) data -> ptrval);
}

data_t * _error_copy(data_t *target, data_t *src) {
  error_t *e;

  e = error_copy((error_t *) src -> ptrval);
  target -> ptrval = e;
  return target;
}

int _error_cmp(data_t *d1, data_t *d2) {
  return error_cmp((error_t *) d1 -> ptrval, (error_t *) d2 -> ptrval);
}

char * _error_tostring(data_t *data) {
  return error_tostring((error_t *) data -> ptrval);
}

data_t * data_error(int code, char * msg, ...) {
  char    buf[256];
  char   *ptr;
  int     size;
  va_list args;
  va_list args_copy;
  data_t *ret;
  
  va_start(args, msg);
  va_copy(args_copy, args);
  size = vsnprintf(buf, 256, msg, args);
  if (size > 256) {
    ptr = (char *) new(size);
    vsprintf(ptr, msg, args_copy);
  } else {
    ptr = buf;
  }
  va_end(args);
  va_end(args_copy);
  ret = data_create(Error, code, ptr);
  if (size > 256) {
    free(ptr);
  }
  return ret;
}

