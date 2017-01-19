/*
 * obelix/src/stdlib/net.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <net.h>

/* -------------------------------------------------------------------------*/

__DLL_EXPORT__ _unused_ socket_t * _function_connect(_unused_ char *name, arguments_t *args) {
  char     *host;
  char     *service;
  socket_t *socket;

  assert(args && (arguments_args_size(args) >= 2));
  host = arguments_arg_tostring(args, 0);
  service = arguments_arg_tostring(args, 1);

  socket = socket_create_byservice(host, service);
  return socket;
}

/*
 * TODO: Parameterize what interface we want to listen on.
 */
__DLL_EXPORT__ _unused_ socket_t * _function_server(_unused_ char *name, arguments_t *args) {
  char *service;

  assert(args && (arguments_args_size(args) >= 1));
  service = arguments_arg_tostring(args, 1);

  return serversocket_create_byservice(service);
}

__DLL_EXPORT__ _unused_ data_t * _function_listener(_unused_ char *name, arguments_t *args) {
  socket_t *listener;
  data_t   *server;

  assert(args && (arguments_args_size(args) >= 2));
  listener = _function_server(NULL, args);
  if (listener) {
    server = arguments_get_arg(args, 1);
    assert(data_is_callable(server));
    socket_listen(listener, connection_listener_service, server);
  }
  return data_exception_from_errno();
}
