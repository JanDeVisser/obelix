/*
 * /obelix/src/stdlib/getenv.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <stdlib.h>
#include <data.h>
#include <object.h>

extern char   **environ;

extern data_t * _function_getenv(char *, array_t *, dict_t *);

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
