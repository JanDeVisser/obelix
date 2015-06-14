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
  data_t     _d;
  script_t  *script;
  object_t  *self;
  closure_t *closure;
} bound_method_t;

extern bound_method_t * bound_method_create(script_t *, object_t *);
extern int              bound_method_cmp(bound_method_t *, bound_method_t *);
extern data_t *         bound_method_execute(bound_method_t *, array_t *, dict_t *);

extern int BoundMethod;

#define data_is_bound_method(d)   ((d) && (data_hastype((data_t *) (d), BoundMethod)))
#define data_as_bound_method(d)   ((bound_method_t *) (data_is_bound_method((data_t *) (d)) ? (d) : NULL))
#define bound_method_free(o)      (data_free((data_t *) (o)))
#define bound_method_tostring(o)  (data_tostring((data_t *) (o)))
#define bound_method_copy(o)      ((bound_method_t *) data_copy((data_t *) (o)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOUNDMETHOD_H__ */
