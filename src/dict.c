/*
 * dict.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include "core.h"
#include "dict.h"

#define INIT_BUCKETS      4

static entry_t * _entry_create(void*, void *);

// ---------------------------
// entry_t static functions

entry_t * _entry_create(void *key, void *value) {
  entry_t *entry;

  entry = NEW(entry_t);
  if (entry) {
    entry -> key = key;
    entry -> value = value;
  }
  return entry;
}

// ---------------------------
// dict_t

dict_t * dict_create(cmp_t cmp) {
  dict_t *ret;
  dict_t *d;
  int i;
  list_t *l;

  d = NEW(dict_t);
  if (d) {
    ok = FALSE;
    d -> buckets = array_create(INIT_BUCKETS);
    if (d -> buckets) {
      ret -> cmp = cmp;
      ret -> size = 0;
      ret -> num_buckets = INIT_BUCKETS;
      for (i = 0; i < array_size(ret -> buckets); i++) {
        l = list_create();
        if (!l) break;
        array_set(d -> buckets, i, l);
      }
    }
    if (!ok) {
      if (d -> buckets) {
        for (i = 0; i < array_size(ret -> buckets); i++) {
          l = array_get(ret -> buckets, i);
          if (!l) break;
          list_free(l);
        }
        array_free(d -> buckets);
    } else {
      d = ret;
    }
  }
  return ret;
}

