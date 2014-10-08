/*
 * core.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdlib.h>

#include <core.h>

void * new(int sz) {
  void * ret = malloc(sz);
  if (sz && !ret) {
    errno = ENOMEM;
  }
  return ret;
}

void * new_ptrarray(int sz) {
  void * ret = calloc(sz, sizeof(void *));
  if (sz && !ret) {
    errno = ENOMEM;
  }
  return ret;
}

void * resize_block(void *block, int newsz) {
  void * ret = realloc(block, newsz);
  if (newsz && !ret) {
    errno = ENOMEM;
  }
  return ret;
}

void * resize_ptrarray(void *array, int newsz) {
  return resize_block(array, newsz * sizeof(void *));
}

int hash(void *buf, int size) {
  int hash = 5381;
  int i;
  int c;
  unsigned char *data = (unsigned char *) buf;

  for (i = 0; i < size; i++) {
    c = *data++;
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

