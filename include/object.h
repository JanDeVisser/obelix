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

#include <core.h>
#include <array.h>
#include <data.h>
#include <dict.h>

typedef struct _object {
  data_t  *constructor;
  int      constructing;
  void    *ptr;
  dict_t  *variables;
  data_t  *retval;
  char    *str;
  char    *debugstr;
  int      refs;
} object_t;

extern data_t *            data_create_object(object_t *);
#define data_is_object(d)  ((d) && (data_type((d)) == Object))
#define data_objectval(d)  (data_is_object((d)) ? ((object_t *) (d) -> ptrval) : NULL)

extern object_t *          object_create(data_t *);
extern void                object_free(object_t *);
extern object_t *          object_copy(object_t *);
extern data_t *            object_get(object_t *, char *);
extern data_t *            object_set(object_t *, char *, data_t *);
extern int                 object_has(object_t *, char *);
extern data_t *            object_call(object_t *, array_t *, dict_t *);
extern char *              object_tostring(object_t *);
extern char *              object_debugstr(object_t *);
extern unsigned int        object_hash(object_t *);
extern int                 object_cmp(object_t *, object_t *);
extern data_t *            object_resolve(object_t *, char *);
extern object_t *          object_bind_all(object_t *, data_t *);
extern data_t *            object_ctx_enter(object_t *);
extern data_t *            object_ctx_leave(object_t *, data_t *);

#endif /* __OBJECT_H__ */
