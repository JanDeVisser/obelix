/*
 * /obelix/include/file.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include <sys/stat.h>

#include <core.h>
#include <data.h>
#include <list.h>
#include <str.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_BUFSZ       16384

extern int file_debug;

typedef struct _stream {
  data_t     _d;
  str_t     *buffer;
  read_t     reader;
  write_t    writer;
  int        _eof;
  int        _errno;
  char      *error;
} stream_t;

OBLCORE_IMPEXP void         file_init(void);

OBLCORE_IMPEXP stream_t *   stream_init(stream_t *, read_t, write_t);
OBLCORE_IMPEXP void         stream_free(stream_t *);
OBLCORE_IMPEXP char *       stream_error(stream_t *);
OBLCORE_IMPEXP int          stream_read(stream_t *, char *, int);
OBLCORE_IMPEXP int          stream_write(stream_t *, char *, int);
OBLCORE_IMPEXP int          stream_getchar(stream_t *);
OBLCORE_IMPEXP char *       stream_readline(stream_t *);
OBLCORE_IMPEXP int          stream_print(stream_t *, char *, array_t *, dict_t *);
OBLCORE_IMPEXP int          stream_vprintf(stream_t *, char *, va_list);
OBLCORE_IMPEXP int          stream_printf(stream_t *, char *, ...);
OBLCORE_IMPEXP int          stream_eof(stream_t *);

typedef struct _file {
  stream_t  _stream;
  int       fh;
  char     *fname;
} file_t;

OBLCORE_IMPEXP int          file_flags(char *);
OBLCORE_IMPEXP int          file_mode(char *);
OBLCORE_IMPEXP file_t *     file_create(int);
OBLCORE_IMPEXP file_t *     file_open_ext(char *, ...);
OBLCORE_IMPEXP file_t *     file_open(char *);
OBLCORE_IMPEXP void         file_free(file_t *);
OBLCORE_IMPEXP int          file_close(file_t *);
OBLCORE_IMPEXP char *       file_name(file_t *);
OBLCORE_IMPEXP unsigned int file_hash(file_t *);
OBLCORE_IMPEXP int          file_cmp(file_t *, file_t *);
OBLCORE_IMPEXP int          file_write(file_t *, char *, int);
OBLCORE_IMPEXP int          file_read(file_t *, char *, int);
OBLCORE_IMPEXP int          file_seek(file_t *, int);
OBLCORE_IMPEXP int          file_isopen(file_t *);
OBLCORE_IMPEXP int          file_flush(file_t *);
OBLCORE_IMPEXP int          file_redirect(file_t *, char *);

OBLCORE_IMPEXP int          Stream;
OBLCORE_IMPEXP int          File;

#define data_is_stream(d)    ((d) && (data_hastype((data_t *) (d), Stream)))
#define data_as_stream(d)    ((stream_t *) (data_is_stream((d)) ? ((stream_t *) (d)) : NULL))
#define stream_tostring(s)   (data_tostring((data_t *) (s)))
#define stream_free(s)       (data_free((data_t *) (s)))
#define stream_copy(s)       ((stream_t *) data_copy((data_t *) (s)))

#define data_is_file(d)      ((d) && (data_hastype((data_t *) (d), File)))
#define data_as_file(d)      ((file_t *) (data_is_file((d)) ? ((file_t *) (d)) : NULL))
#define file_free(f)         (data_free((data_t *) (f)))
#define file_tostring(f)     (data_tostring((data_t *) (f)))
#define file_copy(f)         ((file_t *) data_copy((data_t *) (f)))

#define file_set_errno(f)          (((stream_t *) (f)) -> _errno = errno)
#define file_clear_errno(f)        (((stream_t *) (f)) -> _errno = 0)
#define file_errno(f)              (((stream_t *) (f)) -> _errno)
#define file_error(f)              (stream_error((stream_t *) (f)))
#define file_eof(f)                (((stream_t *) (f)) -> _eof)
#define file_getchar(f)            (stream_getchar((stream_t *) (f)))
#define file_readline(f)           (stream_readline((stream_t *) (f)))
#define file_print(s, f, a, kw)    (stream_print((stream_t *) (s), (f), (a), (kw)))
#define file_vprintf(s, f, args)   (stream_printf((stream_t *) (s), (f), args))
#ifndef _MSC_VER
#define file_printf(s, f, args...) (stream_printf((stream_t *) (s), (f), ## args))
#else
#define file_printf(s, ...)        (stream_printf((stream_t *) (s), __VA_ARGS__))
#endif /* _MSC_VER */

#ifdef __cplusplus
}
#endif

#endif /* __FILE_H__ */
