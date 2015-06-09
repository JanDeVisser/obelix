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

extern int file_debug;

typedef struct _fsentry {
  data_t       _d;
  char        *name;
  struct stat  statbuf;
  int          exists;
} fsentry_t;

typedef struct _file {
  data_t  _d;
  int     fh;
  FILE   *stream;
  char   *fname;
  char   *line;
  int     _errno;
  char   *error;
} file_t;

extern fsentry_t *  fsentry_create(char *);
extern fsentry_t *  fsentry_getentry(fsentry_t *, char *name);
extern unsigned int fsentry_hash(fsentry_t *);
extern int          fsentry_cmp(fsentry_t *, fsentry_t *);
extern int          fsentry_exists(fsentry_t *);
extern int          fsentry_isfile(fsentry_t *);
extern int          fsentry_isdir(fsentry_t *);
extern int          fsentry_canread(fsentry_t *);
extern int          fsentry_canwrite(fsentry_t *);
extern int          fsentry_canexecute(fsentry_t *);
extern list_t *     fsentry_getentries(fsentry_t *);
extern file_t *     fsentry_open(fsentry_t *);

extern int          file_flags(char *);
extern int          file_mode(char *);
extern file_t *     file_create(int);
extern file_t *     file_open_ext(char *, ...);
extern file_t *     file_open(char *);
extern file_t *     file_copy(file_t *);
extern void         file_free(file_t *);
extern int          file_close(file_t *);
extern char *       file_name(file_t *);
extern char *       file_error(file_t *);
extern int          file_errno(file_t *);
extern int          file_cmp(file_t *, file_t *);
extern char *       file_tostring(file_t *);
extern int          file_write(file_t *, char *, int);
extern int          file_read(file_t *, char *, int);
extern int          file_seek(file_t *, int);
extern char *       file_readline(file_t *);
extern int          file_eof(file_t *);
extern int          file_isopen(file_t *);
extern int          file_flush(file_t *);
extern int          file_redirect(file_t *, char *);

extern data_t *     data_wrap_file(file_t *file);
extern int          File;
extern int          FSEntry;

#define data_is_file(d)    ((d) && (data_hastype((d), File)))
#define data_as_file(d)    ((file_t *) (data_is_file((d)) ? ((file_t *) (d)) : NULL))
#define file_free(f)       (data_free((data_t *) (f)))
#define file_tostring(f)   (data_tostring((data_t *) (f)))
#define file_copy(f)       ((file_t *) data_copy((data_t *) (f)))

#define data_is_fsentry(d)  ((d) && (data_hastype((d), FSEntry)))
#define data_as_fsentry(d)  ((fsentry_t *) (data_is_fsentry((d)) ? ((fsentry_t *) (d)) : NULL))
#define fsentry_free(f)     (data_free((data_t *) (f)))
#define fsentry_tostring(f) (data_tostring((data_t *) (f)))
#define fsentry_copy(f)     ((fsentry_t *) data_copy((data_t *) (f)))

#endif /* __FILE_H__ */
