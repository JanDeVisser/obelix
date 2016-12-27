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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "libcore.h"
#include <data.h>
#include <exception.h>
#include <name.h>
#include <str.h>

static code_label_t _builtin_exceptions[] = {
  { .code = ErrorSyntax,                .label = "ErrorSyntax" },
  { .code = ErrorArgCount,              .label = "ErrorArgCount" },
  { .code = ErrorMaxStackDepthExceeded, .label = "ErrorMaxStackDepthExceeded" },
  { .code = ErrorInternalError,         .label = "ErrorInternalError" },
  { .code = ErrorType,                  .label = "ErrorType" },
  { .code = ErrorName,                  .label = "ErrorName" },
  { .code = ErrorNotCallable,           .label = "ErrorNotCallable" },
  { .code = ErrorRange,                 .label = "ErrorRange" },
  { .code = ErrorIOError,               .label = "ErrorIOError" },
  { .code = ErrorSysError,              .label = "ErrorSysError" },
  { .code = ErrorNotIterable,           .label = "ErrorNotIterable" },
  { .code = ErrorExhausted,             .label = "ErrorExhausted" },
  { .code = ErrorNotIterator,           .label = "ErrorNotIterator" },
  { .code = ErrorThrowable,             .label = "ErrorThrowable" },
  { .code = ErrorLeave,                 .label = "ErrorLeave", },
  { .code = ErrorReturn,                .label = "ErrorReturn", },
  { .code = ErrorExit,                  .label = "ErrorExit", },
  { .code = ErrorYield,                 .label = "ErrorYield", },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL },
  { .code = ErrorNoError,               .label = NULL }
};

static int           _exceptions_sz = 30;
static code_label_t *_exceptions = _builtin_exceptions;

extern void          exception_init(void);
extern void          _exception_free(exception_t *);
extern char *        _exception_allocstring(exception_t *);
static int           _exception_intval(data_t *);
static data_t *      _exception_resolve(data_t *, char *);
static data_t *      _exception_call(data_t *, array_t *, dict_t *);
static data_t *      _exception_cast(data_t *, int);
static data_t *      _data_exception_from_exception(exception_t *);

static vtable_t _vtable_exception[] = {
  { .id = FunctionCmp,         .fnc = (void_t) exception_cmp },
  { .id = FunctionHash,        .fnc = (void_t) exception_hash },
  { .id = FunctionFree,        .fnc = (void_t) _exception_free },
  { .id = FunctionAllocString, .fnc = (void_t) _exception_allocstring },
  { .id = FunctionIntValue,    .fnc = (void_t) _exception_intval },
  { .id = FunctionResolve,     .fnc = (void_t) _exception_resolve },
  { .id = FunctionCall,        .fnc = (void_t) _exception_call },
  { .id = FunctionCast,        .fnc = (void_t) _exception_cast },
  { .id = FunctionNone,        .fnc = NULL }
};

/* --  E X C E P T I O N _ T  F U N C T I O N S --------------------------- */

OBLCORE_IMPEXP int _exception_register(char *str) {
  code_label_t *new_exceptions;
  int           newsz;
  int           cursz;
  int           ix;

  for (ix = 0; ix < _exceptions_sz; ix++) {
    if (_exceptions[ix].code == ErrorNoError) {
      _exceptions[ix].code = ix;
      _exceptions[ix].label = str;
      return ix;
    }
  }

  cursz = _exceptions_sz * sizeof(code_label_t);
  newsz = 2 * cursz;
  if (_exceptions == _builtin_exceptions) {
    new_exceptions = (code_label_t *) new(newsz);
    memcpy(new_exceptions, _exceptions, cursz);
  } else {
    new_exceptions = (code_label_t *) resize_block(_exceptions, newsz, cursz);
  }
  _exceptions = new_exceptions;
  for (ix = _exceptions_sz + 1; ix < 2 * _exceptions_sz; ix++) {
    _exceptions[ix].code = ErrorNoError;
    _exceptions[ix].label = NULL;
  }
  ix = _exceptions_sz;
  _exceptions_sz *= 2;
  _exceptions[ix].code = ix;
  _exceptions[ix].label = str;
  return ix;
}

exception_t * exception_create(int code, char *msg, ...) {
  exception_t *ret;
  va_list      args;

  if (strchr(msg, '%')) {
    va_start(args, msg);
    ret = exception_vcreate(code, msg, args);
    va_end(args);
  } else {
    ret = data_new(Exception, exception_t);
    ret -> code = code;
    ret -> msg = strdup(msg);
    ret -> handled = 0;
    ret -> throwable = NULL;
    ret -> trace = NULL;
  }
  return ret;
}

