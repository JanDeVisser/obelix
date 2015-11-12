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

#include <config.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif /* HAVE_DLFCN_H */
#include <string.h>
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

#include <logging.h>
#include <resolve.h>

int resolve_debug = 0;

static void _resolve_init(void) __attribute__((constructor));
static resolve_t *_singleton = NULL;

void _resolve_init(void) {
  void *main;

  logging_register_category("resolve", &resolve_debug);
  assert(!_singleton);
  _singleton = NEW(resolve_t);

  _singleton -> images = list_create();
  list_set_free(_singleton -> images,
#ifdef HAVE_DLFCN_H
  		(free_t) dlclose
#elif defined(HAVE_WINDOWS_H)
			(free_t) FreeLibrary
#endif /* HAVE_DLFCN_H */
  );
  _singleton -> functions = strvoid_dict_create();
  main = resolve_open(_singleton, NULL);
  if (!main) {
    error("Could not load main program image");
    list_free(_singleton -> images);
    dict_free(_singleton -> functions);
    _singleton = NULL;
  } else {
    atexit(resolve_free);
  }
}

resolve_t * resolve_get() {
  return _singleton;
}

void resolve_free() {
  if (_singleton) {
    if (resolve_debug) {
      debug("resolve_free");
    }
    list_free(_singleton -> images);
    dict_free(_singleton -> functions);
    free(_singleton);
    _singleton = NULL;
  }
}

resolve_t * resolve_open(resolve_t *resolve, char *image) {
#ifdef HAVE_DLFCN_H
  void     *handle;
#elif defined(HAVE_WINDOWS_H)
  HINSTANCE handle;
#endif /* HAVE_DLFCN_H */
  int       err;

  if (resolve_debug) {
    debug("dlopen('%s')", image);
  }
#ifdef HAVE_DLFCN_H
  dlerror();
  handle = dlopen(image, RTLD_NOW | RTLD_GLOBAL);
  err = dlerror();
  if (handle) {
    if (list_append(resolve -> images, handle)) {
      return resolve;
    }
    dlclose(handle);
  }
#elif defined(HAVE_WINDOWS_H)
  handle = LoadLibrary(TEXT(image));
  if (handle) {
    if (list_append(resolve -> images, handle)) {
      return resolve;
    }
    FreeLibrary(handle);
  } else {
  	err = GetLastError();
  }
#endif /* HAVE_DLFCN_H */
  error("dlopen('%s') FAILED: %s", image, err);
  return NULL;
}

void_t resolve_resolve(resolve_t *resolve, char *func_name) {
  listiterator_t *iter;
  void           *handle;
  void_t          ret;

  // TODO synchronize
  ret = (void_t) dict_get(resolve -> functions, func_name);
  if (ret) {
    if (resolve_debug) {
      debug("Function '%s' was cached", func_name);
    }
    return ret;
  }

  if (resolve_debug) {
    debug("dlsym('%s')", func_name);
  }  
  ret = NULL;
  for (list_start(resolve -> images); !ret && list_has_next(resolve -> images); ) {
    handle = list_next(resolve -> images);
#ifdef HAVE_DLFCN_H
    ret = (void_t) dlsym(handle, func_name);
#elif defined(HAVE_WINDOWS_H)
    ret = (void_t) GetProcAddress(handle, func_name);
#endif /* HAVE_DLFCN_H */
  }
  if (ret) {
    dict_put(resolve -> functions, strdup(func_name), ret);
  } else {
    if (resolve_debug) {
      error("Could not resolve function '%s'", func_name);
    }
  }
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
  return fnc;
}
