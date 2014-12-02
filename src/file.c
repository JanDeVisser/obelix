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

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <file.h>

file_t * file_create(int fh) {
  file_t *ret;

  ret = NEW(file_t);
  if (ret) {
    ret -> read_fnc = (read_t) file_read;
    ret -> fh = fh;
    ret -> fname = NULL;
  }
  return ret;
}

file_t * file_open(char *fname) {
  file_t *ret;
  int ok;

  ret = NEW(file_t);
  ok = 1;
  if (ret) {
    ret -> read_fnc = (read_t) file_read;
    ret -> fname = strdup(fname);
    if (!ret -> fname) {
      ok = 0;
    } else {
      ret -> fh = open(ret -> fname, O_RDONLY);
      debug("file_open(%s): %d", ret -> fname, ret -> fh);
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
