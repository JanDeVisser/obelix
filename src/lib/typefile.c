/*
 * /obelix/src/stdlib/fileobj.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

#include <data.h>
#include <exception.h>
#include <file.h>
#include <list.h>
#include <re.h>
#include <typedescr.h>
#include <wrapper.h>

static void          _file_init(void) __attribute__((constructor));
static data_t *      _file_new(data_t *, va_list);
static data_t *      _file_copy(data_t *, data_t *);
static int           _file_cmp(data_t *, data_t *);
static char *        _file_tostring(data_t *);
static unsigned int  _file_hash(data_t *);
static data_t *      _file_enter(data_t *);
static data_t *      _file_leave(data_t *, data_t *);
static data_t *      _file_query(data_t *, data_t *);
static data_t *      _file_iter(data_t *);
static data_t *      _file_resolve(data_t *, char *);
static data_t *      _file_read(data_t *, char *, int);
static data_t *      _file_write(data_t *, char *, int);

static data_t *      _file_open(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_adopt(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_readline(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_print(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_close(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_redirect(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_seek(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_file[] = {
  { .id = FunctionNew,      .fnc = (void_t) _file_new },
  { .id = FunctionCopy,     .fnc = (void_t) _file_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _file_cmp },
  { .id = FunctionFree,     .fnc = (void_t) file_free },
  { .id = FunctionToString, .fnc = (void_t) _file_tostring },
  { .id = FunctionHash,     .fnc = (void_t) _file_hash },
  { .id = FunctionEnter,    .fnc = (void_t) _file_enter },
  { .id = FunctionLeave,    .fnc = (void_t) _file_leave },
  { .id = FunctionIter,     .fnc = (void_t) _file_iter },
  { .id = FunctionQuery,    .fnc = (void_t) _file_query },
  { .id = FunctionResolve,  .fnc = (void_t) _file_resolve },
  { .id = FunctionRead,     .fnc = (void_t) _file_read },
  { .id = FunctionWrite,    .fnc = (void_t) _file_write },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_file =   {
  .type      = -1,
  .type_name = "file",
  .vtable    = _vtable_file,
};

static methoddescr_t _methoddescr_file[] = {
  { .type = Any,    .name = "open",     .method = _file_open,     .argtypes = { String, Int, Any },       .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "adopt",    .method = _file_adopt,    .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "readline", .method = _file_readline, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "print",    .method = _file_print,    .argtypes = { String, Any,    NoType }, .minargs = 1, .varargs = 1 },
  { .type = -1,     .name = "close",    .method = _file_close,    .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "redirect", .method = _file_redirect, .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "seek",     .method = _file_seek,     .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,           .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

typedef struct _fileiter {
  data_d   _d;
  file_t  *file;
  re_t    *regex;
  list_t  *next;
} fileiter_t;

static fileiter_t * _fileiter_create(file_t *, data_t *);
static void         _fileiter_free(fileiter_t *);
static fileiter_t * _fileiter_copy(fileiter_t *);
static int          _fileiter_cmp(fileiter_t *, fileiter_t *);
static char *       _fileiter_tostring(fileiter_t *);
static data_t *     _fileiter_iter(fileiter_t *);
static data_t *     _fileiter_has_next(fileiter_t *);
static data_t *     _fileiter_next(fileiter_t *);

static vtable_t _vtable_fileiter[] = {
  { .id = FunctionFactory,  .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,      .fnc = (void_t) _fileiter_cmp },
  { .id = FunctionFree,     .fnc = (void_t) _fileiter_free },
  { .id = FunctionToString, .fnc = (void_t) _fileiter_tostring },
  { .id = FunctionHasNext,  .fnc = (void_t) _fileiter_has_next },
  { .id = FunctionNext,     .fnc = (void_t) _fileiter_next },
  { .id = FunctionNone,     .fnc = NULL }
};

#define data_is_fileiter(d)  ((d) && (data_type((d)) == FileIter))
#define data_fileiterval(d)  ((fileiter_t *) (data_is_fileiter((d)) ? ((d) -> ptrval) : NULL))

int File = -1;
int FileIter = -1;

/* ------------------------------------------------------------------------ */

void _file_init(void) {
  int ix;

  File = typedescr_create_and_register(File, "file", _vtable_file, _methoddescr_file);
  if (file_debug) {
    debug("File type initialized");
  }
 typedescr_register_methods(_methoddescr_file);
  FileIter = typedescr_create_and_register(FileIter, "fileiterator", _vtable_fileiter);
}

/* -- F I L E  I T E R A T O R -------------------------------------------- */

