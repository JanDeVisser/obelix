/*
 * /obelix/include/object.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <libparser.h>
#include <array.h>
#include <data.h>
#include <dict.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _object {
  data_t   _d;
  data_t  *constructor;
  int      constructing;
  void    *ptr;
  dict_t  *variables;
  data_t  *retval;
} object_t;

OBLPARSER_IMPEXP object_t *          object_create(data_t *);
OBLPARSER_IMPEXP object_t *          object_copy(object_t *);
OBLPARSER_IMPEXP data_t *            object_get(object_t *, char *);
OBLPARSER_IMPEXP data_t *            object_set(object_t *, char *, data_t *);
OBLPARSER_IMPEXP int                 object_has(object_t *, char *);
OBLPARSER_IMPEXP data_t *            object_call(object_t *, array_t *, dict_t *);
OBLPARSER_IMPEXP unsigned int        object_hash(object_t *);
OBLPARSER_IMPEXP int                 object_cmp(object_t *, object_t *);
OBLPARSER_IMPEXP data_t *            object_resolve(object_t *, char *);
OBLPARSER_IMPEXP object_t *          object_bind_all(object_t *, data_t *);
OBLPARSER_IMPEXP data_t *            object_ctx_enter(object_t *);
OBLPARSER_IMPEXP data_t *            object_ctx_leave(object_t *, data_t *);

#define data_is_object(d)     ((d) && (data_hastype((data_t *) (d), Object)))
#define data_as_object(d)     ((object_t *) (data_is_object((data_t *) (d)) ? (d) : NULL))
#define object_free(o)        (data_free((data_t *) (o)))
#define object_tostring(o)    (data_tostring((data_t *) (o)))
#define object_debugstr(o)    (data_tostring((data_t *) (o)))
#define object_copy(o)        ((object_t *) data_copy((data_t *) (o)))
#define data_create_object(o) data_create(Object, (o))

OBLPARSER_IMPEXP int obj_debug;
OBLPARSER_IMPEXP int Object;

#ifdef __cplusplus
}
#endif

#endif /* __OBJECT_H__ */
