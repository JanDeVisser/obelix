/*
 * /obelix/test/tdata.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <math.h>

#include <array.h>
#include <data.h>
#include <exception.h>
#include <testsuite.h>

#include "types.h"

static void _init_types(void) __attribute__((constructor(101)));

data_t * execute(data_t *self, char *name, int numargs, ...) {
  va_list  arglist;
  array_t *args;
  data_t  *ret;
  data_t  *d;
  int      ix;
  int      type;
  long     intval;
  double   dblval;
  char    *ptr;
  
  args = data_array_create(numargs);
  va_start(arglist, numargs);
  for (ix = 0; ix < numargs; ix++) {
    type = va_arg(arglist, int);
    d = NULL;
    switch (type) {
      case Int:
        intval = va_arg(arglist, long);
        d = data_create(Int, intval);
        break;
      case Float:
        intval = va_arg(arglist, double);
        d = data_create(Float, dblval);
        break;
      case String:
        ptr = va_arg(arglist, char *);
        d = data_create(String, ptr);
        break;
      case Bool:
        intval = va_arg(arglist, long);
        d = data_create(Bool, intval);
        break;
      default:
        debug("Cannot do type %d. Ignored", type);
        ptr = va_arg(arglist, char *);
        break;
    }
    if (d) {
      array_push(args, d);
    }
  }
  va_end(arglist);
  ret = data_execute(self, name, args, NULL);
  if (ret && data_is_error(ret)) {
    debug("Error executing '%s'.'%s': %s", data_debugstr(self),
	  name, data_tostring(ret));
  }
  array_free(args);
  return ret;
}

void _init_types(void) {
  set_suite_name("Types");
}
