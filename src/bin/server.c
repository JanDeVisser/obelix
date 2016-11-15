/*
 * /obelix/src/bin/server.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#include <time.h>

#include <socket.h>
#include <array.h>
#include <loader.h>
#include <name.h>

#include "obelix.h"
#include "server.h"

typedef struct _server_cmd_handler {
  char  *cmd;
  int  (*handler)(oblserver_t *, char *);
} server_cmd_handler_t;

static inline void   _oblserver_init(void);

static void          _oblserver_free(oblserver_t *);
static char *        _oblserver_tostring(oblserver_t *);
static data_t *      _oblserver_resolve(oblserver_t *, char *);

static oblserver_t * _oblserver_return_error(oblserver_t *, char *, data_t *);
static oblserver_t * _oblserver_return_result(oblserver_t *, data_t *);

static int           _oblserver_welcome(oblserver_t *, char *);
static int           _oblserver_attach(oblserver_t *, char *);
static int           _oblserver_path(oblserver_t *, char *);
static int           _oblserver_eval(oblserver_t *, char *);
static int           _oblserver_run(oblserver_t *, char *);
static int           _oblserver_detach(oblserver_t *, char *);
static int           _oblserver_quit(oblserver_t *, char *);

static vtable_t _vtable_Server[] = {
  { .id = FunctionFree,         .fnc = (void_t) _oblserver_free },
  { .id = FunctionResolve,      .fnc = (void_t) _oblserver_resolve },
  { .id = FunctionStaticString, .fnc = (void_t) _oblserver_tostring },
  { .id = FunctionNone,         .fnc = NULL }
};

int Server = -1;

static server_cmd_handler_t _cmd_handlers[] = {
  { .cmd = OBLSERVER_CMD_HELLO,  .handler = _oblserver_welcome },
  { .cmd = OBLSERVER_CMD_ATTACH, .handler = _oblserver_attach },
  { .cmd = OBLSERVER_CMD_PATH,   .handler = _oblserver_path },
  { .cmd = OBLSERVER_CMD_EVAL,   .handler = _oblserver_eval },
  { .cmd = OBLSERVER_CMD_RUN,    .handler = _oblserver_run },
  { .cmd = OBLSERVER_CMD_DETACH, .handler = _oblserver_detach },
  { .cmd = OBLSERVER_CMD_QUIT,   .handler = _oblserver_quit },
  { .cmd = NULL,                 .handler = NULL }
};

static code_label_t _server_codes[] = {
  { .code = OBLSERVER_CODE_WELCOME,        .label = OBLSERVER_WELCOME },
  { .code = OBLSERVER_CODE_READY,          .label = OBLSERVER_READY },
  { .code = OBLSERVER_CODE_DATA,           .label = OBLSERVER_DATA },
  { .code = OBLSERVER_CODE_ERROR_RUNTIME,  .label = OBLSERVER_ERROR_RUNTIME },
  { .code = OBLSERVER_CODE_ERROR_SYNTAX,   .label = OBLSERVER_ERROR_SYNTAX },
  { .code = OBLSERVER_CODE_ERROR_PROTOCOL, .label = OBLSERVER_ERROR_PROTOCOL },
  { .code = OBLSERVER_CODE_ERROR_INTERNAL, .label = OBLSERVER_ERROR_INTERNAL },
  { .code = OBLSERVER_CODE_COOKIE,         .label = OBLSERVER_COOKIE },
  { .code = OBLSERVER_CODE_BYE,            .label = OBLSERVER_BYE },
  { .code = 0,                             .label = NULL }
};

/* ------------------------------------------------------------------------ */

void _oblserver_init(void) {
  if (Server < 0) {
    typedescr_register(Server, oblserver_t);
  }
}

/* -- S E R V E R  T Y P E  F U N C T I O N S  ---------------------------- */

char * _oblserver_tostring(oblserver_t *server) {
  return "Obelix Server";
}

void _oblserver_free(oblserver_t *server) {
  if (server) {
    obelix_decommission_loader(server -> obelix, server -> loader);
    obelix_free(server -> obelix);
  }
}

