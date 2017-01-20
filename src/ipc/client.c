/*
 * /obelix/src/ipc/client.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libipc.h"

extern void     client_init(void);

static data_t * _client_new(client_t *, va_list);
static void     _client_free(client_t *);
static char   * _client_tostring(client_t *);
static data_t * _client_resolve(client_t *, char *);

static vtable_t _vtable_Client[] = {
  {.id = FunctionNew,         .fnc = (void_t) _client_new},
  {.id = FunctionFree,        .fnc = (void_t) _client_free},
  {.id = FunctionResolve,     .fnc = (void_t) _client_resolve},
  {.id = FunctionAllocString, .fnc = (void_t) _client_tostring},
  {.id = FunctionNone,        .fnc = NULL}
};

int Client = -1;

/* ------------------------------------------------------------------------ */

void client_init(void) {
  if (Client < 1) {
    typedescr_register(Client, client_t);
  }
}

/* -- S E R V E R  T Y P E  F U N C T I O N S  ---------------------------- */

data_t * _client_new(client_t *client, va_list args) {
  mountpoint_t *mountpoint = va_arg(args, mountpoint_t *);
  socket_t     *socket;
  data_t       *ret = NULL;
  data_t       *version;

  socket = socket_open(mountpoint->remote);
  if (socket_error(socket)) {
    ret = data_copy(socket_error(socket));
    socket_free(socket);
    return ret;
  }
  client->socket = (stream_t *) socket;
  client->mountpoint = mountpoint;
  version = protocol_send_handshake(client->socket, mountpoint);
  if (!data_is_exception(version)) {
    if (!mountpoint->version) {
      mountpoint->version = strdup(data_tostring(version));
    }
    ret = (data_t *) client;
  } else {
    socket_free(socket);
    ret = version;
  }
  return ret;
}

char * _client_tostring(client_t *client) {
  char *buf;

  asprintf(&buf, "Obelix IPC Client for '%s'",
      uri_tostring(client->mountpoint->remote));
  return buf;
}

void _client_free(client_t *client) {
  if (client) {
    stream_free(client->socket);
  }
}

data_t * _client_resolve(client_t *client, char *name) {
  if (!strcmp(name, "mountpoint")) {
    return (data_t *) mountpoint_copy(client->mountpoint);
  } else if (!strcmp(name, "socket")) {
    return (data_t *) stream_copy(client->socket);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

data_t * client_run(client_t *client, remote_t *remote, arguments_t *args) {
  data_t          *ret = NULL;
  data_t          *err;
  char            *path;
  char            *mp_path = uri_path(client->mountpoint->remote);
  char            *remote_path = name_tostring_sep(remote->name, "/");
  servermessage_t *msg;

  if (!mp_path || !*mp_path) {
    path = strdup(remote_path);
  } else {
    path = stralloc(strlen(mp_path) + strlen(remote_path) + 1);
    strcpy(path, mp_path);
    strcat(path, "/");
    strcat(path, remote_path);
  }
  msg = servermessage_create(OBLSERVER_CODE_CALL, 2,
      data_tostring(client->mountpoint), path);
  servermessage_set_payload(msg, (data_t *) args);
  ret = protocol_send_message(client->socket, msg);
  servermessage_free(msg);

  if (!ret) {
    ret = protocol_expect(client->socket, OBLSERVER_CODE_DATA, 1, Int);
    if (data_is_servermessage(ret)) {
      msg = data_as_servermessage(ret);
      ret = data_copy(msg->payload);
      servermessage_free(msg);
    }
  }
  if (!data_is_exception(ret) || data_as_exception(ret)->handled) {
    if ((err = protocol_expect(client->socket, OBLSERVER_CODE_READY, 0))) {
      data_free(ret);
      ret = err;
    } else {
      data_free(err);
    }
  }
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t *client_create(mountpoint_t *mountpoint) {
  ipc_init();
  return data_create(Client, mountpoint);
}

