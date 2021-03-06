/*
 * /obelix/include/function.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <data.h>
#include <name.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef data_t * (*native_t)(char *, arguments_t *);

typedef struct _function {
  data_t         _d;
  name_t        *name;
  void_t         fnc;
  int            min_params;
  int            max_params;
  int            type;
  datalist_t    *params;
} function_t;

extern function_t *    function_create(char *, void_t);
extern function_t *    function_create_noresolve(char *);
extern function_t *    function_parse(char *);
extern unsigned int    function_hash(function_t *);
extern int             function_cmp(function_t *, function_t *);
extern function_t *    function_resolve(function_t *fnc);
extern data_t *        function_call(function_t *, char *, arguments_t *);
extern char *          function_funcname(function_t *);
extern char *          function_libname(function_t *);

extern int             FunctionType;

type_skel(function, FunctionType, function_t);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __FUNCTION_H__ */
