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

#include <config.h>
#include <data.h>
#include <socket.h>
#include <exception.h>

/* -------------------------------------------------------------------------*/

socket_t * _function_connect(char *name, array_t *params, dict_t *kwargs) {
  data_t   *host;
  data_t   *service;
  socket_t *socket;

  assert(params && (array_size(params) >= 2));
  host = (data_t *) array_get(params, 0);
  service = (data_t *) array_get(params, 1);

  socket = socket_create_byservice(data_tostring(host), data_tostring(service));
  return socket;
}

/*
 * TODO: Parameterize what interface we want to listen on.
 */
socket_t * _function_server(char *name, array_t *params, dict_t *kwargs) {
  data_t   *host;
  data_t   *service;
  socket_t *socket;

  assert(params && (array_size(params) >= 1));
  service = (data_t *) array_get(params, 0);

  return serversocket_create_byservice(data_tostring(service));
}

data_t * _function_listener(char *name, array_t *params, dict_t *kwargs) {
  socket_t *listener;
  data_t   *service;
  data_t   *server;
  data_t   *ret;

  assert(params && (array_size(params) >= 2));
  service = data_array_get(params, 0);
  server = data_array_get(params, 1);
  assert(data_is_callable(server));

  listener = serversocket_create_byservice(data_tostring(service));
  if (listener) {
    socket_listen(listener, connection_listener_service, server);
  }
  return data_exception_from_errno();
}

