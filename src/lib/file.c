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
typedef HANDLE _oshandle_t;
#else
typedef int _oshandle_t;
#endif /* __WIN32__ */

/* ------------------------------------------------------------------------ */

static void          _file_init(void) __attribute__((constructor));

static data_t *      _stream_enter(data_t *);
static data_t *      _stream_query(data_t *, data_t *);
static data_t *      _stream_iter(data_t *);
static data_t *      _stream_readline(data_t *, char *, array_t *, dict_t *);
static data_t *      _stream_print(data_t *, char *, array_t *, dict_t *);

static FILE *        _file_stream(file_t *);
static _oshandle_t   _file_oshandle(file_t *);

static void          _file_free(file_t *);
static char *        _file_allocstring(file_t *file);
static int           _file_intval(file_t *);
static data_t *      _file_cast(file_t *, int);
static data_t *      _file_leave(file_t *, data_t *);
static data_t *      _file_resolve(file_t *, char *);
static data_t *      _file_read(file_t *, char *, int);
static data_t *      _file_write(file_t *, char *, int);

static data_t *      _file_open(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_adopt(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_readline(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_print(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_close(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_isopen(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_redirect(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_seek(data_t *, char *, array_t *, dict_t *);
static data_t *      _file_flush(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_stream[] = {
  { .id = FunctionEnter,       .fnc = (void_t) _stream_enter },
  { .id = FunctionIter,        .fnc = (void_t) _stream_iter },
  { .id = FunctionQuery,       .fnc = (void_t) _stream_query },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_stream[] = {
  { .type = -1,     .name = "readline", .method = _stream_readline, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "print",    .method = _stream_print,    .argtypes = { String, Any,    NoType }, .minargs = 1, .varargs = 1 },
  { .type = NoType, .name = NULL,       .method = NULL,             .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
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
  { .id = FunctionRead,        .fnc = (void_t) _file_read },
  { .id = FunctionWrite,       .fnc = (void_t) _file_write },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_file[] = {
  { .type = Any,    .name = "open",     .method = _file_open,     .argtypes = { String, Int, Any },       .minargs = 1, .varargs = 1 },
  { .type = Any,    .name = "adopt",    .method = _file_adopt,    .argtypes = { Number, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "readline", .method = _file_readline, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "print",    .method = _file_print,    .argtypes = { String, Any,    NoType }, .minargs = 1, .varargs = 1 },
  { .type = -1,     .name = "close",    .method = _file_close,    .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "isopen",   .method = _file_isopen,   .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "redirect", .method = _file_redirect, .argtypes = { String, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "seek",     .method = _file_seek,     .argtypes = { Int, NoType, NoType },    .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "flush",    .method = _file_flush,    .argtypes = { NoType, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,           .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

/* ------------------------------------------------------------------------ */

typedef struct _streamiter {
  data_t   _d;
  data_t  *stream;
  data_t  *selector;
  list_t  *next;
} streamiter_t;

static streamiter_t * _streamiter_create(data_t *, data_t *);
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

data_t * _stream_enter(data_t *stream) {
  if (file_debug) {
    debug("%s._stream_enter()", data_tostring(stream));
  }
  return stream;
}

data_t * _stream_iter(data_t *stream) {
  streamiter_t *ret;

  ret = _streamiter_create(stream, NULL);

  if (file_debug) {
    debug("%s._stream_iter() -> %s", data_tostring(stream), data_tostring((data_t *) ret));
  }
  return (data_t *) ret;
}

data_t * _stream_query(data_t *stream, data_t *selector) {
  streamiter_t *ret = _streamiter_create(stream, selector);

  if (file_debug) {
    debug("%s._stream_query(%s) -> %s",
          data_tostring(stream),
          data_tostring(selector),
          streamiter_tostring(ret));
  }
  return (data_t *) ret;
}

data_t * _stream_readline(data_t *stream, char *name, array_t *args, dict_t *kwargs) {
  char buf[40];
  int  num;

  (void) name;
  (void) args;
  (void) kwargs;
  return data_null();
}

#define WRITE_TO_STREAM(s, l) if (!retval) {                                 \
	int _len = strlen((l));                                                    \
	retval = data_write((s), (l), _len);                                       \
	if (data_is_numeric(retval) && (data_intval(retval) == _len)) {            \
		data_free(retval);                                                       \
		retval = NULL;                                                           \
	}                                                                          \
}

data_t * _stream_print(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = (file_t *) self;
  data_t *s;
  data_t *fmt;
  char   *line;
  int     ret = 1;
  data_t *retval = NULL;

  (void) name;
  fmt = data_array_get(args, 0);
  assert(fmt);
  args = array_slice(args, 1, -1);
  s = data_execute(fmt, "format", args, kwargs);
  array_free(args);

  line = data_tostring(s);
  WRITE_TO_STREAM(self, line);
#ifdef __WIN32__
  WRITE_TO_STREAM(self, "\r");
#endif /* __WIN32__ */
  WRITE_TO_STREAM(self, "\n");
	if (!retval && data_hasmethod(self, "flush")) {
		retval = data_execute(self, "flush", NULL, NULL);
		if (data_type(retval) == Bool) {
			data_free(retval);
			retval = NULL;
		}
	}
  data_free(s);
  if (!retval) {
  	retval = data_true();
  }
  return data_create(Bool, retval);
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
        } else {
        	list_push(iter -> next,
										data_exception(ErrorType,
											 "streamiter: %s.match(%s) returned invalid object '%s'",
											 data_tostring(iter -> selector), data_tostring(line),
											 data_tostring(matches)));
        }
        data_free(matches);
        array_free(args);
      }
    }
    data_free(line);
  }
  return iter;
}

streamiter_t * _streamiter_create(data_t *stream, data_t *selector) {
  streamiter_t  *ret = data_new(StreamIter, streamiter_t);
  data_t        *retval = NULL;

  ret -> stream = data_copy(stream);
  if (data_hasmethod(selector, "match")) {
    ret -> selector = data_copy(selector);
  } else if (selector && !data_isnull(selector)) {
    ret -> selector = data_create(Regexp, data_tostring(selector), NULL);
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

FILE * _file_stream(file_t *file) {
  if (!file -> stream) {
    file -> stream = fdopen(file -> fh, "r");
  }
  return file -> stream;
}

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
    free(file -> error);
    free(file -> line);
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

data_t * _file_read(file_t *file, char *buf, int num) {
  int retval;

  if (file_debug) {
    debug("%s.read(%d)", file_tostring(file), num);
  }
  retval = file_read(file, buf, num);
  if (retval >= 0) {
    return data_create(Int, retval);
  } else {
    return data_exception_from_errno();
  }
}

data_t * _file_write(file_t *file, char *buf, int num) {
  int retval;

  if (file_debug) {
    debug("%s.write(%d)", file_tostring(file), num);
  }
  retval = file_write(file, buf, num);
  if (retval >= 0) {
    return data_create(Int, retval);
  } else {
    return data_exception_from_errno();
  }
}

/* -- F I L E _ T  P U B L I C  F U N C T I O N S ------------------------- */

file_t * file_create(int fh) {
  file_t *ret;

  ret = data_new(File, file_t);
  ret -> fh = fh;
  ret -> stream = NULL;
  ret -> fname = NULL;
  ret -> line = NULL;
  ret -> _errno = 0;
  ret -> error = NULL;
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
    ret -> _errno = errno;
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
    if (file -> stream) {
      ret = fclose(file -> stream) != 0;
      file -> stream = NULL;
    } else {
      ret = close(file -> fh);
    }
    file -> _errno = errno;
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

char * file_error(file_t *file) {
  if (file -> _errno) {
    free(file -> error);
    file -> error = strdup(strerror(file -> _errno));
  }
  return file -> error;
}

int file_errno(file_t *file) {
  return file -> _errno;
}

int file_cmp(file_t *f1, file_t *f2) {
  return f1 -> fh - f2 -> fh;
}

int file_seek(file_t *file, int offset) {
  int   whence = (offset >= 0) ? SEEK_SET : SEEK_END;
  off_t o = (whence == SEEK_SET) ? offset : -offset;
  int   ret = lseek(file -> fh, o, whence);

  if (ret < 0) {
    file -> _errno = errno;
  }
  return ret;
}

int file_read(file_t *file, char *target, int num) {
  int ret = read(file -> fh, target, num);

  if (ret < 0) {
    file -> _errno = errno;
  }
  return ret;
}

int file_write(file_t *file, char *buf, int num) {
  int ret = write(file -> fh, buf, num);

  if (ret < 0) {
    file -> _errno = errno;
  }
  return ret;
}

int file_printf(file_t *file, char *fmt, ...) {
  char    *buf;
  va_list  args;
  int      ret;

  va_start(args, fmt);
  vasprintf(&buf, fmt, args);
  va_end(args);
  ret = file_write(file, buf, strlen(buf));
  free(buf);
  return ret;
}

int file_flush(file_t *file) {
  int   ret = 0;
#ifdef HAVE_FSYNC
  ret = fsync(file -> fh);
#elif defined(HAVE_FLUSHFILEBUFFERS)
  DWORD err;

  if (!FlushFileBuffers(_file_oshandle(file))) {
    err = GetLastError();
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
	file -> _errno = (ret < 0) ? errno : 0;
  return ret;
}

char * file_readline(file_t *file) {
  size_t n = 0;
  int    num = 0;
  FILE  *stream;

  free(file -> line);
  file -> line = NULL;
  stream = _file_stream(file);
  file -> _errno = 0;
#ifdef HAVE_GETLINE
  num = getline(&file -> line, &n, stream);
#endif /* HAVE_GETLINE */
  if (num != -1) {
    while ((num >= 0) && iscntrl(*(file -> line + num))) {
      *(file -> line + num) = 0;
      num--;
    }
    return file -> line;
  } else {
    file -> _errno = errno;
    return NULL;
  }
}

int file_eof(file_t *file) {
  return feof(_file_stream(file));
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

data_t * _file_readline(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  file_t *file = (file_t *) self;
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
  file_t *file = (file_t *) self;
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
  if (file_write(file, line, strlen(line)) == strlen(line)) {
    if (file_write(file, "\n", 1) == 1) {
      file_flush(file);
      ret = 1;
    }
  }
  data_free(s);
  return data_create(Bool, ret);
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
