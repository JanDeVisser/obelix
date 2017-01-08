/*
 * /obelix/src/bin/client.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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
#include <time.h>

#include <net.h>
#include <array.h>
#include <exception.h>
#include <name.h>

#include "obelix.h"

static inline void   _client_init(void);

static data_t *      _client_new(client_t *, va_list);
static void          _client_free(client_t *);
static char *        _client_tostring(client_t *);
static data_t *      _client_resolve(client_t *, char *);

static data_t *      _client_handshake(client_t *);
static data_t *      _client_expect(client_t *, int, int, ...);
static data_t *      _client_read_data(client_t *, int);

static vtable_t _vtable_Client[] = {
  { .id = FunctionNew,         .fnc = (void_t) _client_new },
  { .id = FunctionFree,        .fnc = (void_t) _client_free },
  { .id = FunctionResolve,     .fnc = (void_t) _client_resolve },
  { .id = FunctionAllocString, .fnc = (void_t) _client_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

int Client = -1;

/* ------------------------------------------------------------------------ */

void client_init(void) {
  if (Client < 0) {
    typedescr_register(Client, client_t);
  }
}

/* -- S E R V E R  T Y P E  F U N C T I O N S  ---------------------------- */

data_t * _client_new(client_t *client, va_list args) {
  clientpool_t *pool = va_arg(args, clientpool_t *);
  socket_t     *socket;
  data_t       *ret = NULL;

  socket = socket_open(pool -> server);
  if (socket -> error) {
    ret = data_copy(socket -> error);
    socket_free(socket);
    return ret;
  }
  client -> socket = socket;
  client -> pool = pool;
  if (!(ret = _client_handshake(client))) {
    ret = (data_t *) client;
  }
  return ret;
}

char * _client_tostring(client_t *client) {
  char *buf;

  asprintf(&buf, "Obelix Client for '%s'", uri_tostring(client -> mountpoint -> remote));
  return buf;
}

void _client_free(client_t *client) {
  if (client) {
    socket_free(client -> socket);
  }
}

