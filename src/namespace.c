/*
 * /obelix/src/namespace.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <namespace.h>

ns_t * ns_create(script_t *script) {
  ns_t *ret;
  data_t      *data;

  ret = NEW(ns_t);
  ret -> native_allowed = 1;
  data = script_create_object(script, NULL, NULL);
  if (data_is_error(data)) {
    error_report((error_t *) data -> ptrval);
    free(ret);
    ret = NULL;
  } else {
    ret -> root = (object_t *) data -> ptrval;
    ret -> root -> refs++;
    script -> ns = ret;
  }
  data_free(data);
  return ret;
}

void ns_free(ns_t *ns) {
  if (ns) {
    object_free(ns -> root);
  }
}

object_t * ns_get(ns_t *ns, array_t *name) {
  return object_get(ns -> root, name);
}
