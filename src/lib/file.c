/*
 * /obelix/src/lib/file.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "libcore.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif /* HAVE_STRINGS_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <array.h>
#include <data.h>
#include <exception.h>
#include <file.h>
#include <logging.h>
#include <re.h>
#include <str.h>

int file_debug = 0;

typedef int _oshandle_t;

/* ------------------------------------------------------------------------ */

static stream_t *    _stream_new(stream_t *, va_list args);
static void          _stream_free(stream_t *);
static data_t *      _stream_resolve(stream_t *, char *);
static stream_t *    _stream_enter(stream_t *);
static data_t *      _stream_query(stream_t *, data_t *, array_t *);
static data_t *      _stream_iter(stream_t *);
static data_t *      _stream_readline(stream_t *, char *, arguments_t *);
static data_t *      _stream_print(stream_t *, char *, arguments_t *);
static void *        _stream_reduce_children(stream_t *, reduce_t, void *);

static _oshandle_t   _file_oshandle(file_t *);

static file_t *      _file_new(file_t *, va_list);
static void          _file_free(file_t *);
static char *        _file_allocstring(file_t *file);
static int           _file_intval(file_t *);
static data_t *      _file_cast(file_t *, int);
static data_t *      _file_leave(file_t *, data_t *);
static data_t *      _file_resolve(file_t *, char *);
static void *        _file_reduce_children(file_t *, reduce_t, void *);

extern data_t *      _file_open(char *, arguments_t *);
extern data_t *      _file_adopt(char *, arguments_t *);

static data_t *      _file_close(data_t *, char *, arguments_t *);
static data_t *      _file_isopen(data_t *, char *, arguments_t *);
static data_t *      _file_redirect(data_t *, char *, arguments_t *);
static data_t *      _file_seek(data_t *, char *, arguments_t *);
static data_t *      _file_flush(data_t *, char *, arguments_t *);