fileiter_t * _fileiter_readnext(fileiter_t *iter) {
  data_t  *matches;
  array_t *matchvals;
  int      ix;

  while (list_empty(iter -> next)) {
    if (file_readline(iter -> file)) {
      if (!iter -> regex) {
        list_push(iter -> next, data_create(String, iter -> file -> line));
      } else {
        matches = regexp_match(iter -> regex, iter -> file -> line);
        if (data_type(matches) == String) {
          list_push(iter -> next, data_copy(matches));
        } else if (data_type(matches) == List) {
          matchvals = data_arrayval(matches);
          for (ix = 0; ix < array_size(matchvals); ix++) {
            list_push(iter -> next, data_copy(data_array_get(matchvals, ix)));
          }
        }
        data_free(matches);
      }
    } else if (!file_errno(iter -> file)) {
      list_push(iter -> next,
                data_exception(ErrorExhausted, "Iterator exhausted"));
    } else {
      list_push(iter -> next,
                data_exception_from_my_errno(file_errno(iter -> file)));
    }
  }
  return iter;
}

fileiter_t * _fileiter_create(file_t *file, data_t *regex) {
  fileiter_t *ret = data_new(FileIter, fileiter_t);
  data_t     *regex;
  re_t       *re = NULL;
  int         retval;

  ret -> file = file;
  if (data_is_regexp(regex)) {
    ret -> regex = data_as_regexp(regex);
  } else if (regex) {
    ret -> regex = regexp_create(data_tostring(regex), NULL);
  } else {
    ret -> regex = NULL;
  }
  // FIXME: Error handling for bad regexp.
  retval = file_seek(ret -> file, 0);
  ret -> next = data_list_create();
  if (retval >= 0) {
    _fileiter_readnext(ret);
  } else {
    list_push(ret -> next,
              data_exception_from_my_errno(file_errno(ret -> file)));
  }
  ret -> refs = 1;
  ret -> str = NULL;
  return ret;
}

void _fileiter_free(fileiter_t *fileiter) {
  if (fileiter && (--fileiter -> refs <= 0)) {
    list_free(fileiter -> next);
    free(fileiter);
  }
}

int _fileiter_cmp(fileiter_t *fileiter1, fileiter_t *fileiter2) {
  return fileiter1 -> file -> fh - fileiter2 -> file -> fh;
}

char * _fileiter_tostring(fileiter_t *fileiter) {
  return file_tostring(fileiter -> file);
}

data_t * _fileiter_has_next(fileiter_t *fi) {
  data_t      *next;
  data_t      *ret;
  exception_t *ex;

  _fileiter_readnext(fi);
  next = list_head(fi -> next);
  if (data_is_exception(next)) {
    ex = data_exceptionval(next);
    ret = (ex -> code == ErrorExhausted) ? data_create(Bool, 0) : data_copy(next);
  } else {
    ret = data_true();
  }
  if (file_debug) {
    debug("%s._fileiter_has_next() -> %s", _fileiter_tostring(fi), data_tostring(ret));
  }
  return ret;
}

data_t * _fileiter_next(fileiter_t *fi) {
  _fileiter_readnext(fi);
  return list_shift(fi -> next);
}


/* ------------------------------------------------------------------------ */

/* -- F I L E  D A T A T Y P E -------------------------------------------- */

data_t * data_wrap_file(file_t *file) {
  data_t *ret = data_create_noinit(File);

  ret -> ptrval = file_copy(file);
  return ret;
}

data_t * _file_new(data_t *target, va_list arg) {
  file_t *f;
  char   *name;

  name = va_arg(arg, char *);
  if (name) {
    f = file_open(name);
    if (file_isopen(f)) {
      target -> ptrval = f;
      return target;
    } else {
      return data_exception_from_my_errno(f -> _errno);
    }
  } else {
    f = file_create(-1);
    target -> ptrval = f;
    return target;
  }
}

int _file_cmp(data_t *d1, data_t *d2) {
  file_t *f1 = data_fileval(d1);
  file_t *f2 = data_fileval(d2);

  return file_cmp(f1, f2);
}

data_t * _file_copy(data_t *dest, data_t *src) {
  dest -> ptrval = file_copy(data_fileval(src));
  return dest;
}

char * _file_tostring(data_t *data) {
  return file_tostring(data_fileval(data));
}

unsigned int _file_hash(data_t *data) {
  return strhash(data_fileval(data) -> fname);
}

data_t * _file_enter(data_t *file) {
  if (file_debug) {
    debug("%s._file_enter()", data_tostring(file));
  }
  return file;
}

