/*
 * /obelix/include/boundmethod.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __BOUNDMETHOD_H__
#define __BOUNDMETHOD_H__

#include <closure.h>
#include <data.h>
#include <script.h>
#include <object.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _bound_method {
  script_t  *script;
  object_t  *self;
  closure_t *closure;
  char      *str;
  int        refs;
} bound_method_t;

extern bound_method_t * bound_method_create(script_t *, object_t *);
extern void             bound_method_free(bound_method_t *);
extern bound_method_t * bound_method_copy(bound_method_t *);
extern int              bound_method_cmp(bound_method_t *, bound_method_t *);
extern char *           bound_method_tostring(bound_method_t *);
extern data_t *         bound_method_execute(bound_method_t *, array_t *, dict_t *);

#define data_is_boundmethod(d)  ((d) && (data_type((d)) == BoundMethod))
#define data_boundmethodval(d)  (data_is_boundmethod((d)) ? ((bound_method_t *) (d) -> ptrval) : NULL)

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOUNDMETHOD_H__ */
