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

#include "../include/core.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


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

unsigned int hash(void *buf, size_t size) {
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

void _debug(char *file, int line, char *msg, ...) {
  va_list args;

  va_start(args, msg);
  fprintf(stderr, "%s:%d ", file, line);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");
  va_end(args);
}

unsigned int strhash(char *str) {
  return hash(str, strlen(str));
}

static int rand_initialized = 0;
static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
static int my_seed = 3425674;

static void initialize_random(void) {
  if (!rand_initialized++) {
    srand(time(NULL) + my_seed);
  }
}

char * strrand(char *buf, size_t numchars) {
  size_t n;
  int key;
  
  initialize_random();
  if (numchars) {
    for (n = 0; n < numchars; n++) {
      key = rand() % (int) (sizeof charset - 1);
      buf[n] = charset[key];
    }
    buf[numchars] = '\0';
  }
  return buf;
}

function_ptr_t no_func_ptr = { void_fnc: (void_t) NULL };
function_t no_func = { type: Void, fnc: (void_t) NULL };

reduce_ctx * reduce_ctx_create(void *user, void *data, function_ptr_t fnc) {
  reduce_ctx *ctx;

  ctx = NEW(reduce_ctx);
  if (ctx) {
    ctx -> data = data;
    ctx -> fnc = fnc;
    ctx -> user = user;
  }
  return ctx;
}