data_t * _oblserver_resolve(oblserver_t *server, char *name) {
  if (!strcmp(name, "loader")) {
    return (data_t *) scriptloader_copy(server -> loader);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

oblserver_t * _oblserver_return_error(oblserver_t *server, char *code, 
                                      data_t *param) {
  debug(obelix, "Returning error %s %s", code, param);
  stream_printf(server -> stream, "${0;s} ${1}", code, param);
  return server;
}

oblserver_t * _oblserver_return_result(oblserver_t *server, data_t *result) {
  exception_t *ex;
  array_t     *arr;
  char        *str;
  oblserver_t *ret;

  debug(obelix, "Returning %s [%s]", data_tostring(result), data_typename(result));
  if (data_is_exception(result)) {
    ex = data_as_exception(result);
    switch (ex -> code) {
      case ErrorExit:
        result = data_copy(ex -> throwable);
        exception_free(ex);
        break;

      case ErrorSyntax:
        ret = _oblserver_return_error(server, OBLSERVER_ERROR_SYNTAX, result);
        data_free(result);
        result = NULL;
        break;

      default:
        ret = _oblserver_return_error(server, OBLSERVER_ERROR_RUNTIME, result); 
        data_free(result);
        result = NULL;
        break;

    }
  }
  if (result) {
    str = data_tostring(result);
    stream_printf(server -> stream, OBLSERVER_DATA " ${0;d} ${1;s}", 
                  strlen(str) + 1, data_typename(result));
    stream_write(server -> stream, str, strlen(str));
    stream_printf(server -> stream, "");
    data_free(result);
    ret = server;
  }
  return ret;
}

oblserver_t * _oblserver_create_loader(oblserver_t *server) {
  if (!server -> loader) {
    if (!(server -> loader = obelix_create_loader(server -> obelix))) {
      return NULL;
    }
  }
  server -> loader -> lastused = time(NULL);
  return server;
}

/* ------------------------------------------------------------------------ */

int _oblserver_path(oblserver_t *server, char *path) {
  array_t *loadpath = array_split(path, ":");
  
  _oblserver_create_loader(server);
  debug(obelix, "Adding '%s' to load path", path);
  scriptloader_extend_loadpath(server -> loader, loadpath);
  array_free(loadpath);
  return 0;
}

int _oblserver_run(oblserver_t *server, char *cmd) {
  array_t     *line;
  char        *script;
  name_t      *name;
  array_t     *obl_argv;
  data_t      *ret;
  
  _oblserver_create_loader(server);
  debug(obelix, "Executing '%s'", cmd);
  line = array_split(cmd, " ");
  if (array_size(line)) {
    script = str_array_get(line, 0);
    name = obelix_build_name(script);
    obl_argv = array_slice(line, 1, array_size(line));
    
    ret = scriptloader_run(server -> loader, name, obl_argv, NULL);
    _oblserver_return_result(server, ret);
  }
  array_free(line);
  return 0;
}

int _oblserver_eval(oblserver_t *server, char *script) {
  data_t  *ret;
  data_t  *dscript;
  
  _oblserver_create_loader(server);
  debug(obelix, "Evaluating '%s'", script);
  ret = scriptloader_eval(server -> loader, dscript = (data_t *) str_wrap(script));
  _oblserver_return_result(server, ret);
  data_free(ret);
  data_free(dscript);
  return 0;
}

int _oblserver_welcome(oblserver_t *server, char *ignore) {
  (void) ignore;
  stream_printf(server -> stream, OBLSERVER_WELCOME " " OBELIX_NAME " " OBELIX_VERSION);
  return 0;
}

int _oblserver_quit(oblserver_t *server, char *ignore) {
  (void) ignore;
  
  return - OBLSERVER_CODE_BYE;
}

int _oblserver_detach(oblserver_t *server, char *ignore) {
  data_t *dcookie;
  
  (void) ignore;
  
  if (server -> loader) {
    _oblserver_return_error(server, OBLSERVER_COOKIE, 
                            dcookie = (data_t *) str_wrap(server -> loader -> cookie));
    data_free(dcookie);
    
    /* 
     * Preserve the loader. First free it, which decrements the counter. There
     * is at least one other copy, in obelix -> loaders. Then set the reference 
     * in the server to NULL, so it will not be decommissioned when the server 
     * is deleted.
     */
    scriptloader_free(server -> loader);
    server -> loader = NULL;
  }
  return - OBLSERVER_CODE_BYE;
}

int _oblserver_attach(oblserver_t *server, char *cookie) {
  scriptloader_t *loader = obelix_get_loader(server -> obelix, cookie);

  if (loader) {
    scriptloader_free(server -> loader);
    server -> loader = loader;
    loader -> lastused = time(NULL);
  } else {
    _oblserver_return_error(server, OBLSERVER_ERROR_PROTOCOL, 
                            (data_t *) str_wrap(cookie));
  }
  return 0;
}

void * _oblserver_connection_handler(connection_t *connection) {
  obelix_t    *obl = (obelix_t *) connection -> context;
  oblserver_t *server = oblserver_create(obl, data_as_stream(connection -> client));
  
  oblserver_free(oblserver_run(server));
  return connection;
}

/* ------------------------------------------------------------------------ */

oblserver_t * oblserver_create(obelix_t *obelix, stream_t *stream) {
  oblserver_t *ret;

  _oblserver_init();
  debug(obelix, "Creating server using stream '%s'", stream_tostring(stream));
  ret = data_new(Server, oblserver_t);
  ret -> obelix = obelix_copy(obelix);
  ret -> stream = stream_copy(stream);
  ret -> loader = NULL;
  return ret;
}

oblserver_t * oblserver_run(oblserver_t *server) {
  char                 *cmd = OBLSERVER_CMD_HELLO;
  server_cmd_handler_t *handler;
  int                   ret = 0;
  int                   len;
  int                   code;
  char                 *msg = OBLSERVER_READY;

  while (cmd && (ret >= 0)) {
    debug(obelix, "Command: '%s'", cmd);
    msg = NULL;
    for (handler = _cmd_handlers; handler -> cmd; handler++) {
      len = strlen(handler -> cmd);
      if (!strncmp(cmd, handler -> cmd, len)) {
        ret = (handler -> handler)(server, cmd + len + 1);
      }
    }
    if (ret == -1) {
      ret = - OBLSERVER_CODE_ERROR_INTERNAL;
    } else if (!ret) {
      ret = OBLSERVER_CODE_READY;
    }
    code = (ret < 0) ? -ret : ret;
    msg = label_for_code(_server_codes, code);
    if (msg) {
      stream_printf(server -> stream, msg);
    }
    cmd = stream_readline(server -> stream);
  };
  return server;
}

/* ------------------------------------------------------------------------ */

int oblserver_start(obelix_t *obelix) {
  socket_t *server;
  
  server = serversocket_create(obelix -> server);
  socket_listen(server, 
                _oblserver_connection_handler, 
                obelix);
  return 0;
}