static vtable_t _vtable_Stream[] = {
  { .id = FunctionNew,         .fnc = (void_t) _stream_new },
  { .id = FunctionFree,        .fnc = (void_t) _stream_free },
  { .id = FunctionResolve,     .fnc = (void_t) _stream_resolve },
  { .id = FunctionRead,        .fnc = (void_t) stream_read },
  { .id = FunctionWrite,       .fnc = (void_t) stream_write },
  { .id = FunctionEnter,       .fnc = (void_t) _stream_enter },
  { .id = FunctionIter,        .fnc = (void_t) _stream_iter },
  { .id = FunctionQuery,       .fnc = (void_t) _stream_query },
  { .id = FunctionReduce,      .fnc = (void_t) _stream_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Stream[] = {
  { .type = -1,     .name = "readline", .method = (method_t) _stream_readline, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "print",    .method = (method_t) _stream_print,    .argtypes = { String, Any,    NoType }, .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,       .method = NULL,                        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static vtable_t _vtable_File[] = {
  { .id = FunctionNew,         .fnc = (void_t) _file_new },
  { .id = FunctionFree,        .fnc = (void_t) _file_free },
  { .id = FunctionCmp,         .fnc = (void_t) file_cmp },
  { .id = FunctionAllocString, .fnc = (void_t) _file_allocstring },
  { .id = FunctionIntValue,    .fnc = (void_t) _file_intval },
  { .id = FunctionCast,        .fnc = (void_t) _file_cast },
  { .id = FunctionHash,        .fnc = (void_t) file_hash },
  { .id = FunctionLeave,       .fnc = (void_t) _file_leave },
  { .id = FunctionResolve,     .fnc = (void_t) _file_resolve },
  { .id = FunctionReduce,      .fnc = (void_t) _file_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_File[] = {
  { .type = -1,     .name = "close",    .method = _file_close,    .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isopen",   .method = _file_isopen,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "redirect", .method = _file_redirect, .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "seek",     .method = _file_seek,     .argtypes = { Int, NoType, NoType },    .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "flush",    .method = _file_flush,    .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,           .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/* ------------------------------------------------------------------------ */

typedef struct Streamiter {
  data_t   _d;
  data_t  *stream;
  data_t  *selector;
  list_t  *next;
} streamiter_t;

static streamiter_t * _streamiter_create(stream_t *, data_t *);
static void           _streamiter_free(streamiter_t *);
static int            _streamiter_cmp(streamiter_t *, streamiter_t *);
static char *         _streamiter_allocstring(streamiter_t *);
static data_t *       _streamiter_has_next(streamiter_t *);
static data_t *       _streamiter_next(streamiter_t *);
static streamiter_t * _streamiter_readnext(streamiter_t *);
static data_t *       _streamiter_interpolate(streamiter_t *, arguments_t *);
static void *         _streamiter_reduce_children(streamiter_t *, reduce_t, void *);

static vtable_t _vtable_StreamIter[] = {
  { .id = FunctionCmp,         .fnc = (void_t) _streamiter_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _streamiter_free },
  { .id = FunctionAllocString, .fnc = (void_t) _streamiter_allocstring },
  { .id = FunctionHasNext,     .fnc = (void_t) _streamiter_has_next },
  { .id = FunctionNext,        .fnc = (void_t) _streamiter_next },
  { .id = FunctionInterpolate, .fnc = (void_t) _streamiter_interpolate },
  { .id = FunctionReduce,      .fnc = (void_t) _streamiter_reduce_children },
  { .id = FunctionNone,        .fnc = NULL }
};

int Stream = -1;
int File = -1;
int StreamIter = -1;

type_skel(streamiter, StreamIter, streamiter_t);

/* ------------------------------------------------------------------------ */

void file_init(void) {
  if (File < 1) {
    logging_register_module(file);
    typedescr_register_with_methods(Stream, stream_t);
    typedescr_register_with_methods(File, file_t);
    typedescr_assign_inheritance(File, Stream);
    typedescr_register(StreamIter, streamiter_t);
  }
  assert(File);
  assert(Stream);
  assert(StreamIter);
}


/* -- S T R E A M  A B S T R A C T  T Y P E ------------------------------- */

stream_t * _stream_new(stream_t *stream, va_list args) {
  stream -> _eof = 0;
  stream -> _errno = 0;
  stream -> error = NULL;
  return stream;
}

void _stream_free(stream_t *stream) {
  if (stream) {
    str_free(stream -> buffer);
    data_free(stream -> error);
  }
}

stream_t * _stream_enter(stream_t *stream) {
  debug(file, "%s._stream_enter()", data_tostring((data_t *) stream));
  return stream;
}

data_t * _stream_resolve(stream_t *stream, char *name) {
  if (!strcmp(name, "errno")) {
    return int_to_data(stream -> _errno);
  } else if (!strcmp(name, "errormsg")) {
    return str_to_data(data_tostring(stream_error(stream)));
  } else if (!strcmp(name, "error")) {
    return (stream_error(stream)) ? data_copy(stream_error(stream)) : data_null();
  } else if (!strcmp(name, "eof")) {
    return int_as_bool(stream_eof(stream));
  } else {
    return NULL;
  }
}

data_t * _stream_iter(stream_t *stream) {
  streamiter_t *ret;

  ret = _streamiter_create(stream, NULL);

  debug(file, "%s._stream_iter() -> %s", data_tostring((data_t *) stream), data_tostring((data_t *) ret));
  return (data_t *) ret;
}

data_t * _stream_query(stream_t *stream, data_t *selector, array_t *params) {
  streamiter_t *ret = _streamiter_create(stream, selector);

  (void) params;
  debug(file, "%s._stream_query(%s) -> %s",
        data_tostring((data_t *) stream),
        data_tostring(selector),
        streamiter_tostring(ret));
  return (data_t *) ret;
}

void * _stream_reduce_children(stream_t *stream, reduce_t reducer, void *ctx) {
  return reducer(stream->error, reducer(stream->buffer, ctx));
}

/* ------------------------------------------------------------------------ */

data_t * _stream_readline(stream_t *stream, char _unused_ *name, arguments_t _unused_ *args) {
  char   *line;
  data_t *ret;

  line = stream_readline(stream);
  if (line) {
    ret = str_to_data(line);
    free(line);
  } else if (stream -> _errno) {
    ret = data_exception_from_errno();
  } else {
    ret = data_null();
  }
  return ret;
}

data_t * _stream_print(stream_t *self, char _unused_ *name, arguments_t *args) {
  int     ret = 1;
  data_t *retval = NULL;
  data_t *fmt;

  args = arguments_shift(args, &fmt);
  ret = stream_print(self, data_tostring(fmt), args);
  arguments_free(args);
  if (ret < 0) {
    retval = data_exception_from_my_errno(self -> _errno);
  } else {
    retval = data_true();
  }
  return retval;
}

/* ------------------------------------------------------------------------ */

stream_t * stream_init(stream_t *stream, read_t reader, write_t writer) {
  stream -> reader = reader;
  stream -> writer = writer;
  return stream;
}

data_t * stream_error(stream_t *stream) {
  if (stream -> _errno) {
    data_free(stream -> error);
    stream -> error = data_exception_from_my_errno(stream -> _errno);
  }
  return stream -> error;
}

int stream_getchar(stream_t *stream) {
  int ch;
  int ret;

  if (!stream -> buffer) {
    stream -> buffer = str_create(STREAM_BUFSZ);
    if (!stream -> buffer) {
      errno = 1;
      stream -> _errno = 1;
      return -1;
    }
    ret = str_read_from_stream(stream -> buffer, stream, stream -> reader);
    if (ret < 0) {
      stream -> _errno = errno;
      return ret;
    } else if (!ret) {
      stream -> _eof = 1;
      return ret;
    }
  }
  ch = str_readchar(stream -> buffer);
  if (!ch) {
    ret = str_read_from_stream(stream -> buffer, stream, stream -> reader);
    if (ret < 0) {
      stream -> _errno = errno;
      return ret;
    } else if (!ret) {
      stream -> _eof = 1;
      return 0;
    } else {
      ch = str_readchar(stream -> buffer);
    }
  }
  if (ch < 0) {
    stream -> _errno = errno;
    return -1;
  }
  return ch;
}

int stream_eof(stream_t *stream) {
  return stream -> _eof;
}

int stream_read(stream_t *stream, char *buf, int num) {
  int     ix;
  char   *ptr;
  int     ch;

  debug(file, "%s.read(%d)", data_tostring((data_t *) stream), num);
  ptr = buf;
  for (ix = 0; ix < num; ix++) {
    ch = stream_getchar(stream);
    if (ch > 0) {
      *ptr++ = ch;
    } else if (ch == 0) {
      break;
    } else {
      return -1;
    }
  }
  return ix;
}

static inline int _stream_write_buffer(stream_t *stream, char *buf, int num, int retval) {
  if (!retval) {
    int _len = (num > 0) ? num : strlen(buf);
    if (_len > 0) {
      debug(file, "Writing %d bytes to %s", _len, stream_tostring(stream));
      retval = (stream->writer)(stream, buf, _len);
      if (retval > 0) {
        debug(file, "Wrote %d bytes", retval);
        retval = (retval == _len) ? 0 : -1;
      } else {
        debug(file, "error: %s (%d)", strerror(errno), errno);
        retval = -1;
      }
    }
  }
  return retval;
}

int stream_write(stream_t *stream, char *buf, int num) {
  return _stream_write_buffer(stream, buf, num, 0);
}

char * stream_readline(stream_t *stream) {
  char   *buf;
  size_t  num = 0;
  int     ch;
  size_t  cursz = 40;
  size_t  newsz;
  char   *ptr;

  buf = stralloc(cursz);
  do {
    ch = stream_getchar(stream);
    if ((!ch && !num) || (ch < 0)) {
      free(buf);
      return NULL;
    } else if (!ch || (ch == '\n')) {
      break;
    } else {
      buf[num++] = ch;
      if (num >= cursz) {
        newsz = (size_t)(cursz * 1.6);
        buf = resize_block(buf, newsz, cursz);
        cursz = newsz;
      }
    }
  } while (ch > 0);
  ptr = buf + (strlen(buf) - 1);
  while (*buf && iscntrl(*ptr)) {
    *ptr-- = 0;
  }
  return buf;
}

int _stream_print_data(stream_t *stream, data_t *s) {
  int     retval = 0;
  data_t *r = NULL;

  retval = _stream_write_buffer(stream, data_tostring(s), 0, 0);
  retval = _stream_write_buffer(stream, "\r\n", 2, retval);
  if (!retval && data_hasmethod((data_t *) stream, "flush")) {
    r = data_execute((data_t *) stream, "flush", NULL);
    if ((data_type(r) != Bool) || !data_intval(r)) {
      retval = -1;
    }
    data_free(r);
  }
  return retval;
}

int stream_print(stream_t *stream, char *fmt, arguments_t *args) {
  str_t *s;
  int    ret;

  if (strstr(fmt, "${")) {
    s = str_format(fmt, args);
  } else {
    s = str_wrap(fmt);
  }
  ret = _stream_print_data(stream, (data_t *) s);
  str_free(s);
  return ret;
}

int stream_vprintf(stream_t *stream, char *fmt, va_list args) {
  str_t *s;
  int    ret;

  s = str_vformatf(fmt, args);
  ret = _stream_print_data(stream, (data_t *) s);
  str_free(s);
  return ret;
}

int stream_printf(stream_t *stream, char *fmt, ...) {
  va_list args;
  int      ret;

  va_start(args, fmt);
  ret = stream_vprintf(stream, fmt, args);
  va_end(args);
  return ret;
}

/* -- S T R E A M  I T E R A T O R ---------------------------------------- */

streamiter_t * _streamiter_readnext(streamiter_t *iter) {
  data_t      *matches;
  array_t     *matchvals;
  int          ix;
  data_t      *line;
  arguments_t *args;

  while (list_empty(iter -> next)) {
    line = data_execute(iter -> stream, "readline", NULL);
    if (data_isnull(line)) {
      list_push(iter -> next,
                data_exception(ErrorExhausted, "Iterator exhausted"));
    } else if (data_is_exception(line)) {
      list_push(iter -> next, data_copy(line));
    } else {
      if (!iter -> selector) {
        list_push(iter -> next, data_copy(line));
      } else {
        args = arguments_create_args(1, line);
        matches = data_execute(iter -> selector, "match", args);
        if (data_is_exception(matches) || data_is_string(matches)) {
          list_push(iter -> next, data_copy(matches));
        } else if (data_is_list(matches)) {
          /* FIXME: Should be able to handle any iterable */
          matchvals = data_as_array(matches);
          for (ix = 0; ix < array_size(matchvals); ix++) {
            list_push(iter -> next, data_copy(data_array_get(matchvals, ix)));
          }
        }
        data_free(matches);
        arguments_free(args);
      }
    }
    data_free(line);
  }
  return iter;
}

streamiter_t * _streamiter_create(stream_t *stream, data_t *selector) {
  streamiter_t  *ret = data_new(StreamIter, streamiter_t);
  data_t        *retval = NULL;

  ret -> stream = data_copy((data_t *) stream);
  if (selector && data_hasmethod(selector, "match")) {
    ret -> selector = data_copy(selector);
  } else if (selector && !data_isnull(selector)) {
    ret -> selector = (data_t *) regexp_create(data_tostring(selector), NULL);
  } else {
    ret -> selector = NULL;
  }
  // FIXME: Error handling for bad regexp.
  if (data_hasmethod(ret -> stream, "seek")) {
    retval = data_execute(ret -> stream, "seek", NULL);
  }
  ret -> next = data_list_create();
  if (!retval || data_intval(retval) >= 0) {
    _streamiter_readnext(ret);
  } else {
    list_push(ret -> next, data_copy(retval));
  }
  data_free(retval);
  return ret;
}

void _streamiter_free(streamiter_t *streamiter) {
  if (streamiter) {
    data_free(streamiter -> stream);
    data_free(streamiter -> selector);
    list_free(streamiter -> next);
  }
}

int _streamiter_cmp(streamiter_t *streamiter1, streamiter_t *streamiter2) {
  int ret;

  ret = data_cmp(streamiter1 -> stream, streamiter2 -> stream);
  if (!ret) {
    ret = data_cmp(streamiter1 -> selector, streamiter2 -> selector);
  }
  return ret;
}

char * _streamiter_allocstring(streamiter_t *streamiter) {
  char *buf;

  if (streamiter -> selector) {
    asprintf(&buf, "%s `%s`",
        data_tostring(streamiter -> stream),
        data_tostring(streamiter -> selector));
  } else {
    buf = strdup(data_tostring(streamiter -> stream));
  }
  return buf;
}

data_t * _streamiter_has_next(streamiter_t *si) {
  data_t      *next;
  data_t      *ret;
  exception_t *ex;

  _streamiter_readnext(si);
  next = list_head(si -> next);
  if (data_is_exception(next)) {
    ex = data_as_exception(next);
    ret = (ex -> code == ErrorExhausted) ? data_false() : data_copy(next);
  } else {
    ret = data_true();
  }
  debug(file, "%s._streamiter_has_next() -> %s", streamiter_tostring(si), data_tostring(ret));
  return ret;
}

data_t * _streamiter_next(streamiter_t *si) {
  _streamiter_readnext(si);
  return list_shift(si -> next);
}

data_t * _streamiter_interpolate(streamiter_t *si, arguments_t *args) {
  if (si -> selector) {
    si -> selector = data_interpolate(si -> selector, args);
  }
  return (data_t *) si;
}

void * _streamiter_reduce_children(streamiter_t *si, reduce_t reducer, void *ctx) {
  ctx = list_reduce(si->next, reducer, ctx);
  return reducer(si->selector, reducer(si->stream, ctx));
}

/* ------------------------------------------------------------------------ */

/* -- F I L E _ T  S T A T I C  F U N C T I O N S ------------------------- */

_oshandle_t _unused_ _file_oshandle(file_t *file) {
  return file -> fh;
}

file_t * _file_new(file_t *file, va_list args) {
  file -> fh = va_arg(args, int);
  file -> fname = NULL;
  stream_init((stream_t *) file, (read_t) file_read, (write_t) file_write);
  return file;
}

void _file_free(file_t *file) {
  if (file && file -> fname) {
    file_close(file);
    free(file -> fname);
  }
}

char * _file_allocstring(file_t *file) {
  char *buf;

  asprintf(&buf, "%s:%d",
	   (file -> fname) ? file -> fname : "anon",
	   file -> fh);
  return buf;
}

int _file_intval(file_t *file) {
  return file -> fh;
}

data_t * _file_cast(file_t *file, int totype) {
  if (totype == Bool) {
    return int_as_bool(file_isopen(file));
  }
  return NULL;
}

data_t * _file_leave(file_t *file, data_t *param) {
  data_t *ret = param;

  if (file_close(file)) {
    ret = data_exception_from_my_errno(file_errno(file));
  }
  debug(file, "%s._file_leave() -> %s", file_tostring(file), data_tostring(ret));
  return ret;
}

data_t * _file_resolve(file_t *file, char *name) {
  if (!strcmp(name, "name")) {
    return str_to_data(file_name(file));
  } else if (!strcmp(name, "fh")) {
    return int_to_data(file -> fh);
  } else {
    return NULL;
  }
}

void * _file_reduce_children(file_t *file, reduce_t reducer, void *ctx) {
  return ctx;
}

/* -- F I L E _ T  P U B L I C  F U N C T I O N S ------------------------- */

file_t * file_create(int fh) {
  file_init();
  return (file_t *) data_create(File, fh);
}

int file_flags(const char *flags) {
  int ret;

  if (!strcasecmp(flags, "r")) {
    ret = O_RDONLY;
  } else if (!strcasecmp(flags, "r+")) {
    ret = O_RDWR;
  } else if (!strcasecmp(flags, "w")) {
    ret = O_WRONLY | O_TRUNC | O_CREAT;
  } else if (!strcasecmp(flags, "w+")) {
    ret = O_RDWR | O_TRUNC | O_CREAT;
  } else if (!strcasecmp(flags, "a")) {
    ret = O_WRONLY | O_APPEND | O_CREAT;
  } else if (!strcasecmp(flags, "a+")) {
    ret = O_RDWR | O_APPEND | O_CREAT;
  } else {
    ret = -1;
  }
  return ret;
}

int file_mode(const char *mode) {
  unsigned int  ret = 0;
  array_t      *parts;
  char         *str;
  char         *ptr;
  int           ix;
  unsigned int  mask = 0;

  parts = array_split(mode, ",");
  for (ix = 0; (ret != -1) && (ix < array_size(parts)); ix++) {
    str = str_array_get(parts, ix);
    mask = 0;
    for (ptr = str; *ptr && (*ptr != '='); ptr++) {
      switch (*ptr) {
        case 'u':
        case 'U':
          mask |= (unsigned int) S_IRWXU;
          break;
        case 'g':
        case 'G':
          mask |= (unsigned int) S_IRWXG;
          break;
        case 'o':
        case 'O':
          mask |= (unsigned int) S_IRWXO;
          break;
        case 'a':
        case 'A':
          mask |= (unsigned int) S_IRWXU | (unsigned int) S_IRWXG | (unsigned int) S_IRWXO;
          break;
      }
    }
    if (*ptr == '=') {
      for (ptr++; *ptr; ptr++) {
        switch (*ptr) {
          case 'r':
          case 'R':
            ret |= (mask & ((unsigned int) S_IRUSR | (unsigned int) S_IRGRP | (unsigned int) S_IROTH));
            break;
          case 'w':
          case 'W':
            ret |= (mask & ((unsigned int) S_IWUSR | (unsigned int) S_IWGRP | (unsigned int) S_IWOTH));
            break;
          case 'x':
          case 'X':
            ret |= (mask & ((unsigned int) S_IXUSR | (unsigned int) S_IXGRP | (unsigned int) S_IXOTH));
            break;
        }
      }
    } else {
      ret = -1;
    }
  }
  return (int) ret;
}

file_t * file_open_ext(const char *fname, ...) {
  file_t  *ret = NULL;
  char    *n;
  int      fh = -1;
  va_list  args;
  char    *flags;
  char    *mode;
  int      open_flags = 0;
  int      open_mode = 0;

  n = strdup(fname);
  va_start(args, fname);
  flags = va_arg(args, char *);
  errno = 0;
  if (flags && flags[0]) {
    open_flags = file_flags(flags);
    if (open_flags == -1) {
      errno = EINVAL;
    } else {
      if (open_flags & O_CREAT) {
        mode = va_arg(args, char *);
      }
      if (mode && mode[0]) {
        open_mode = file_mode(mode);
        if (open_mode == -1) {
          errno = EINVAL;
        }
      } else {
        open_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
      }
    }
  } else {
    open_flags = O_RDONLY;
    open_mode = 0;
  }
  va_end(args);
  if (!errno) {
    fh = (open_mode) ? open(n, open_flags, open_mode) : open(n, open_flags);
  }
  if (fh >= 0) {
    ret = file_create(fh);
  } else {
    if (file_debug) {
      _debug("File open(%s)", n);
      perror("error");
    }
    ret = file_create(-1);
    file_set_errno(ret);
  }
  ret -> fname = n;
  debug(file, "file_open(%s): %d", ret -> fname, ret -> fh);
  return ret;
}

file_t * file_open(const char *fname) {
  return file_open_ext(fname, NULL);
}

int file_close(file_t *file) {
  int ret = 0;

  if (file -> fh >= 0) {
    if (file->fname && *(file->fname)) {
      debug(file, "Closing %s", file->fname);
    } else {
      debug(file, "Closing file #%d", file->fh);
    }
    ret = close(file -> fh);
    if (ret) {
      if (errno) {
        file_set_errno(file);
      } else {
        ret = 0;
      }
    }
    file -> fh = -1;
  }
  return ret;
}

char * file_name(file_t *file) {
  return file -> fname;
}

unsigned int file_hash(file_t *file) {
  return strhash(file -> fname);
}

int file_cmp(file_t *f1, file_t *f2) {
  return f1 -> fh - f2 -> fh;
}

int file_seek(file_t *file, int offset) {
  int   whence = (offset >= 0) ? SEEK_SET : SEEK_END;
  off_t o = (whence == SEEK_SET) ? offset : -offset;
  int   ret = lseek(file -> fh, o, whence);

  if (ret < 0) {
    file_set_errno(file);
  }
  return ret;
}

int file_read(file_t *file, char *target, int num) {
  int ret = read(file -> fh, target, num);

  if (ret < 0) {
    file_set_errno(file);
  }
  return ret;
}

int file_write(file_t *file, char *buf, int num) {
  int ret = write(file -> fh, buf, num);

  if (ret < 0) {
    file_set_errno(file);
  }
  return ret;
}

int file_flush(file_t *file) {
  int   ret = 0;

  debug(file, "%s.file_flush", file_tostring(file));
  if (file -> fh <= 2) {
    return 0;
  }
#ifdef HAVE_FSYNC
  ret = fsync(file -> fh);
#elif defined(HAVE_FLUSHFILEBUFFERS)
  DWORD err;

  /*
   * From the FlushFileBuffer docs:
   * The function fails if hFile is a handle to the console output. That is
   * because the console output is not buffered. The function returns FALSE,
   * and GetLastError returns ERROR_INVALID_HANDLE.
   */
  file_clear_errno(file);
  if (!FlushFileBuffers(_file_oshandle(file))) {
    err = GetLastError();
    debug(file, "FlushFileBuffers(%d) failed. err = %d", _file_oshandle(file), err);
    ret = -1;
    switch (err) {
      case ERROR_ACCESS_DENIED:
        /*
         * For a read-only handle, fsync should succeed, even though we have
         * no way to sync the access-time changes.
         */
        ret = 0;
        break;
      case ERROR_INVALID_HANDLE:
        errno = EINVAL;
        break;
      default:
        errno = EIO;
        break;
    }
  }
#endif /* HAVE_FSYNC */
  if (ret < 0) {
    file_set_errno(file);
  }
  return ret;
}

int file_isopen(file_t *file) {
  return (file) ? (file -> fh >= 0) : 0;
}

int file_redirect(file_t *file, char *newname) {
  assert(0); /* Not yet implemented */
  return 0;
}

/* -- F I L E  D A T A T Y P E  M E T H O D S ----------------------------- */

data_t * _file_open(char _unused_ *name, arguments_t *args) {
  char   *n;
  data_t *file;

  if (datalist_size(args -> args) > 1) {
    // FIXME open mode!
    return data_exception(ErrorArgCount, "open() takes exactly one argument");
  } else {
    n = arguments_arg_tostring(args, 0);
  }
  file = (data_t *) file_open(n);
  if (!file) {
    file = data_false();
  }
  return file;
}

data_t * _file_adopt(char _unused_ *name, arguments_t *args) {
  data_t *handle = arguments_get_arg(args, 0);
  file_t *ret = file_create(data_intval(handle));

  debug(file, "_file_adopt(%d) -> %s", data_intval(handle), file_tostring(ret));
  return (data_t *) ret;
}

data_t * _file_seek(data_t *self, char _unused_ *name, arguments_t *args) {
  file_t *file = (file_t *) self;
  int     offset = (arguments_has_args(args))
                        ? data_intval(arguments_get_arg(args, 0)) : 0;
  int     retval;
  data_t *ret;

  retval = file_seek(file, offset);
  if (retval >= 0) {
    ret = int_to_data(retval);
  } else {
    ret = data_exception_from_my_errno(file_errno(file));
  }
  return ret;
}

data_t * _file_close(data_t *self, char _unused_ *name, arguments_t _unused_ *args) {
  file_t *file = (file_t *) self;
  int     retval = file_close(file);

  if (!retval) {
    return data_true();
  } else {
    return data_exception_from_my_errno(file_errno(file));
  }
}

data_t * _file_flush(data_t *self, char _unused_ *name, arguments_t _unused_ *args) {
  file_t *file = (file_t *) self;
  int     retval = file_flush(file);

  if (!retval) {
    return data_true();
  } else {
    return data_exception_from_my_errno(file_errno(file));
  }
}

data_t * _file_isopen(data_t *self, char _unused_ *name, arguments_t _unused_ *args) {
  file_t *file = (file_t *) self;

  return int_as_bool(file_isopen(file));
}

data_t * _file_redirect(data_t *self, char _unused_ *name, arguments_t _unused_ *args) {
  return int_as_bool(
    file_redirect((file_t *) self, arguments_arg_tostring(args, 0)) == 0);
}
