/*
 * /obelix/src/ipc/server.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include "libipc.h"

#include <exception.h>

typedef struct _server_cmd_handler {
  int        code;
  data_t * (*handler)(server_t *, servermessage_t *);
} server_cmd_handler_t;

extern void       server_init(void);

static server_t * _server_new(server_t *, va_list);
static void       _server_free(server_t *);
static char *     _server_tostring(server_t *);
static data_t *   _server_resolve(server_t *, char *);

static data_t *   _server_welcome(server_t *, servermessage_t *);
static data_t *   _server_call(server_t *, servermessage_t *);
static data_t *   _server_quit(server_t *, servermessage_t *);

static vtable_t _vtable_Server[] = {
  { .id = FunctionNew,          .fnc = (void_t) _server_new },
  { .id = FunctionFree,         .fnc = (void_t) _server_free },
  { .id = FunctionResolve,      .fnc = (void_t) _server_resolve },
  { .id = FunctionStaticString, .fnc = (void_t) _server_tostring },
  { .id = FunctionNone,         .fnc = NULL }
};

int Server = -1;

static server_cmd_handler_t _cmd_handlers[] = {
  { .code = OBLSERVER_CODE_HELLO,    .handler = _server_welcome },
  { .code = OBLSERVER_CODE_CALL,     .handler = _server_call },
  { .code = OBLSERVER_CODE_QUIT,     .handler = _server_quit },
  { .code = 0,                       .handler = NULL }
};

static servermessage_t * _bye = NULL;
static servermessage_t * _hello = NULL;
static servermessage_t * _ready = NULL;

/* ------------------------------------------------------------------------ */

void server_init(void) {
  if (Server < 1) {
    typedescr_register(Server, server_t);
    _bye = servermessage_create(OBLSERVER_CODE_BYE, 0);
    _bye -> _d.free_me = Constant;
    _hello = servermessage_create(OBLSERVER_CODE_HELLO, 0);
    _hello -> _d.free_me = Constant;
    _ready = servermessage_create(OBLSERVER_CODE_READY, 0);
    _ready -> _d.free_me = Constant;
  }
}

/* -- S E R V E R  T Y P E  F U N C T I O N S  ---------------------------- */

static inline data_t * _server_engine_call(server_t *server, vtable_id_t func,
                                           servermessage_t *msg) {
  va_list        args;
  data_t        *ret;
  data_t *      (*f)(data_t *, server_t *, servermessage_t *);

  f = (data_t * (*)(data_t *, server_t *, servermessage_t *)) data_get_function(server -> engine, func);
  if (!f) {
    return data_exception(ErrorInternalError,
        "No function with code '%d' defined in engine '%s'",
        func, data_tostring(server -> engine));
  }
  ret = f(server -> engine, server, msg);
  return ret;
}

server_t * _server_new(server_t *server, va_list args) {
  server -> engine = data_copy(va_arg(args, data_t *));
  server -> stream = stream_copy(va_arg(args, stream_t *));
  return server;
}

char * _server_tostring(server_t * _unused_ server) {
  return "Obelix IPC Server";
}

void _server_free(server_t *server) {
  if (server) {
    _server_engine_call(server, FunctionUnregisterServer, NULL);
    data_free(server -> engine);
  }
}

data_t * _server_resolve(server_t *server, char *name) {
  if (!strcmp(name, "engine")) {
    return data_copy(server -> engine);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

data_t * _server_call(server_t *server, servermessage_t *msg) {
  data_t *retval;
  data_t *ret = NULL;

  retval = _server_engine_call(server, FunctionRemoteCall, msg);
  ret = protocol_return_result(server->stream, retval);
  data_free(retval);
  return ret;
}

data_t * _server_welcome(server_t *server, servermessage_t *hello) {
  servermessage_t *welcome;
  data_t          *ret;
  dictionary_t    *welcome_data;

  ret = servermessage_match(hello, OBLSERVER_CODE_HELLO, 1, String);
  if (ret) {
    return ret;
  }

  ret = _server_engine_call(server, FunctionRegisterServer, hello);
  if (!data_is_exception(ret)) {
    assert(!ret || data_is_dictionary(ret));
    welcome = servermessage_create(OBLSERVER_CODE_WELCOME, 0);
    welcome_data = (ret) ? data_as_dictionary(ret) : dictionary_create(NULL);
    dictionary_set(welcome_data, "engine",
        str_to_data(data_tostring(server -> engine)));
    dictionary_set(welcome_data, "host",
        str_to_data("localhost")); // FIXME
    servermessage_set_payload(welcome, (data_t *) welcome_data);
    ret = protocol_send_message(server -> stream, welcome);
    servermessage_free(welcome);
    dictionary_free(welcome_data);
  }
  return ret;
}

data_t * _server_quit(server_t *server, servermessage_t *ignore) {
  (void) ignore;
  /* Ignore return value of protocol_send_message since we're quitting anyway */
  protocol_send_message(server -> stream, _bye);
  return data_exception(ErrorQuit, "Quit");
}

void * _server_connection_handler(connection_t *connection) {
  data_t   *engine = data_as_data(connection -> context);
  server_t *server = server_create(engine, data_as_stream(connection -> client));

  server_free(server_run(server));
  return connection;
}

/* ------------------------------------------------------------------------ */

server_t * server_create(data_t *engine, stream_t *stream) {
  ipc_init();
  debug(ipc, "Creating IPC server for engine '%s' using stream '%s'",
      data_tostring(engine), stream_tostring(stream));
  return data_as_server(data_create(Server, engine, stream));
}

server_t * server_run(server_t *server) {
  servermessage_t      *msg = _hello;
  server_cmd_handler_t *handler;
  data_t               *ret = 0;
  exception_t          *ex;

  while (msg) {
    debug(ipc, "Message: '%s'", servermessage_tostring(msg));
    for (handler = _cmd_handlers; handler -> code; handler++) {
      if (msg -> code == handler -> code) {
        ret = (handler -> handler)(server, msg);
      }
    }
    servermessage_free(msg);
    if (data_is_exception(ret)) {
      ex = data_as_exception(ret);
      if (ex -> code == ErrorProtocol) {
        ret = protocol_send_data(server -> stream,
            OBLSERVER_CODE_ERROR_PROTOCOL, (data_t *) ex);
        exception_free(ex);
      }
    }
    if (!ret) {
      ret = protocol_send_message(server -> stream, _ready);
    }
    for (msg = NULL; !msg; ) {
      ret = protocol_read_message(server -> stream);
      if (data_is_servermessage(ret)) {
        msg = data_as_servermessage(ret);
        ret = NULL;
      } else if (data_is_exception(ret)) {
        ex = data_as_exception(ret);
        if (ex -> code == ErrorProtocol) {
          ret = protocol_send_data(server -> stream,
              OBLSERVER_CODE_ERROR_PROTOCOL, (data_t *) ex);
          exception_free(ex);
        }
      }
    }
  }
  return server;
}

/* ------------------------------------------------------------------------ */

int server_start(data_t *engine, int port) {
  socket_t *server;

  if (port <= 0) {
    port = OBELIX_DEFAULT_PORT;
  }
  server = serversocket_create(port);
  debug(ipc, "Establishing IPC server on port %d", port);
  socket_listen(server,
                _server_connection_handler,
                engine);
  return 0;
}
