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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include <file.h>
#include <logging.h>
#include <ctype.h>

#include "array.h"

int file_debug = 0;

static void   _file_init(void) __attribute__((constructor(102)));
static FILE * _file_stream(file_t *);

void _file_init(void) {
  logging_register_category("file", &file_debug);
}

/*
 * fsentry_t -
 */

fsentry_t * fsentry_create (char *name) {
  fsentry_t *ret;

  ret = NEW(fsentry_t);
  ret -> name = strdup(name);
  ret -> exists = (stat(ret->name, &ret->statbuf)) ? errno : 0;
  return ret;
}

fsentry_t * fsentry_getentry(fsentry_t *dir, char *name) {
  char      *n;
  fsentry_t *ret;

  if (!fsentry_isdir (dir)) {
    return NULL;
  }
  n = (char *) new(strlen(dir->name) + strlen (name) + 2);
  sprintf(n, "%s/%s", dir->name, name);
  ret = fsentry_create(n);
  free(n);
  return ret;
}

void fsentry_free (fsentry_t *e) {
  if (e) {
    free(e -> name);
    free(e);
  }
}

unsigned int fsentry_hash(fsentry_t *e) {
  return strhash(e -> name);
}

char * fsentry_tostring(fsentry_t *e) {
  return e -> name;
}

int fsentry_cmp(fsentry_t *e1, fsentry_t *e2) {
  return strcmp(e1 -> name, e2 -> name);
}

int fsentry_exists(fsentry_t *e) {
  return e -> exists == 0;
}

int fsentry_isfile(fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IFREG);
}

int fsentry_isdir (fsentry_t *e) {
  return !e -> exists && (e -> statbuf.st_mode & S_IFDIR);
}

int fsentry_canread(fsentry_t *e) {
  return !access(e -> name, R_OK);
}

int fsentry_canwrite(fsentry_t *e) {
  return !access(e -> name, W_OK);
}

int fsentry_canexecute(fsentry_t *e) {
  return !access(e -> name, X_OK);
}

list_t * fsentry_getentries(fsentry_t *dir) {
  list_t        *ret;
  DIR           *dirpointer;
  struct dirent *entrypointer;

  if (!fsentry_isdir(dir)) {
    return NULL;
  }

  ret = NULL;
  dirpointer = opendir(dir -> name);
  if (dirpointer) {
    ret = list_create();
    list_set_free(ret, fsentry_free);
    list_set_hash(ret, fsentry_hash);
    list_set_tostring(ret, fsentry_tostring);
    list_set_cmp(ret, fsentry_cmp);
    for (entrypointer = readdir(dirpointer);
         entrypointer;
         entrypointer = readdir(dirpointer)) {
      list_push(ret, fsentry_getentry(dir, entrypointer -> d_name));
    }
    closedir (dirpointer);
  }
  return ret;
}

file_t * fsentry_open(fsentry_t *e) {
  if (!fsentry_isfile(e)) {
    return NULL;
  }
  return file_open(e -> name);
}

/*
 * file_t static functions
 */

FILE * _file_stream(file_t *file) {
  if (!file -> stream) {
    file -> stream = fdopen(file -> fh, "r");
  }
  return file -> stream;
}

/*
 * file_t public functions
 */

file_t * file_create(int fh) {
  file_t *ret;

  ret = NEW(file_t);
  ret -> read_fnc = (read_t) file_read;
  ret -> free = (free_t) file_free;
  ret -> fh = fh;
  ret -> stream = NULL;
  ret -> fname = NULL;
  ret -> line = NULL;
  ret -> _errno = 0;
  ret -> error = NULL;
  ret -> str = NULL;
  ret -> refs = 1;
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
      }
    }
    if (*ptr == '=') {
      for (ptr++; *ptr; ptr++) {
        switch (*ptr) {
          case 'r':
          case 'R':
            ret |= (mask & (S_IRUSR | S_IRGRP | S_IROTH));
            break;
          case 'w':
          case 'W':
            ret |= (mask & (S_IWUSR | S_IWGRP | S_IWOTH));
            break;
          case 'x':
          case 'X':
            ret |= (mask & (S_IXUSR | S_IXGRP | S_IXOTH));
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

file_t * file_copy(file_t *file) {
  file -> refs++;
  return file;
}

void file_free(file_t *file) {
  if (file) {
    file -> refs--;
    if (file -> refs <= 0) {
      if (file -> fname) {
        file_close(file);
        free(file -> fname);
      }
      free(file -> error);
      free(file -> line);
      free(file -> str);
      free(file);
    }
  }
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

char * file_tostring(file_t *file) {
  if (!file -> str) {
    asprintf(&file -> str, "%s:%d", 
             (file -> fname) ? file -> fname : "anon", 
             file -> fh);
  }
  return file -> str;
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


int file_read(file_t *file, char *target, int num) {
  int ret;

  ret = read(file -> fh, target, num);
  file -> _errno = errno;
  return ret;
}

int file_write(file_t *file, char *buf, int num) {
  int ret = write(file -> fh, buf, num);
  
  file -> _errno = errno;
  return ret;
}

int file_flush(file_t *file) {
  int ret = fsync(file -> fh);
  
  file -> _errno = errno;
  return ret;
}

char * file_readline(file_t *file) {
  size_t n = 0;
  int    num;
  FILE  *stream;
  
  free(file -> line);
  file -> line = NULL;
  stream = _file_stream(file);
  file -> _errno = 0;
  num = getline(&file -> line, &n, stream);
  if (num >= 0) {
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

int file_isopen(file_t *file) {
  return file -> fh >= 0;
}

int file_redirect(file_t *file, char *newname) {
  assert(0); /* Not yet implemented */
  return 0;
}
