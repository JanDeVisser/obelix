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
#include <limits.h>
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
  }
  if (_singleton) {
    atexit(resolve_free);
  }
}

#ifndef MAX_PATH
#define MAX_PATH       PATH_MAX
#endif /* MAX_PATH */

char * _resolve_rewrite_image(char *image, char *buf) {
  int   len;
  int   ix;
  char  canonical_buf[MAX_PATH + 1];
  char *ptr;
  char *canonical;

  if (!image) {
    return NULL;
  }
  canonical = canonical_buf;
  len = strlen(image);
  if (len > MAX_PATH - 5) len = MAX_PATH - 5;
#ifdef __WIN32__
  for (ix = 0; ix < len; ix++) {
    canonical[ix] = tolower(image[ix]);
  }
  canonical[len] = 0;
#else /* __WIN32__ */
  strcpy(canonical, image);
#endif /* __WIN32__ */

  /* FIXME Relative paths! */
  if (ptr = strchr(canonical, '.')) {
    *ptr = 0;
  }
#ifndef __WIN32__
  strcpy(buf, "lib");
  strcat(buf, canonical);
  strcat(buf, ".so");
#else /* __WIN32__ */
  strcpy(buf, canonical);
  strcat(buf, ".dll");
#endif /* __WIN32__ */
  return buf;
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
  HMODULE   handle;
#endif /* HAVE_DLFCN_H */
  char     *err;
  char      image_plf[MAX_PATH];

  if (_resolve_rewrite_image(image, image_plf)) {
    if (resolve_debug) {
      debug("resolve_open('%s') ~ '%s'", image, image_plf);
    }
    image = image_plf;
  } else {
    if (resolve_debug) {
    	debug("resolve_open('Main Program Image')");
    }
  }
#ifdef HAVE_DLFCN_H
  dlerror();
  handle = dlopen(image, RTLD_NOW | RTLD_GLOBAL);
  err = dlerror();
#elif defined(HAVE_WINDOWS_H)
  handle = (image) ? LoadLibrary(TEXT(image)) : GetModuleHandle(NULL);
  err = itoa(GetLastError()); // FIXME
#endif /* HAVE_DLFCN_H */
  if (handle) {
		list_append(resolve -> images, handle);
    if (resolve_debug) {
    	info("resolve_open('%s') SUCCEEDED", image ? image : "'Main Program Image'");
    }
		return resolve;
	}
  error("resolve_open('%s') FAILED: %s", image ? image : "'Main Program Image'", err);
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
