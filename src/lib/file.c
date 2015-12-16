/*
 * /obelix/src/file.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <config.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

#include <array.h>
#include <data.h>
#include <exception.h>
#include <file.h>
#include <logging.h>
#include <re.h>

int file_debug = 0;

#ifdef __WIN32__
#define open(...)      _open(__VA_ARGS__)
#define close(f)       _close((f))
typedef HANDLE         _oshandle_t;
#else
typedef int _oshandle_t;
#endif /* __WIN32__ */

/* ------------------------------------------------------------------------ */

static void          _file_init(void) __attribute__((constructor(110)));

static int           _stream_read(stream_t *, char *, int);
static stream_t *    _stream_enter(stream_t *);
static data_t *      _stream_query(stream_t *, data_t *, array_t *);
static data_t *      _stream_iter(stream_t *);
static data_t *      _stream_readline(stream_t *, char *, array_t *, dict_t *);
static data_t *      _stream_print(stream_t *, char *, array_t *, dict_t *);

static _oshandle_t   _file_oshandle(file_t *);

static void          _file_free(file_t *);
static char *        _file_allocstring(file_t *file);
static int           _file_intval(file_t *);
static data_t *      _file_cast(file_t *, int);
static data_t *      _file_leave(file_t *, data_t *);
static data_t *      _file_resolve(file_t *, char *);

