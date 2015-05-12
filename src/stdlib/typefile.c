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

#include <stdio.h>

#include <data.h>
#include <exception.h>
#include <file.h>

int File = -1;

static void          _file_init(void) __attribute__((constructor));
static data_t *      _file_new(data_t *, va_list);
static data_t *      _file_copy(data_t *, data_t *);
static int           _file_cmp(data_t *, data_t *);
static char *        _file_tostring(data_t *);
static unsigned int  _file_hash(data_t *);
static data_t *      _file_leave(data_t *, data_t *);
static data_t *      _file_query(data_t *, data_t *);
static data_t *      _file_iter(data_t *);
static data_t *      _file_has_next(data_t *);
static data_t *      _file_next(data_t *);
static data_t *      _file_resolve(data_t *, char *);

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
  { .id = FunctionLeave,    .fnc = (void_t) _file_leave },
  { .id = FunctionIter,     .fnc = (void_t) _file_iter },
  { .id = FunctionHasNext,  .fnc = (void_t) _file_has_next },
  { .id = FunctionNext,     .fnc = (void_t) _file_next },
  { .id = FunctionQuery,    .fnc = (void_t) _file_query },
  { .id = FunctionResolve,  .fnc = (void_t) _file_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_file =   {
  .type      = -1,
  .type_name = "file",
  .vtable    = _vtable_file,
};

/* FIXME Add append, delete, head, tail, etc... */
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

#define data_is_file(d)  ((d) && (data_type((d)) == File))
#define data_fileval(d)  ((file_t *) (data_is_file((d)) ? ((d) -> ptrval) : NULL))

extern int file_debug;

/*
 * --------------------------------------------------------------------------
 * List datatype functions
 * --------------------------------------------------------------------------
 */

void _file_init(void) {
  int ix;

  File = typedescr_register(&_typedescr_file);
  if (file_debug) {
    debug("File type initialized");
  }
  for (ix = 0; _methoddescr_file[ix].type != NoType; ix++) {
    if (_methoddescr_file[ix].type == -1) {
      _methoddescr_file[ix].type = File;
    }
  }
  typedescr_register_methods(_methoddescr_file);
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
      return data_create(ErrorIOError, file_error(f));
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

data_t * _file_leave(data_t *data, data_t *param) {
  data_t *ret = param;
  
  if (!file_close(data_fileval(self))) {
    ret = data_exception_from_errno();
  }
  return ret;
}

data_t* _file_resolve(data_t *, char*) {

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
  
  if (file_debug) {
    debug("_file_adopt(%d)", fh);
  }
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
    ret = data_exception_from_my_errno(data_fileval(self) -> _errno);
  }
  return ret;
}

data_t * _file_readline(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *ret;
  char   *line;

  line = file_readline(data_fileval(self));
  if (line) {
    ret = data_create(String, line);
  } else {
    ret = data_exception_from_my_errno(data_fileval(self) -> _errno);
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
  return data_create(Bool, file_close(data_fileval(self)) == 0);
}

data_t * _file_redirect(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Bool, file_redirect(data_fileval(self), 
					 data_tostring(data_array_get(args, 0))) == 0);
}
