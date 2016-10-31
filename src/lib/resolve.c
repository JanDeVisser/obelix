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
#include <stdio.h>
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

#include "libcore.h"
#include <resolve.h>

#define OBL_INIT    "_obl_init"

typedef struct _resolve_result {
  void     *result;
  int       errorcode;
  char     *error;
} resolve_result_t;

       int                resolve_debug = 0;

static resolve_result_t * _resolve_result_create(void *);
static void               _resolve_result_free(resolve_result_t *);

static resolve_handle_t * _resolve_handle_create(char *);
static void               _resolve_handle_free(resolve_handle_t *);
static char *             _resolve_handle_tostring(resolve_handle_t *);
static char *             _resolve_handle_get_platform_image(resolve_handle_t *);
static resolve_handle_t * _resolve_handle_open(resolve_handle_t *);
static resolve_result_t * _resolve_handle_get_function(resolve_handle_t *, char *);

static inline void        __resolve_init(void);
static resolve_t *        _resolve_open(resolve_t *, char *);

static resolve_t *        _singleton = NULL;
static pthread_once_t     _resolve_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t    _resolve_mutex;

#define _resolve_init() pthread_once(&_resolve_once, __resolve_init)

/* ------------------------------------------------------------------------ */

resolve_result_t * _resolve_result_create(void *result) {
  char             *error;
  int               errorcode;
  resolve_result_t *ret;

  errorcode = 0;
  error = NULL;
  if (!result) {
#ifdef HAVE_DLFCN_H
    error = dlerror();
    if (error && !strstr(error, "undefined symbol")) {
      error = strdup(error);
      errorcode = -1;
    } else {
      error = NULL;
    }
#elif defined(HAVE_WINDOWS_H)
    errorcode = GetLastError();
    if (errorcode == ERROR_PROC_NOT_FOUND) {
      errorcode = 0;
    } else if (errorcode) {
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, errorcode,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &error, 0, NULL);
    } else {
      error = NULL;
    }
#endif /* HAVE_DLFCN_H */
  }
  ret = NEW(resolve_result_t);
  ret -> result = (errorcode) ? NULL : result;
  ret -> error = error;
  ret -> errorcode = errorcode;
  if (ret -> error) {
    debug(resolve, "resolve_result has error '%s' (%d)", ret -> error, ret -> errorcode);
  } else {
    debug(resolve, "resolve_result OK, result is %sNULL", (ret -> result) ? "NOT " : "");
  }
  return ret;
}

void _resolve_result_free(resolve_result_t *result) {
  if (result) {
    free(result -> error);
  }
}

/* ------------------------------------------------------------------------ */

resolve_handle_t * _resolve_handle_create(char *image) {
  resolve_handle_t *ret = NEW(resolve_handle_t);

  ret -> image = image ? strdup(image) : NULL;
  ret -> platform_image = NULL;
  ret -> next = NULL;
  return _resolve_handle_open(ret);
}

void _resolve_handle_free(resolve_handle_t *handle) {
  if (handle) {
#ifdef HAVE_DLFCN_H
    dlclose(handle -> handle);
#elif defined(HAVE_WINDOWS_H)
    FreeLibrary(handle -> handle);
#endif /* HAVE_DLFCN_H */
    free(handle -> image);
    free(handle -> platform_image);
  }
}

char * _resolve_handle_tostring(resolve_handle_t *handle) {
  return handle -> image ? handle -> image : "Main Program Image";
}