data_t * _client_resolve(client_t *client, char *name) {
  if (!strcmp(name, "mountpoint")) {
    return (data_t *) mountpoint_copy(client -> mountpoint);
  } else if (!strcmp(name, "socket")) {
    return (data_t *) socket_copy(client -> socket);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

data_t * _client_handshake(client_t *client) {
  data_t  *ret;
  array_t *params;

  ret = _client_expect(client, OBLSERVER_CODE_WELCOME, 2, String, String);
  if (!data_is_exception(ret)) {
    params = data_unwrap(ret);
    data_free(ret);
    debug(obelix, "Connected to server %s %s on %s",
      data_tostring(data_array_get(params, 0)),
      data_tostring(data_array_get(params, 1)),
      uri_tostring(client -> pool -> server));
    if (client -> pool -> version) {
      client -> pool -> version = strdup(data_tostring(data_array_get(params, 1)));
    }
    ret = NULL;
  }
  if (!ret) {
    ret = _client_expect(client, OBLSERVER_CODE_READY, 0);
    if (!data_is_exception(ret)) {
      data_free(ret);
      ret = NULL;
    }
  }
  if (ret) {
    debug(obelix, "Handshake with server failed: %s", data_tostring(ret));
  }
  return ret;
}

data_t * _client_read_data(client_t *client, int len) {
  char    *data;
  data_t  *ret;
  int      r;

  data = stralloc(len + 1);
  debug(obelix, "Reading %d bytes of data", len);
  if ((r = socket_read(client -> socket, data, len)) == len) {
    data[len] = 0;
    strrtrim(data);
    ret = data_decode(data);
    if (data_is_exception(ret)) {
      data_as_exception(ret) -> handled = TRUE;
    }
    socket_readline(client -> socket); // FIXME error handling
  } else if (client -> socket -> error) {
    ret = data_copy(client -> socket -> error);
  } else {
    ret = data_exception(ErrorProtocol,
      "Protocol error reading data. Expected %d bytes, but could only read %d",
      len, r);
  }
  free(data);
  debug(obelix, "Returns '%s'", data_tostring(ret));
  return ret;
}

data_t * _client_expect(client_t *client, int expected, int numparams, ...) {
  va_list  types;
  char    *tag = label_for_code(server_codes, expected);
  char    *reply = NULL;
  array_t *line = NULL;
  data_t  *ret = NULL;
  int      code;
  array_t *params;
  int      ix;
  int      type;
  data_t  *param;

  debug(obelix, "Expecting code '%s' with %d parameters", tag, numparams);
  reply = socket_readline(client -> socket);
  debug(obelix, "Server sent '%s'", reply);
  if (!reply) {
    ret = (client -> socket -> error)
      ? data_copy(client -> socket -> error)
      : data_exception(ErrorIOError, "Could not read server response");
  }

  if (!ret) {
    line = array_split(reply, " ");
    if (array_size(line) != numparams + 2) {
      ret = data_exception(ErrorProtocol,
        "Protocol error reading data. Expected response line with %d parameters but got %s",
        numparams, reply);
    }
  }

  if (!ret) {
    line = array_split(reply, " ");
    code = code_for_label(server_codes, (char *) array_get(line, 1));
    if (code != expected) {
      ret = data_exception(ErrorProtocol,
        "Protocol error reading data. Expected %s tag but got %s",
        tag, (char *) array_get(line, 1));
    }
  }
  // if (!ret && strcmp((char *) array_get(line, 1), tag)) {
  //   ret = data_exception(ErrorProtocol,
  //     "Protocol error reading data. Expected %s tag but got %s",
  //     tag, (char *) array_get(line, 1));
  // }
  if (!ret && (numparams > 0)) {
    params = data_array_create(numparams);
    va_start(types, numparams);
    for (ix = 0; ix < numparams; ix++) {
      type = va_arg(types, int);
      param = data_parse(type, (char *) array_get(line, ix+2));
      if (!param) {
        ret = data_exception(ErrorProtocol,
          "Protocol error reading data. Expected parameter of type '%s' but got %s",
          typename(typedescr_get(type)), (char *) array_get(line, ix+2));
        break;
      } else {
        array_push(params, param);
      }
    }
    if (!ret) {
      ret = data_wrap(params);
    }
  }
  free(reply);
  array_free(line);
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t * client_run(client_t *client, char *cmd, array_t *args, dict_t *kwargs) {
  array_t *params = NULL;
  int      len;
  data_t  *ret = NULL;
  data_t  *data;

  /* FIXME pass parameters as data. Reuse DATA protocol? */
  (void) args;
  (void) kwargs;

  debug(cs, "Running %s(%s, %s) on %s using socket %d",
        cmd,
        (args) ? array_tostring(args) : "[]",
        (kwargs) ? dict_tostring(kwargs) : "{}",
        uri_tostring(client -> pool -> server),
        client -> socket -> fh);
  if (socket_printf(client -> socket, OBLSERVER_CMD_RUN " ${0;s}\n", cmd) <= 0) {
    ret = (client -> socket -> error)
      ? data_copy(client -> socket -> error)
      : data_exception(ErrorIOError, "Could not send forward command to server");
  }

  if (!ret) {
    data = _client_expect(client, OBLSERVER_CODE_DATA, 1, Int);
    if (data_is_exception(data)) {
      ret = data;
    } else {
      params = data_unwrap(data);
      debug(obelix, "Received parameters from server: %s", array_tostring(params));
      data_free(data);
    }
  }

  if (!ret) {
    len = data_intval(data_array_get(params, 0));
    ret = _client_read_data(client, len);
  }
  if (!data_is_exception(ret) || data_as_exception(ret) -> handled) {
    if ((data = _client_expect(client, OBLSERVER_CODE_READY, 0))) {
      data_free(ret);
      ret = data;
    } else {
      data_free(data);
    }
  }
  array_free(params);
  debug(obelix, "Returns '%s'", data_tostring(ret));
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t * client_create(clientpool_t *pool) {
  _client_init();
  return data_create(Client, pool);
}
