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

#include <time.h>

#include <net.h>
#include <array.h>
#include <exception.h>
#include <loader.h>
#include <name.h>

#include "obelix.h"

static inline void   _oblclient_init(void);

static data_t *      _oblclient_new(oblclient_t *, va_list);
static void          _oblclient_free(oblclient_t *);
static char *        _oblclient_tostring(oblclient_t *);
static data_t *      _oblclient_resolve(oblclient_t *, char *);

static data_t *      _oblclient_handshake(oblclient_t *);
static data_t *      _oblclient_expect(oblclient_t *, int, int, int *);
static data_t *      _oblclient_read_data(oblclient_t *, int);

static vtable_t _vtable_Client[] = {
  { .id = FunctionNew,         .fnc = (void_t) _oblclient_new },
  { .id = FunctionFree,        .fnc = (void_t) _oblclient_free },
  { .id = FunctionResolve,     .fnc = (void_t) _oblclient_resolve },
  { .id = FunctionAllocString, .fnc = (void_t) _oblclient_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

int Client = -1;

/* ------------------------------------------------------------------------ */

void _oblclient_init(void) {
  if (Client < 0) {
    typedescr_register(Client, oblclient_t);
  }
}

/* -- S E R V E R  T Y P E  F U N C T I O N S  ---------------------------- */

data_t * _oblclient_new(oblclient_t *client, va_list args) {
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
  client -> pool = clientpool_copy(pool);
  if ((ret = _oblclient_handshake(client))) {
    clientpool_free(client -> pool);
    socket_free(socket);
  } else {
    ret = (data_t *) client;
  }
  return ret;
}

char * _oblclient_tostring(oblclient_t *client) {
  char *buf;

  asprintf(&buf, "Obelix Client for '%s'", uri_tostring(client -> pool -> server));
  return buf;
}

void _oblclient_free(oblclient_t *client) {
  if (client) {
    socket_free(client -> socket);
    clientpool_free(client -> pool);
  }
}

data_t * _oblclient_resolve(oblclient_t *client, char *name) {
  if (!strcmp(name, "pool")) {
    return (data_t *) clientpool_copy(client -> pool);
  } else if (!strcmp(name, "socket")) {
    return (data_t *) socket_copy(client -> socket);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

data_t * _oblclient_handshake(oblclient_t *client) {
  data_t  *ret;
  array_t *params;

  ret = _oblclient_expect(client, OBLSERVER_CODE_WELCOME, 2, (int[]) { String, String });
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
  return ret;
}

data_t * _oblclient_read_data(oblclient_t *client, int len) {
  char    *data;
  data_t  *ret;
  int      r;

  data = stralloc(len + 1);
  if ((r = socket_read(client -> socket, data, len)) == len) {
    data[len] = 0;
    strrtrim(data);
    ret = data_decode(data);
    if (data_is_exception(ret)) {
      data_as_exception(ret) -> handled = TRUE;
    }
  } else if (client -> socket -> error) {
    ret = data_copy(client -> socket -> error);
  } else {
    ret = data_exception(ErrorProtocol,
      "Protocol error reading data. Expected %d bytes, but could only read %d",
      len, r);
  }
  free(data);
  return ret;
}

data_t * _oblclient_expect(oblclient_t *client, int expected, int numparams, int *types) {
  char    *tag = label_for_code(server_codes, expected);
  char    *reply = NULL;
  array_t *line = NULL;
  data_t  *ret = NULL;
  int      code;
  array_t *params;
  int      ix;
  data_t  *param;

  reply = socket_readline(client -> socket);
  if (!reply) {
    ret = (client -> socket -> error)
      ? data_copy(client -> socket -> error)
      : data_exception(ErrorIOError, "Could not read server response");
  }

  if (!ret) {
    if (array_size(line) != numparams + 2) {
      ret = data_exception(ErrorProtocol,
        "Protocol error reading data. Expected response line with %d parameters but got %s",
        numparams, reply);
    }
  }

  if (!ret) {
    line = array_split(reply, " ");
    code = code_for_label(server_codes, data_tostring(data_array_get(line, 0)));
    if (code != expected) {
      ret = data_exception(ErrorProtocol,
        "Protocol error reading data. Expected %s tag but got %s",
        tag, data_tostring(data_array_get(line, 0)));
    }
  }
  if (!ret && strcmp(data_tostring(data_array_get(line, 1)), tag)) {
    ret = data_exception(ErrorProtocol,
      "Protocol error reading data. Expected %s tag but got %s",
      tag, data_tostring(data_array_get(line, 1)));
  }
  if (!ret && (numparams > 0)) {
    params = data_array_create(numparams);
    for (ix = 0; ix < numparams; ix++) {
      param = data_parse(types[ix], data_tostring(data_array_get(line, ix+2)));
      if (!param) {
        ret = data_exception(ErrorProtocol,
          "Protocol error reading data. Expected parameter of type '%s' but got %s",
          typename(typedescr_get(types[ix])), data_tostring(data_array_get(line, ix+2)));
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

data_t * oblclient_run(oblclient_t *client, char *cmd, array_t *args, dict_t *kwargs) {
  array_t *params = NULL;
  int      len;
  data_t  *ret = NULL;
  data_t  *data;

  /* FIXME pass parameters as data. Reuse DATA protocol? */
  (void) args;
  (void) kwargs;

  debug(obelix, "Forwarding %s(%s, %s) to %s on socket %d",
        cmd,
        (args) ? array_tostring(args) : "[]",
        (kwargs) ? dict_tostring(kwargs) : "{}",
        uri_tostring(client -> pool -> server),
        client -> socket -> fh);
  if (socket_printf(client -> socket, OBLSERVER_CMD_RUN " %s\n", cmd) <= 0) {
    ret = (client -> socket -> error)
      ? data_copy(client -> socket -> error)
      : data_exception(ErrorIOError, "Could not send forward command to server");
  }

  if (!ret) {
    data = _oblclient_expect(client, OBLSERVER_CODE_DATA, 1, &(int) { Int });
    if (data_is_exception(data)) {
      ret = data;
    } else {
      params = data_unwrap(data);
      data_free(data);
    }
  }

  if (!ret) {
    len = data_intval(data_array_get(params, 0));
    ret = _oblclient_read_data(client, len);
  }
  if (!data_is_exception(ret) || data_as_exception(ret) -> handled) {
    if ((data = _oblclient_expect(client, OBLSERVER_CODE_READY, 0, NULL))) {
      data_free(ret);
      ret = data;
    } else {
      data_free(data);
    }
  }
  array_free(params);
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t * oblclient_create(clientpool_t *pool) {
  _oblclient_init();
  return data_create(Client, pool);
}
