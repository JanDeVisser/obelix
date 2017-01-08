/*
 * /obelix/include/arguments.h - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <data.h>
#include <dictionary.h>

#ifndef __ARGUMENTS_H__
#define __ARGUMENTS_H__

typedef struct _arguments {
  data_t        _d;
  datalist_t   *args;
  dictionary_t *kwargs;
} arguments_t;

type_skel(arguments, Arguments, arguments_t);

OBLCORE_IMPEXP arguments_t * arguments_create(array_t *, dict_t *);
OBLCORE_IMPEXP arguments_t * arguments_create_from_cmdline(int, char **);
OBLCORE_IMPEXP data_t *      arguments_get_arg(arguments_t *, int);
OBLCORE_IMPEXP data_t *      arguments_get_kwarg(arguments_t *, char *);

#endif /* __ARGUMENTS_H__ */
