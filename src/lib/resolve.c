/*
 * /obelix/src/lib/resolve.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif /* HAVE_PTHREAD_H */
#include <string.h>
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif /* HAVE_WINDOWS_H */

#include <stdio.h>
#include <logging.h>
#include <resolve.h>

#define OBL_INIT    "_obl_init"

int resolve_debug = 0;

static inline void __resolve_init(void);
static char *      _resolve_rewrite_image(char *, char *);
static resolve_t * _resolve_open(resolve_t *, char *);

static resolve_t      *_singleton = NULL;
static pthread_once_t  _resolve_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t _resolve_mutex;

#define _resolve_init() pthread_once(&_resolve_once, __resolve_init)

void __resolve_init(void) {
  logging_register_category("resolve", &resolve_debug);
  assert(!_singleton);
  _singleton = NEW(resolve_t);

  _singleton -> images = list_create();
#ifdef HAVE_DLFCN_H
  list_set_free(_singleton -> images, (free_t) dlclose);
#elif defined(HAVE_WINDOWS_H)
  list_set_free(_singleton -> images, (free_t) FreeLibrary);
#endif /* HAVE_DLFCN_H */
  _singleton -> functions = strvoid_dict_create();
  if (!_resolve_open(_singleton, NULL)) {
    error("Could not load main program image");
  }
  pthread_mutex_init(&_resolve_mutex, NULL);
  atexit(resolve_free);
}

#ifndef MAX_PATH
#define MAX_PATH       PATH_MAX
#endif /* MAX_PATH */

char * _resolve_rewrite_image(char *image, char *buf) {
  int   len;
  char  canonical_buf[MAX_PATH + 1];
  char *ptr;
  char *canonical;
#ifdef __WIN32__
  int   ix;
#endif /* __WIN32__ */

  if (!image) {
    return NULL;
  }
  canonical = canonical_buf;
  len = strlen(image);
  if (len > MAX_PATH - 8) len = MAX_PATH - 8;
#ifdef __CYGWIN__
  strcpy(canonical, "cyg");
  canonical += strlen("cyg");
#endif /* __CYGWIN__ */
#ifdef __WIN32__
  for (ix = 0; ix < len; ix++) {
    canonical[ix] = tolower(image[ix]);
  }
  canonical[len] = 0;
#else /* !__WIN32__ */
  strcpy(canonical, image);
#endif /* __WIN32__ */

  /* FIXME Relative paths! */
  if ((ptr = strchr(canonical, '.'))) {
    *ptr = 0;
  }
  strcpy(buf, canonical);
#if defined(__WIN32__) || defined(__CYGWIN__)
  strcat(buf, ".dll");
#elif defined(__APPLE__)
  strcat(buf, ".dylib");
#else /* Linux */
  strcat(buf, ".so");
#endif
  return buf;
}

resolve_t * resolve_get(void) {
  _resolve_init();
  return _singleton;
}

void resolve_free(void) {
  if (_singleton) {
    mdebug(resolve, "resolve_free");
    list_free(_singleton -> images);
    dict_free(_singleton -> functions);
    free(_singleton);
    _singleton = NULL;
  }
}

resolve_t * _resolve_open(resolve_t *resolve, char *image) {
#ifdef HAVE_DLFCN_H
  void      *handle;
#elif defined(HAVE_WINDOWS_H)
  HMODULE    handle;
#endif /* HAVE_DLFCN_H */
  char      *err = NULL;
  char       image_plf[MAX_PATH];
  void_t     initializer;
  resolve_t *ret = NULL;

  pthread_mutex_lock(&_resolve_mutex);
  if (_resolve_rewrite_image(image, image_plf)) {
    mdebug(resolve, "resolve_open('%s') ~ '%s'", image, image_plf);
    image = image_plf;
  } else {
    mdebug(resolve, "resolve_open('Main Program Image')");
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
#ifdef HAVE_DLFCN_H
    dlerror();
    initializer = (void_t) dlsym(handle, OBL_INIT);
    err = dlerror();
#elif defined(HAVE_WINDOWS_H)
    initializer = (void_t) GetProcAddress(handle, OBL_INIT);
    err = itoa(GetLastError()); // FIXME
#endif /* HAVE_DLFCN_H */
    if (!err && initializer) {
      mdebug(resolve, "resolve_open('%s') Execute initializer", image ? image : "Main Program Image");
      initializer();
    } else {
      mdebug(resolve, "resolve_open('%s') No initializer", image ? image : "Main Program Image");
    }
    list_append(resolve -> images, handle);
    if (resolve_debug) {
      info("resolve_open('%s') SUCCEEDED", image ? image : "Main Program Image");
    }
    ret = resolve;
  }
  if (!ret) {
    error("resolve_open('%s') FAILED: %s", image ? image : "Main Program Image", err);
  }
  pthread_mutex_unlock(&_resolve_mutex);
  return ret;
}

resolve_t * resolve_open(resolve_t *resolve, char *image) {
  _resolve_init();
  return _resolve_open(resolve, image);
}


void_t resolve_resolve(resolve_t *resolve, char *func_name) {
  void   *handle;
  void_t  ret = NULL;
  char   *err = NULL;

  // TODO synchronize
  ret = (void_t) dict_get(resolve -> functions, func_name);
  if (ret) {
    mdebug(resolve, "Function '%s' was cached", func_name);
    return ret;
  }

  mdebug(resolve, "dlsym('%s')", func_name);
  ret = NULL;
  for (list_start(resolve -> images); !err && !ret && list_has_next(resolve -> images); ) {
    handle = list_next(resolve -> images);
#ifdef HAVE_DLFCN_H
    dlerror();
    ret = (void_t) dlsym(handle, func_name);
    err = dlerror();
#elif defined(HAVE_WINDOWS_H)
    ret = (void_t) GetProcAddress(handle, func_name);
    err = itoa(GetLastError()); // FIXME
#endif /* HAVE_DLFCN_H */
  }
  if (!err) {
    if (ret) {
      dict_put(resolve -> functions, strdup(func_name), ret);
    } else {
      info("Could not resolve function '%s'", func_name);
    }
  } else {
    error("resolve_resolve('%s') FAILED: %s", func_name, err);
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
