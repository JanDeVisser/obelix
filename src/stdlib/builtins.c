/*
 * /obelix/src/stdlib/builtins.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>
#include <unistd.h>

#include <array.h>
#include <data.h>

__DLL_EXPORT__ _unused_ data_t * _function_print(char *_unused_ func_name, arguments_t *args) {
  data_t  *fmt = NULL;
  data_t  *s;

  assert(args && arguments_args_size(args));
  if ((arguments_args_size(args) > 1) || (arguments_kwargs_size(args))) {
    args = arguments_shift(args, &fmt);
    s = data_interpolate(fmt, args);
    arguments_free(args);
  } else {
    s = arguments_get_arg(args, 0);
  }
  if (!data_is_exception(s)) {
    printf("%s\n", data_tostring(s));
    if (fmt) {
      data_free(s);
      data_free(fmt);
    }
    s = data_true();
  }
  return s;
}

__DLL_EXPORT__ _unused_ data_t * _function_sleep(char _unused_ *func_name, arguments_t *args) {
  data_t       *naptime;

  assert(args && arguments_args_size(args));
  naptime = data_uncopy(arguments_get_arg(args, 0));
  assert(naptime);
  return int_to_data(sleep((unsigned int) data_intval(naptime)));
}

__DLL_EXPORT__ _unused_ data_t * _function_usleep(char _unused_ *func_name, arguments_t *args) {
  data_t  *naptime;

  assert(args && arguments_args_size(args));
  naptime = data_uncopy(arguments_get_arg(args, 0));
  assert(naptime);
  return int_to_data(usleep((useconds_t) data_intval(naptime)));
}