char * _resolve_handle_get_platform_image(resolve_handle_t *handle) {
  int   len;
  char *ptr;
  char *canonical;
  int   ix;

  if (!handle -> image) {
    return NULL;
  }
  if (!handle -> platform_image) {
    handle -> platform_image = (char *) _new(MAX_PATH + 1);
    canonical = handle -> platform_image;
    len = strlen(handle -> image);
    if (len > MAX_PATH - 8) len = MAX_PATH - 8;
#ifdef __CYGWIN__
    strcpy(canonical, "cyg");
    canonical += strlen("cyg");
#endif /* __CYGWIN__ */
    for (ix = 0; ix < len; ix++) {
#ifdef __WIN32__
      canonical[ix] = tolower(handle -> image[ix]);
#else /* !__WIN32__ */
      canonical[ix] = handle -> image[ix];
      if (canonical[ix] == '\\') {
        canonical[ix] = '/';
      }
#endif /* __WIN32__ */
    }
    canonical[len] = 0;

    if ((ptr = strrchr(canonical, '.'))) {
      ptr++;
      if ((*ptr != '/') && (*ptr != '\\')) {
        *ptr = 0;
      }
      canonical = ptr;
    } else {
      canonical[len + 1] = 0;
      canonical[len] = '.';
      canonical += (len + 1);
    }
#if defined(__WIN32__) || defined(__CYGWIN__)
    strcpy(canonical, "dll");
#elif defined(__APPLE__)
    strcpy(canonical, "dylib");
#else
    strcpy(canonical, "so");
#endif
  }
  return handle -> platform_image;
}

resolve_handle_t * _resolve_handle_try_open(resolve_handle_t *handle, char *dir) {
  char              path[MAX_PATH + 1];
  char             *image;
  lib_handle_t      libhandle;
  resolve_result_t *res;

  image = _resolve_handle_get_platform_image(handle);
  if (image) {
    if (dir) {
      snprintf(path, MAX_PATH, "%s/%s", dir, image);
    } else {
      strncpy(path, _resolve_handle_get_platform_image(handle), MAX_PATH);
      path[MAX_PATH] = 0;
    }
    debug(resolve, "Attempting to open library '%s'", path);
  } else {
    debug(resolve, "Attempting to open main program module");
  }
#ifdef HAVE_DLFCN_H
  dlerror();
  libhandle = dlopen(image ? path : NULL, RTLD_NOW | RTLD_GLOBAL);
#elif defined(HAVE_WINDOWS_H)
  SetLastError(0);
  libhandle = (image) ? LoadLibrary(TEXT(path)) : GetModuleHandle(NULL);
#endif /* HAVE_DLFCN_H */
  res = _resolve_result_create((void *) libhandle);
  handle -> handle = (lib_handle_t) res -> result;
  if (handle -> handle) {
    debug(resolve, "Successfully opened '%s'", (image) ? path : "main program module");
  }
  _resolve_result_free(res);
  return handle;
}

resolve_handle_t * _resolve_handle_open(resolve_handle_t *handle) {
  resolve_handle_t *ret = NULL;
  char             *image;
  char             *obldir;
  resolve_result_t *result;

  image = _resolve_handle_get_platform_image(handle);
  if (image) {
    debug(resolve, "resolve_open('%s') ~ '%s'", handle -> image, image);
  } else {
    debug(resolve, "resolve_open('Main Program Image')");
  }
  handle -> handle = NULL;
  if (image) {
    obldir = getenv("OBL_DIR");
    if (obldir) {
      asprintf(&obldir, "%s/lib", obldir);
      _resolve_handle_try_open(handle, obldir);
      if (!handle -> handle) {
        strcpy(obldir + (strlen(obldir) - 3), "bin");
        _resolve_handle_try_open(handle, obldir);
      }
      if (!handle -> handle) {
        obldir[strlen(obldir) - 4] = 0;
        _resolve_handle_try_open(handle, obldir);
      }
      if (!handle -> handle) {
        free(obldir);
        asprintf(&obldir, "%s/share/lib", getenv("OBL_DIR"));
        _resolve_handle_try_open(handle, obldir);
      }
      free(obldir);
    }
    if (!handle -> handle) {
      _resolve_handle_try_open(handle, NULL);
    }
    if (!handle -> handle) {
      _resolve_handle_try_open(handle, "lib");
    }
    if (!handle -> handle) {
      _resolve_handle_try_open(handle, "bin");
    }
    if (!handle -> handle) {
      _resolve_handle_try_open(handle, "share/lib");
    }
  } else {
    _resolve_handle_try_open(handle, NULL);
  }
  if (handle -> handle) {
    ret = handle;
    if (image) {
      result = _resolve_handle_get_function(handle, OBL_INIT);
      if (result -> result) {
        debug(resolve, "resolve_open('%s') Executing initializer", _resolve_handle_tostring(handle));
        ((void_t) result -> result)();
      } else if (!result -> errorcode){
        debug(resolve, "resolve_open('%s') No initializer", _resolve_handle_tostring(handle));
      } else {
        error("resolve_open('%s') Error finding initializer: %s (%d)",
              _resolve_handle_tostring(handle), result -> error, result -> errorcode);
        ret = NULL;
      }
      _resolve_result_free(result);
    }
    if (ret) {
      debug(resolve, "Library '%s' opened successfully", _resolve_handle_tostring(handle));
    }
  } else {
    error("resolve_open('%s') FAILED",  _resolve_handle_tostring(handle));
  }
  return ret;
}