static data_t *      _file_open(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_adopt(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_close(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_isopen(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_redirect(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_seek(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_flush(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_stream[] = {
  { .id = FunctionRead,        .fnc = (void_t) _stream_read },
  { .id = FunctionEnter,       .fnc = (void_t) _stream_enter },
  { .id = FunctionIter,        .fnc = (void_t) _stream_iter },
  { .id = FunctionQuery,       .fnc = (void_t) _stream_query },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_stream[] = {
  { .type = -1,     .name = "readline", .method = (method_t) _stream_readline, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "print",    .method = (method_t) _stream_print,    .argtypes = { String, Any,    NoType }, .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,       .method = NULL,                        .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static vtable_t _vtable_file[] = {
  { .id = FunctionCmp,         .fnc = (void_t) file_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _file_free },
  { .id = FunctionAllocString, .fnc = (void_t) _file_allocstring },
  { .id = FunctionIntValue,    .fnc = (void_t) _file_intval },
  { .id = FunctionCast,        .fnc = (void_t) _file_cast },
  { .id = FunctionHash,        .fnc = (void_t) file_hash },
  { .id = FunctionLeave,       .fnc = (void_t) _file_leave },
  { .id = FunctionResolve,     .fnc = (void_t) _file_resolve },
  { .id = FunctionWrite,       .fnc = (void_t) file_write },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_file[] = {
  { .type = Any,    .name = "open",     .method = _file_open,     .argtypes = { String, Int, Any },       .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "adopt",    .method = _file_adopt,    .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "close",    .method = _file_close,    .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isopen",   .method = _file_isopen,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "redirect", .method = _file_redirect, .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "seek",     .method = _file_seek,     .argtypes = { Int, NoType, NoType },    .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "flush",    .method = _file_flush,    .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,           .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/* ------------------------------------------------------------------------ */

typedef struct _streamiter {
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

static vtable_t _vtable_streamiter[] = {
  { .id = FunctionCmp,         .fnc = (void_t) _streamiter_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _streamiter_free },
  { .id = FunctionAllocString, .fnc = (void_t) _streamiter_allocstring },
  { .id = FunctionHasNext,     .fnc = (void_t) _streamiter_has_next },
  { .id = FunctionNext,        .fnc = (void_t) _streamiter_next },
  { .id = FunctionNone,        .fnc = NULL }
};

int Stream = -1;
int File = -1;
int StreamIter = -1;

#define data_is_streamiter(d)  ((d) && data_hastype((data_t *) (d), StreamIter))
#define data_as_streamiter(d)  (data_is_streamiter((d)) ? ((streamiter_t *) (d)) : NULL)
#define streamiter_copy(o)     ((streamiter_t *) data_copy((data_t *) (o)))
#define streamiter_free(o)     (data_free((data_t *) (o)))
#define streamiter_tostring(o) (data_tostring((data_t *) (o)))

/* ------------------------------------------------------------------------ */

void _file_init(void) {
  logging_register_category("file", &file_debug);
  Stream = typedescr_create_and_register(Stream, "stream", _vtable_stream, _methoddescr_stream);
  File = typedescr_create_and_register(File, "file", _vtable_file, _methoddescr_file);
  typedescr_assign_inheritance(typedescr_get(File), Stream);
  StreamIter = typedescr_create_and_register(StreamIter, "streamiterator", _vtable_streamiter, NULL);
}


/* -- S T R E A M  A B S T R A C T  T Y P E ------------------------------- */

int _stream_read(stream_t *stream, char *buf, int num) {
  int     retval;
  int     ix;
  char   *ptr;
  int     ch;
  data_t *ret;

  if (file_debug) {
    debug("%s.read(%d)", data_tostring((data_t *) stream), num);
  }
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

stream_t * _stream_enter(stream_t *stream) {
  if (file_debug) {
    debug("%s._stream_enter()", data_tostring((data_t *) stream));
  }
  return stream;
}

data_t * _stream_iter(stream_t *stream) {
  streamiter_t *ret;

  ret = _streamiter_create(stream, NULL);

  if (file_debug) {
    debug("%s._stream_iter() -> %s", data_tostring((data_t *) stream), data_tostring((data_t *) ret));
  }
  return (data_t *) ret;
}

data_t * _stream_query(stream_t *stream, data_t *selector, array_t *params) {
  streamiter_t *ret = _streamiter_create(stream, selector);

  (void) params;
  if (file_debug) {
    debug("%s._stream_query(%s) -> %s",
          data_tostring((data_t *) stream),
          data_tostring(selector),
          streamiter_tostring(ret));
  }
  return (data_t *) ret;
}

data_t * _stream_readline(stream_t *stream, char *name, array_t *args, dict_t *kwargs) {
  char   *line;
  data_t *ret;

  (void) name;
  (void) args;
  (void) kwargs;

  line = stream_readline(stream);
  if (line) {
    ret = data_create(String, line);
    free(line);
  } else if (stream -> _errno) {
    ret = data_exception_from_errno();
  } else {
    ret = data_null();
  }
  return ret;
}

data_t * _stream_print(stream_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     ret = 1;
  data_t *retval = NULL;
  data_t *fmt;

  (void) name;
  fmt = data_array_get(args, 0);
  assert(fmt);
  args = array_slice(args, 1, -1);
  ret = stream_print(self, data_tostring(fmt), args, kwargs);
  array_free(args);
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
  stream -> _eof = 0;
  stream -> _errno = 0;
  stream -> error = NULL;
  return stream;
}

void stream_free(stream_t *stream) {
  if (stream -> buffer) {
    str_free(stream -> buffer);
  }
  free(stream -> error);
}

char * stream_error(stream_t *stream) {
  if (stream -> _errno) {
    free(stream -> error);
    stream -> error = strdup(strerror(stream -> _errno));
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
    ret = str_read_from_reader(stream -> buffer, stream, stream -> reader);
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
    ret = str_read_from_reader(stream -> buffer, stream, stream -> reader);
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

char * stream_readline(stream_t *stream) {
  char   *buf;
  int     num = 0;
  int     ch;
  int     cursz = 40;

  buf = (char *) _new(cursz);
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
        buf = resize_block(buf, cursz * 2, cursz);
        cursz *= 2;
      }
    }
  } while (ch > 0);
  while (*buf && iscntrl(buf[strlen(buf) - 1])) {
    buf[strlen(buf) - 1] = 0;
  }
  return buf;
}

#define WRITE_TO_STREAM(s, l) if (retval >= 0) {                          \
    int _len = strlen((l));                                               \
    if (file_debug) {                                                     \
      debug("Writing %d bytes to %s", _len, data_tostring((data_t *) s)); \
    }                                                                     \
    retval = (s -> writer)((s), (l), _len);                               \
    if (file_debug) {                                                     \
      if (retval >= 0) {                                                  \
        debug("Wrote %d bytes", retval);                                  \
      } else {                                                            \
        debug("error: %d", errno);                                        \
      }                                                                   \
    }                                                                     \
}

int stream_print(stream_t *stream, char *fmt, array_t *args, dict_t *kwargs) {
  data_t *s;
  char   *line;
  char   *buf;
  int     retval = 0;
  data_t *r = NULL;

  s = data_execute((r = data_create(String, fmt)), "format", args, kwargs);
  data_free(r);

  line = data_tostring(s);
  buf = (char *) _new(strlen(line) +
#ifndef __WIN32__
    2
#else
    3
#endif /* __WIN32__ */
          );
  strcpy(buf, line);
#ifdef __WIN32__
  strcat(buf, "\r");
#endif /* __WIN32__ */
  strcat(buf, "\n");
  WRITE_TO_STREAM(stream, buf);
  if ((retval >= 0) && data_hasmethod((data_t *) stream, "flush")) {
    r = data_execute((data_t *) stream, "flush", NULL, NULL);
    if ((data_type(r) != Bool) || !data_intval(r)) {
      retval = -1;
    }
    data_free(r);
  }
  data_free(s);
  return retval;
}

/* -- S T R E A M  I T E R A T O R ---------------------------------------- */

streamiter_t * _streamiter_readnext(streamiter_t *iter) {
  data_t  *matches;
  array_t *matchvals;
  int      ix;
  data_t  *line;
  array_t *args;

  while (list_empty(iter -> next)) {
    line = data_execute(iter -> stream, "readline", NULL, NULL);
    if (data_isnull(line)) {
      list_push(iter -> next,
                data_exception(ErrorExhausted, "Iterator exhausted"));
    } else if (data_is_exception(line)) {
      list_push(iter -> next, data_copy(line));
    } else {
      if (!iter -> selector) {
        list_push(iter -> next, data_copy(line));
      } else {
        args = array_create(1);
        array_push(args, line);
        matches = data_execute(iter -> selector, "match", args, NULL);
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
        array_free(args);
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
    retval = data_execute(ret -> stream, "seek", NULL, NULL);
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
    asprintf(&buf, "%s",
	     file_tostring(streamiter -> stream));
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
    ret = (ex -> code == ErrorExhausted) ? data_create(Bool, 0) : data_copy(next);
  } else {
    ret = data_true();
  }
  if (file_debug) {
    debug("%s._streamiter_has_next() -> %s", streamiter_tostring(si), data_tostring(ret));
  }
  return ret;
}

data_t * _streamiter_next(streamiter_t *si) {
  _streamiter_readnext(si);
  return list_shift(si -> next);
}

/* -- F I L E _ T  S T A T I C  F U N C T I O N S ------------------------- */

_oshandle_t _file_oshandle(file_t *file) {
#ifdef __WIN32__
  return (_oshandle_t) _get_osfhandle(file -> fh);
#else
  return file -> fh;
#endif /* __WIN32__ */
}

void _file_free(file_t *file) {
  if (file ) {
    if (file -> fname) {
      file_close(file);
      free(file -> fname);
    }
    stream_free((stream_t *) file);
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
    return data_create(Bool, file_isopen(file));
  }
  return NULL;
}

data_t * _file_leave(file_t *file, data_t *param) {
  data_t *ret = param;

  if (file_close(file)) {
    ret = data_exception_from_my_errno(file_errno(file));
  }
  if (file_debug) {
    debug("%s._file_leave() -> %s", file_tostring(file), data_tostring(ret));
  }
  return ret;
}

data_t * _file_resolve(file_t *file, char *name) {
  if (!strcmp(name, "errno")) {
    return data_create(Int, file_errno(file));
  } else if (!strcmp(name, "errormsg")) {
    return data_create(String, file_error(file));
  } else if (!strcmp(name, "name")) {
    return data_create(String, file_name(file));
  } else if (!strcmp(name, "fh")) {
    return data_create(Int, file -> fh);
  } else if (!strcmp(name, "eof")) {
    return data_create(Bool, file_eof(file));
  } else {
    return NULL;
  }
}

/* -- F I L E _ T  P U B L I C  F U N C T I O N S ------------------------- */

file_t * file_create(int fh) {
  file_t *ret;

  ret = data_new(File, file_t);
  ret -> fh = fh;
  ret -> fname = NULL;
  stream_init((stream_t *) ret, (read_t) file_read, (write_t) file_write);
  return ret;
}

int file_flags(char *flags) {
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

int file_mode(char *mode) {
  int      ret = 0;
  array_t *parts;
  char    *str;
  char    *ptr;
  int      ix;
  int      mask;

  parts = array_split(mode, ",");
  for (ix = 0; (ret != -1) && (ix < array_size(parts)); ix++) {
    str = str_array_get(parts, ix);
    mask = 0;
    for (ptr = str; *ptr && (*ptr != '='); ptr++) {
      switch (*ptr) {
        case 'u':
        case 'U':
          mask |= S_IRWXU;
          break;
#ifndef __WIN32__ // FIXME
        case 'g':
        case 'G':
          mask |= S_IRWXG;
          break;
        case 'o':
        case 'O':
          mask |= S_IRWXO;
          break;
        case 'a':
        case 'A':
          mask |= S_IRWXU | S_IRWXG | S_IRWXO;
          break;
#endif
      }
    }
    if (*ptr == '=') {
      for (ptr++; *ptr; ptr++) {
        switch (*ptr) {
          case 'r':
          case 'R':
#ifndef __WIN32__ // FIXME
            ret |= (mask & (S_IRUSR | S_IRGRP | S_IROTH));
#else
            ret |= (mask & S_IRUSR);
#endif
            break;
          case 'w':
          case 'W':
#ifndef __WIN32__ // FIXME
            ret |= (mask & (S_IWUSR | S_IWGRP | S_IWOTH));
#else
            ret |= (mask & S_IWUSR);
#endif
            break;
          case 'x':
          case 'X':
#ifndef __WIN32__ // FIXME
            ret |= (mask & (S_IXUSR | S_IXGRP | S_IXOTH));
#else
            ret |= (mask & S_IXUSR);
#endif
            break;
        }
      }
    } else {
      ret = -1;
    }
  }
  return ret;
}

file_t * file_open_ext(char *fname, ...) {
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
#ifndef __WIN32__ // FIXME
        open_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#else
        open_mode = S_IRUSR | S_IWUSR;
#endif
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
    debug("File open(%s)", n);
    perror("error");
    ret = file_create(-1);
    file_set_errno(ret);
  }
  ret -> fname = n;
  if (file_debug) {
    debug("file_open(%s): %d", ret -> fname, ret -> fh);
  }
  return ret;
}

file_t * file_open(char *fname) {
  return file_open_ext(fname, NULL);
}

int file_close(file_t *file) {
  int ret = 0;

  if (file -> fh >= 0) {
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
  
  if (file_debug) {
    debug("%s.file_flush", file_tostring(file));
  }
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
    if (file_debug) {
      debug("FlushFileBuffers(%d) failed. err = %d", _file_oshandle(file), err);
    }
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

data_t * _file_open(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  char   *n;
  data_t *file;

  (void) name;
  (void) kwargs;
  if (!args || !array_size(args)) {
    n = data_tostring(self);
  } else if (array_size(args) > 1) {
    // FIXME open mode!
    return data_exception(ErrorArgCount, "open() takes exactly one argument");
  } else {
    n = data_tostring(array_get(args, 0));
  }
  file = (data_t *) file_open(n);
  if (!file) {
    file = data_create(Bool, 0);
  }
  return file;
}

data_t * _file_adopt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  data_t *handle = data_array_get(args, 0);
  file_t *ret = file_create(data_intval(handle));

  (void) name;
  (void) kwargs;
  if (file_debug) {
    debug("_file_adopt(%d) -> %s", data_intval(handle), file_tostring(ret));
  }
  return (data_t *) ret;
}

data_t * _file_seek(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = (file_t *) self;
  int     offset = (args && array_size(args))
                        ? data_intval(data_array_get(args, 0)) : 0;
  int     retval;
  data_t *ret;

  (void) name;
  (void) kwargs;
  retval = file_seek(file, offset);
  if (retval >= 0) {
    ret = data_create(Int, retval);
  } else {
    ret = data_exception_from_my_errno(file_errno(file));
  }
  return ret;
}

data_t * _file_close(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = (file_t *) self;
  int     retval = file_close(file);

  (void) name;
  (void) args;
  (void) kwargs;
  if (!retval) {
    return data_create(Bool,  1);
  } else {
    return data_exception_from_my_errno(file_errno(file));
  }
}

data_t * _file_flush(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = (file_t *) self;
  int     retval = file_flush(file);

  (void) name;
  (void) args;
  (void) kwargs;
  if (!retval) {
    return data_create(Bool,  1);
  } else {
    return data_exception_from_my_errno(file_errno(file));
  }
}

data_t * _file_isopen(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = (file_t *) self;

  (void) name;
  (void) args;
  (void) kwargs;
  return data_create(Bool, file_isopen(file));
}

data_t * _file_redirect(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  return data_create(Bool, file_redirect((file_t *) self,
                     data_tostring(data_array_get(args, 0))) == 0);
}