exception_t * exception_vcreate(int code, char *msg, va_list args) {
  exception_t *ret;

  ret = data_new(Exception, exception_t);
  ret -> code = code;
  vasprintf(&ret -> msg, msg, args);
  ret -> handled = 0;
  ret -> throwable = NULL;
  ret -> trace = NULL;
  return ret;
}

exception_t * exception_from_errno(void) {
  return exception_from_my_errno(errno);
}

exception_t * exception_from_my_errno(int err) {
  // FIXME Map errno to ErrorXXX. It's not always an IOError.
  return exception_create(ErrorIOError, strerror(err));
}

unsigned int exception_hash(exception_t *exception) {
  return hashptr(exception);
}

int exception_cmp(exception_t *e1, exception_t *e2) {
  return (e1 -> code != e2 -> code)
    ? e1 -> code - e2 -> code
    : strcmp(e1 -> msg, e2 -> msg);
}

void exception_report(exception_t *e) {
  error(exception_tostring(e));
}

/*
 * --------------------------------------------------------------------------
 * Error datatype functions
 * --------------------------------------------------------------------------
 */

void exception_init(void) {
  typedescr_create_and_register(Exception, "exception",
				_vtable_exception, NULL);
}

char * _exception_allocstring(exception_t *exception) {
  char *buf;

  asprintf(&buf, "Error %s (%d): %s",
          label_for_code(_exceptions, exception -> code),
          exception -> code,
          exception -> msg);
  return buf;
}

void _exception_free(exception_t *exception) {
  if (exception) {
    free(exception -> msg);
    data_free(exception -> throwable);
    data_free(exception -> trace);
  }
}

int _exception_intval(data_t *data) {
  exception_t *ex = data_as_exception(data);
  data_t      *throwable = (ex) ? ex -> throwable : NULL;
  int          ret = 0;

  if (throwable) {
    ret = data_intval(throwable);
  } else if (ex) {
    ret = - ex -> code;
  }
  return ret;
}

data_t * _exception_cast(data_t *src, int totype) {
  exception_t *ex = data_as_exception(src);
  data_t      *ret = NULL;

  if (totype == Bool) {
    ret = int_as_bool(ex != NULL);
  }
  return ret;
}

data_t * _exception_resolve(data_t *exception, char *name) {
  exception_t *e = data_as_exception(exception);
  name_t      *n = NULL;
  data_t      *ret;

  if (!strcmp(name, "message")) {
    return (data_t *) str_copy_chars(e -> msg);
  } else if (!strcmp(name, "stacktrace")) {
    return data_copy(e -> trace);
  } else if (!strcmp(name, "code")) {
    return (data_t *) int_create(e -> code);
  } else if (!strcmp(name, "codename")) {
    return (data_t *) str_wrap(_exceptions[e -> code].label);
  } else if (!strcmp(name, "throwable")) {
    return data_copy(e -> throwable);
  } else if (e -> throwable) {
    ret = data_resolve(e -> throwable, n = name_create(1, name));
    name_free(n);
    return ret;
  } else {
    return NULL;
  }
}

data_t * _exception_call(data_t *exception, array_t *args, dict_t *kwargs) {
  exception_t *e = data_as_exception(exception);

  if ((e -> throwable) && data_is_callable(e -> throwable)) {
    return data_call(e -> throwable, args, kwargs);
  } else {
    return NULL;
  }
}

/* -- E X C E P T I O N  F A C T O R Y  F U N C T I O N S ----------------- */

_unused_ data_t * _data_exception_from_exception(exception_t *exception) {
  return data_copy((data_t *) exception);
}

data_t * data_exception(int code, char *msg, ...) {
  exception_t *exception;
  va_list      args;

  va_start(args, msg);
  exception = exception_vcreate(code, msg, args);
  va_end(args);
  return (data_t *) exception;
}

data_t * data_exception_from_my_errno(int err) {
  return (data_t *) exception_from_my_errno(err);
}

data_t * data_exception_from_errno(void) {
  return (data_t *) exception_from_errno();
}

data_t * data_throwable(data_t *throwable) {
  exception_t *exception;

  exception = exception_create(ErrorThrowable, data_tostring(throwable));
  exception -> throwable = data_copy(throwable);
  return (data_t *) exception;
}