resolve_result_t * _resolve_handle_get_function(resolve_handle_t *handle, char *function_name) {
  void_t            function;

  debug(resolve, "dlsym('%s', '%s')", _resolve_handle_tostring(handle), function_name);
#ifdef HAVE_DLFCN_H
  dlerror();
  function = (void_t) dlsym(handle -> handle, function_name);
#elif defined(HAVE_WINDOWS_H)
  SetLastError(0);
  function = (void_t) GetProcAddress(handle -> handle, function_name);
#endif /* HAVE_DLFCN_H */
  return _resolve_result_create(function);
}

/* ------------------------------------------------------------------------ */

void __resolve_init(void) {
  logging_register_category("resolve", &resolve_debug);
  assert(!_singleton);
  _singleton = NEW(resolve_t);

  _singleton -> images = NULL;
  _singleton -> functions = strvoid_dict_create();
  if (!_resolve_open(_singleton, NULL)) {
    error("Could not load main program image");
  }
  pthread_mutex_init(&_resolve_mutex, NULL);
  atexit(resolve_free);
}

OBLCORE_IMPEXP resolve_t * resolve_get(void) {
  _resolve_init();
  return _singleton;
}

OBLCORE_IMPEXP void resolve_free(void) {
  resolve_handle_t *image;
  if (_singleton) {
    debug(resolve, "resolve_free");
    while (_singleton -> images) {
      image = _singleton -> images;
      _singleton -> images = image -> next;
      _resolve_handle_free(image);
    }
    dict_free(_singleton -> functions);
    free(_singleton);
    _singleton = NULL;
  }
}

resolve_t * _resolve_open(resolve_t *resolve, char *image) {
  resolve_t        *ret = NULL;
  resolve_handle_t *handle;

  pthread_mutex_lock(&_resolve_mutex);
  if ((handle = _resolve_handle_create(image))) {
    handle -> next = resolve -> images;
    resolve -> images = handle;
    ret = resolve;
  }
  pthread_mutex_unlock(&_resolve_mutex);
  return ret;
}

OBLCORE_IMPEXP resolve_t * resolve_open(resolve_t *resolve, char *image) {
  _resolve_init();
  return _resolve_open(resolve, image);
}


OBLCORE_IMPEXP void_t resolve_resolve(resolve_t *resolve, char *func_name) {
  resolve_handle_t *handle;
  void_t            ret = NULL;
  int               err = 0;
  resolve_result_t *result;

  // TODO synchronize
  ret = (void_t) dict_get(resolve -> functions, func_name);
  if (ret) {
    debug(resolve, "Function '%s' was cached", func_name);
    return ret;
  }

  debug(resolve, "dlsym('%s')", func_name);
  ret = NULL;
  for (handle = resolve -> images; handle && !err && !ret; handle = handle -> next) {
    result = _resolve_handle_get_function(handle, func_name);
    if (result -> errorcode) {
      error("Error resolving function '%s' in library '%s': %s (%d)",
            func_name, _resolve_handle_tostring(handle),
            result -> error, result -> errorcode);
      err = result -> errorcode;
    }
    ret = result -> result;
    _resolve_result_free(result);
  }
  dict_put(resolve -> functions, strdup(func_name), ret);
  return ret;
}

OBLCORE_IMPEXP int resolve_library(char *library) {
  resolve_t *resolve;

  resolve = resolve_get();
  assert(resolve);
  return resolve_open(resolve, library) != NULL;
}

OBLCORE_IMPEXP void_t resolve_function(char *func_name) {
  resolve_t *resolve;
  void_t     fnc;

  resolve = resolve_get();
  assert(resolve);
  fnc = resolve_resolve(resolve, func_name);
  return fnc;
}
