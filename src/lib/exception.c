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
#include <stdio.h>
#include <string.h>

#include <core.h>
#include <data.h>
#include <exception.h>
#include <math.h>

static code_label_t  builtin_exceptions[];
static int           num_exceptions;
static code_label_t *exceptions;

static code_label_t builtin_exceptions[] = {
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
  { .code = ErrorExit,                  .label = "ErrorExit", },
};

static int           num_exceptions = sizeof(builtin_exceptions) / sizeof(code_label_t);
static code_label_t *exceptions = builtin_exceptions;

static void          _exception_init(void) __attribute__((constructor(105)));
static data_t *      _exception_new(data_t *, va_list);
static unsigned int  _exception_hash(data_t *);
static data_t *      _exception_copy(data_t *, data_t *);
static int           _exception_cmp(data_t *, data_t *);
static char *        _exception_tostring(data_t *);
static int           _exception_intval(data_t *);

static data_t *      _exception_create(data_t *, char *, array_t *, dict_t *);
static data_t *      _exception_resolve(data_t *, char *);
static data_t *      _exception_call(data_t *, array_t *, dict_t *);
static data_t *      _exception_cast(data_t *, int);
static data_t *      _data_exception_from_exception(exception_t *);


static vtable_t _vtable_exception[] = {
  { .id = FunctionNew,      .fnc = (void_t) _exception_new },
  { .id = FunctionCopy,     .fnc = (void_t) _exception_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _exception_cmp },
  { .id = FunctionFree,     .fnc = (void_t) exception_free },
  { .id = FunctionToString, .fnc = (void_t) _exception_tostring },
  { .id = FunctionIntValue, .fnc = (void_t) _exception_intval },
  { .id = FunctionResolve,  .fnc = (void_t) _exception_resolve },
  { .id = FunctionCall,     .fnc = (void_t) _exception_call },
  { .id = FunctionCast,     .fnc = (void_t) _exception_cast },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_exception = {
  .type      = Exception,
  .type_name = "exception",
  .vtable    = _vtable_exception
};

/*
 * exception_t functions
 */

int exception_register(char *str) {
  code_label_t *new_exceptions;
  code_label_t  descr;
  int           newsz;
  int           cursz;

  descr.code = num_exceptions;
  descr.label = str;
  newsz = (num_exceptions + 1) * sizeof(code_label_t);
  cursz = num_exceptions * sizeof(code_label_t);
  if (exceptions == builtin_exceptions) {
    new_exceptions = (code_label_t *) new(newsz);
    memcpy(new_exceptions, exceptions, cursz);
  } else {
    new_exceptions = (code_label_t *) resize_block(exceptions, newsz, cursz);
  }
  exceptions = new_exceptions;
  num_exceptions++;
  memcpy(exceptions + descr.code, &descr, sizeof(code_label_t));
  return descr.code;
}

exception_t * exception_create(int code, char *msg, ...) {
  exception_t *ret;
  va_list      args;

  va_start(args, msg);
  ret = exception_vcreate(code, msg, args);
  va_end(args);
  return ret;
}

exception_t * exception_vcreate(int code, char *msg, va_list args) {
  int          size;
  exception_t *ret;

  ret = NEW(exception_t);
  ret -> code = code;
  vasprintf(&ret -> msg, msg, args);
  ret -> str = NULL;
  ret -> handled = 0;
  ret -> throwable = NULL;
  ret -> trace = NULL;
  ret -> refs = 1;
  return ret;
}

exception_t * exception_from_errno(void) {
  return exception_from_my_errno(errno);
}

exception_t * exception_from_my_errno(int err) {
  // FIXME Map errno to ErrorXXX. It's not always an IOError.
  exception_create(ErrorIOError, strerror(err));
}

exception_t * exception_copy(exception_t *src) {
  if (src) {
    src -> refs++;
  }
  return src;
}

void exception_free(exception_t *exception) {
  if (exception && (--exception -> refs <= 0)) {
    free(exception -> msg);
    free(exception -> str);
    data_free(exception -> throwable);
    free(exception);
  }
}

unsigned int exception_hash(exception_t *exception) {
  return hashptr(exception);
}

int exception_cmp(exception_t *e1, exception_t *e2) {
  return (e1 -> code != e2 -> code)
    ? e1 -> code - e2 -> code
    : strcmp(e1 -> msg, e2 -> msg);
}

char * exception_tostring(exception_t *exception) {
  if (!exception -> str) {
    asprintf(&exception -> str, "Error %s (%d): %s",
            label_for_code(exceptions, exception -> code),
            exception -> code,
            exception -> msg);
  }
  return exception -> str;
}

void exception_report(exception_t *e) {
  error(exception_tostring(e));
}

/*
 * --------------------------------------------------------------------------
 * Error datatype functions
 * --------------------------------------------------------------------------
 */

void _exception_init(void) {
  typedescr_register(&_typedescr_exception);
}

data_t * _exception_new(data_t *target, va_list args) {
  exception_t *e = exception_vcreate(va_arg(args, int), va_arg(args, char *), args);
  
  target -> ptrval = e;
  return target;
}

unsigned int _exception_hash(data_t *data) {
  return exception_hash((exception_t *) data -> ptrval);
}

data_t * _exception_copy(data_t *target, data_t *src) {
  exception_t *e;

  e = exception_copy((exception_t *) src -> ptrval);
  target -> ptrval = e;
  return target;
}

int _exception_intval(data_t *data) {
  exception_t *ex = data_exceptionval(data);
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
  exception_t *ex = data_exceptionval(src);
  data_t      *ret = NULL;

  if (totype == Bool) {
    ret = data_create(Bool, ex != NULL);
  }
  return ret;
}

int _exception_cmp(data_t *d1, data_t *d2) {
  return exception_cmp((exception_t *) d1 -> ptrval, (exception_t *) d2 -> ptrval);
}

char * _exception_tostring(data_t *data) {
  return exception_tostring((exception_t *) data -> ptrval);
}

data_t * _exception_resolve(data_t *exception, char *name) {
  exception_t *e = data_exceptionval(exception);
  name_t      *n = NULL;
  data_t      *ret;
  
  if (!strcmp(name, "message")) {
    return data_create(String, e -> msg);
  } else if (!strcmp(name, "stacktrace")) {
    return data_copy(e -> trace);
  } else if (!strcmp(name, "code")) {
    return data_create(Int, e -> code);
  } else if (!strcmp(name, "codename")) {
    return data_create(String, exceptions[e -> code].label);
  } else if (!strcmp(name, "")) {
  } else if (e -> throwable) {
    ret = data_resolve(e -> throwable, n = name_create(1, name));
    name_free(n);
    return ret;
  } else {
    return NULL;
  }
}

data_t * _exception_call(data_t *exception, array_t *args, dict_t *kwargs) {
  exception_t *e = data_exceptionval(exception);

  if ((e -> throwable) && data_is_callable(e -> throwable)) {
    return data_call(e -> throwable, args, kwargs);
  } else {
    return NULL;
  } 
}

/* -- E X C E P T I O N  F A C T O R Y  F U N C T I O N S ----------------- */

data_t * _data_exception_from_exception(exception_t *exception) {
  data_t *ret = data_create_noinit(Exception);
  ret -> ptrval = exception_copy(exception);
}

data_t * data_exception(int code, char * msg, ...) {
  exception_t *exception;
  va_list      args;
  
  va_start(args, msg);
  exception = exception_vcreate(code, msg, args);
  va_end(args);
  return _data_exception_from_exception(exception);
}

data_t * data_exception_from_my_errno(int err) {
  exception_t *exception = exception_from_my_errno(err);
  return _data_exception_from_exception(exception);
}

data_t * data_exception_from_errno(void) {
  exception_t *exception = exception_from_errno();
  return _data_exception_from_exception(exception);
}

data_t * data_throwable(data_t *throwable) {
  data_t *exception;
  
  exception = data_exception(ErrorThrowable, data_tostring(throwable));
  data_exceptionval(exception) -> throwable = data_copy(throwable);
  return exception;
}
