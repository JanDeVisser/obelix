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

#include <core.h>
#include <sys/stat.h>

#include <list.h>

extern int file_debug;

typedef struct _fsentry {
  char        *name;
  struct stat  statbuf;
  int          exists;
} fsentry_t;

typedef struct _file {
  read_t  read_fnc;
  free_t  free;
  int     fh;
  FILE   *stream;
  char   *fname;
  char   *line;
  int     refs;
} file_t;

extern fsentry_t *  fsentry_create(char *);
extern fsentry_t *  fsentry_getentry(fsentry_t *, char *name);
extern void         fsentry_free(fsentry_t *);
extern unsigned int fsentry_hash(fsentry_t *);
extern char *       fsentry_tostring(fsentry_t *);
extern int          fsentry_cmp(fsentry_t *, fsentry_t *);
extern int          fsentry_exists(fsentry_t *);
extern int          fsentry_isfile(fsentry_t *);
extern int          fsentry_isdir(fsentry_t *);
extern int          fsentry_canread(fsentry_t *);
extern int          fsentry_canwrite(fsentry_t *);
extern int          fsentry_canexecute(fsentry_t *);
extern list_t *     fsentry_getentries(fsentry_t *);
extern file_t *     fsentry_open(fsentry_t *);

extern file_t *     file_create(int);
extern file_t *     file_open(char *);
extern file_t *     file_copy(file_t *);
extern void         file_free(file_t *);
extern int          file_close(file_t *);
extern int          file_cmp(file_t *, file_t *);
extern int          file_read(file_t *, char *, int);
extern char *       file_readline(file_t *);
extern int          file_isopen(file_t *);

#endif /* __FILE_H__ */