data_t * _file_leave(data_t *data, data_t *param) {
  data_t *ret = param;

  if (file_close(data_fileval(data))) {
    ret = data_exception_from_my_errno(file_errno(data_fileval(data)));
  }
  if (file_debug) {
    debug("%s._file_leave() -> %s", data_tostring(data), data_tostring(ret));
  }
  return ret;
}

data_t * _file_resolve(data_t *file, char *name) {
  file_t *f = data_fileval(file);

  if (!strcmp(name, "errno")) {
    return data_create(Int, file_errno(f));
  } else if (!strcmp(name, "errormsg")) {
    return data_create(String, file_error(f));
  } else if (!strcmp(name, "name")) {
    return data_create(String, file_name(f));
  } else if (!strcmp(name, "fh")) {
    return data_create(Int, f -> fh);
  } else if (!strcmp(name, "eof")) {
    return data_create(Bool, file_eof(f));
  } else {
    return NULL;
  }
}

data_t * _file_iter(data_t *file) {
  file_t     *f = data_as_file(file);
  data_t     *ret = data_create(FileIter, f, NULL);

  if (file_debug) {
    debug("%s._file_iter() -> %s", data_tostring(file), data_tostring(ret));
  }
  return ret;
}

data_t * _file_query(data_t *file, data_t *regex) {
  file_t     *f = data_fileval(file);
  data_t     *ret = data_create(FileIter, f, regex);

  if (file_debug) {
    debug("%s._file_query(%s) -> %s",
          data_tostring(file),
          data_tostring(regex),
          data_tostring(ret));
  }
  return ret;
}

data_t * _file_read(data_t *file, char *buf, int num) {
  int retval;

  if (file_debug) {
    debug("%s.read(%d)", data_tostring(file), num);
  }
  retval = file_read(data_fileval(file), buf, num);
  if (retval >= 0) {
    return data_create(Int, retval);
  } else {
    return data_exception_from_errno();
  }
}

data_t * _file_write(data_t *file, char *buf, int num) {
  int retval;

  if (file_debug) {
    debug("%s.write(%d)", data_tostring(file), num);
  }
  retval = file_write(data_fileval(file), buf, num);
  if (retval >= 0) {
    return data_create(Int, retval);
  } else {
    return data_exception_from_errno();
  }
}

/* ----------------------------------------------------------------------- */

data_t * _file_open(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char   *n;
  data_t *ret;

  if (!args || !array_size(args)) {
    n = data_tostring(self);
  } else if (array_size(args) > 1) {
    // FIXME open mode!
    return data_exception(ErrorArgCount, "open() takes exactly one argument");
  } else {
    n = data_tostring(array_get(args, 0));
  }
  ret = data_create(File, n);
  if (!ret) {
    ret = data_create(Bool, 0);
  }
  return ret;
}

data_t * _file_adopt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *ret = data_create(File, NULL);
  int     fh = data_intval(data_array_get(args, 0));

  data_fileval(ret) -> fh = fh;
  if (file_debug) {
    debug("_file_adopt(%d) -> %s", fh, data_tostring(ret));
  }
  return ret;
}

data_t * _file_seek(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     offset = data_intval(data_array_get(args, 0));
  int     retval;
  data_t *ret;

  retval = file_seek(data_fileval(self), offset);
  if (retval >= 0) {
    ret = data_create(Int, retval);
  } else {
    ret = data_exception_from_my_errno(file_errno(data_fileval(self)));
  }
  return ret;
}

data_t * _file_readline(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = data_fileval(self);
  data_t *ret;
  char   *line;

  (void) name;
  (void) args;
  (void) kwargs;
  if (line = file_readline(file)) {
    ret = data_create(String, line);
  } else if (!file_errno(file)) {
    ret = data_null();
  } else {
    ret = data_exception_from_my_errno(file_errno(file));
  }
  return ret;
}

data_t * _file_print(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *s;
  data_t *fmt;
  char   *line;
  int     ret = 1;

  fmt = data_array_get(args, 0);
  assert(fmt);
  args = array_slice(args, 1, -1);
  s = data_execute(fmt, "format", args, kwargs);
  array_free(args);

  line = data_tostring(s);
  if (file_write(data_fileval(self), line, strlen(line)) == strlen(line)) {
    if (file_write(data_fileval(self), "\n", 1) == 1) {
      file_flush(data_fileval(self));
      ret = 1;
    }
  }
  data_free(s);
  return data_create(Bool, ret);
}

data_t * _file_close(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int retval = file_close(data_fileval(self));

  if (!retval) {
    return data_create(Bool,  1);
  } else {
    return data_exception_from_my_errno(file_errno(data_fileval(self)));
  }
}

data_t * _file_redirect(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Bool, file_redirect(data_fileval(self),
					 data_tostring(data_array_get(args, 0))) == 0);
}
