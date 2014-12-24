/*
 * /obelix/src/resolve.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <dlfcn.h>
#include <string.h>

#include <resolve.h>

static resolve_t *_resolve_create();
static resolve_t *singleton = NULL;

resolve_t * _resolve_create() {
  resolve_t *ret;
  int ok;
  void *main;

  assert(!singleton);
  ret = NEW(resolve_t);
  ok = 0;
  if (ret) {
    ret -> images = list_create();
    ok = (ret -> images != NULL);
    if (ok) {
      list_set_free(ret -> images, (free_t) dlclose);
      ret -> functions = dict_create((cmp_t) strcmp);
      ok = (ret -> functions != NULL);
    }
    if (ok) {
      dict_set_hash(ret -> functions, (hash_t) strhash);
      dict_set_free_key(ret -> functions, (free_t) free);
      main = resolve_open(ret, NULL);
      ok = (main != NULL);
    }
    if (!ok) {
      if (ret -> images) {
        list_free(ret -> images);
      }
      if (ret -> functions) {
        dict_free(ret -> functions);
      }
      ret = NULL;
    }
    singleton = ret;
    atexit(resolve_free);
  }
  return ret;
}

resolve_t * resolve_get() {
  resolve_t *ret;

  ret = singleton;
  if (!ret) {
    ret = _resolve_create();
  }
  return ret;
}

void resolve_free() {
  if (singleton) {
    list_free(singleton -> images);
    dict_free(singleton -> functions);
    free(singleton);
    singleton = NULL;
  }
}

resolve_t * resolve_open(resolve_t *resolve, char *image) {
  void *handle;

  handle = dlopen(image, RTLD_NOW | RTLD_GLOBAL);
  if (handle) {
    if (list_append(resolve -> images, handle)) {
      return resolve;
    }
    dlclose(handle);
  }
  return NULL;
}

void_t resolve_resolve(resolve_t *resolve, char *func_name) {
  listiterator_t *iter;
  void           *handle;
  void_t          ret;

  ret = (void_t) dict_get(resolve -> functions, func_name);
  if (ret) return ret;

  ret = NULL;
  for (iter = li_create(resolve -> images); iter && li_has_next(iter); ) {
    handle = li_next(iter);
    ret = (void_t) dlsym(handle, func_name);
    if (ret) {
      dict_put(resolve -> functions, strdup(func_name), ret);
      break;
    }
  }
  li_free(iter);
  return ret;
}

int resolve_library(char *library) {
  resolve_t *resolve;

  resolve = resolve_get();
  assert(resolve);
  return resolve_open(resolve, library) != NULL;
}

void_t resolve_function(char *func_name) {
  resolve_t *resolve;
  void_t     fnc;

  resolve = resolve_get();
  assert(resolve);
  fnc = resolve_resolve(resolve, func_name);
  if (!fnc) {
    error("Could not resolve function %s", func_name);
  }
  return fnc;
}
