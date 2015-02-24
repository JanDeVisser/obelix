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
#include <sys/types.h>
#include <unistd.h>

#include <file.h>
#include <logging.h>

int file_debug = 0;

static void _file_init(void) __attribute__((constructor(102)));

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
 * fsentry_t -
 */

file_t * file_create(int fh) {
  file_t *ret;

  ret = NEW(file_t);
  if (ret) {
    ret -> read_fnc = (read_t) file_read;
    ret -> free = (free_t) file_free;
    ret -> fh = fh;
    ret -> fname = NULL;
  }
  return ret;
}

file_t * file_open(char *fname) {
  file_t *ret;
  int     ok;

  ret = NEW(file_t);
  ok = 1;
  if (ret) {
    ret -> read_fnc = (read_t) file_read;
    ret -> free = (free_t) file_free;
    ret -> fname = strdup(fname);
    if (!ret -> fname) {
      ok = 0;
    } else {
      ret -> fh = open(ret -> fname, O_RDONLY);
      if (file_debug) {
        debug("file_open(%s): %d", ret->fname, ret->fh);
      }
      if (ret -> fh < 0) {
        ok = 0;
      }
    }
    if (!ok) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void file_free(file_t *file) {
  if (file) {
    if (file -> fname) {
      file_close(file);
      free(file -> fname);
    }
    free(file);
  }
}

void file_close(file_t *file) {
  if (file -> fh >= 0) {
    close(file -> fh);
    file -> fh = -1;
  }
}

int file_read(file_t *file, char *target, int num) {
  return read(file -> fh, target, num);
}
