/*
 * /obelix/include/generator.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <data.h>
#include <closure.h>
#include <exception.h>
#include <vm.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _generator {
  data_t       _d;
  closure_t   *closure;
  vm_t        *vm;
  exception_t *status;
} generator_t;

extern generator_t * generator_create(closure_t *, vm_t *, exception_t *);
extern generator_t * generator_copy(generator_t *);
extern char *        generator_tostring(generator_t *);
extern void          generator_free(generator_t *);
extern int           data_is_generator(data_t *);
extern generator_t * data_as_generator(data_t *);

extern int Generator = -1;
  
#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __GENERATOR_H__ */
