/*
 * /obelix/src/stdlib/sys.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include <data.h>
#include <exception.h>
#include <object.h>
#include <math.h>

extern char   **environ;

extern data_t * _function_getenv(char *, array_t *, dict_t *);

static void _object_set(object_t *obj, char *attr, data_t *value) {
  object_set(obj, attr, value);
  data_free(value);
}

data_t * _function_getenv(char *name, array_t *params, dict_t *kwargs) {
  object_t  *obj;
  data_t    *value;
  data_t    *ret;
  char     **e;
  char      *n = NULL;
  char      *v;
  int        len = 0;
  
  obj = object_create(NULL);
  for (e = environ; *e; e++) {
    if (strlen(*e) > len) {
      if (n) free(n);
      n = strdup(*e);
      len = strlen(n);
    } else {
      strcpy(n, *e);
    }
    v = strchr(n, '=');
    assert(v);
    *v++ = 0;
    value = data_create(String, v);
    object_set(obj, n, value);
    data_free(value);
  }
  ret = data_create(Object, obj);
  object_free(obj);
  free(n);
  return ret;
}

data_t * _function_uname(char *name, array_t *params, dict_t *kwargs) {
  object_t       *obj;
  data_t         *ret;
  struct utsname  buf;

  if (!uname(&buf)) {
    obj = object_create(NULL);
    object_set(obj, "sysname", data_create(String, buf.sysname));
    object_set(obj, "nodename", data_create(String, buf.nodename));
    object_set(obj, "release", data_create(String, buf.release));
    object_set(obj, "version", data_create(String, buf.version));
    object_set(obj, "machine", data_create(String, buf.machine));
#ifdef _GNU_SOURCE
    object_set(obj, "domainname", data_create(String, buf.domainname));
#endif
    ret = data_create(Object, obj);
    object_free(obj);
  } else {
    ret = data_exception(ErrorSysError, "Error executing uname(): %s",
                     strerror(errno));
  }
  return ret;
}

data_t * _function_exit(char *name, array_t *params, dict_t *kwargs) {
  data_t *exit_code;
  data_t *error;
  
  if (params && array_size(params)) {
    exit_code = data_copy(data_array_get(params, 0));
  } else {
    exit_code = data_create(Int, 0);
  }
  
  error = data_exception(ErrorExit, "Exit with code '%s'", data_tostring(exit_code));
  data_exceptionval(error) -> throwable = exit_code;
  return error;
}
