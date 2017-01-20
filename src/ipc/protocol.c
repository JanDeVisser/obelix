/*
 * /obelix/src/ipc/protocol.c - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#include "libipc.h"

#include <exception.h>
#include <json.h>

static data_t *    _protocol_read_payload(stream_t *, servermessage_t *);

/* ------------------------------------------------------------------------ */

data_t * _protocol_read_payload(stream_t *stream, servermessage_t *msg) {
  data_t  *reader;
  data_t  *ret = NULL;
  int      r;

  msg -> encoded = stralloc(msg -> payload_size + 1);
  debug(ipc, "Reading %d bytes of data", msg -> payload_size);
  r = stream_read(stream, msg -> encoded, msg -> payload_size);
  if (r == msg -> payload_size) {
    msg -> encoded[msg -> payload_size] = 0;
    strrtrim(msg -> encoded);
    reader = (data_t *) str_copy_chars(msg -> encoded);
    msg -> payload = json_decode(reader);
    data_free(reader);
    if (data_is_exception(msg -> payload)) {
      data_as_exception(msg -> payload) -> handled = TRUE;
    }
  } else if (stream -> error) {
    ret = data_copy(stream -> error);
  } else {
    ret = data_exception(ErrorProtocol,
        "Protocol error reading data. Expected %d bytes, but could only read %d",
        msg -> payload_size, r);
  }
  debug(ipc, "Returns '%s'", data_tostring(ret));
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t * protocol_write(stream_t *stream, char *buf, int len) {
  data_t *ret = NULL;

  if (stream_write(stream, buf, len) <= 0) {
    ret = (stream -> error)
          ? data_copy(stream -> error)
          : data_exception(ErrorIOError, "Could not write to IPC channel");
  }
  return ret;
}

data_t * protocol_vprintf(stream_t *stream, char *fmt, va_list args) {
  data_t  *ret = NULL;

  if (stream_vprintf(stream, fmt, args) <= 0) {
    ret = (stream -> error)
          ? data_copy(stream -> error)
          : data_exception(ErrorIOError, "Could not write to IPC channel");
  }
  return ret;
}

data_t * protocol_printf(stream_t *stream, char *fmt, ...) {
  data_t  *ret = NULL;
  va_list  args;

  va_start(args, fmt);
  ret = protocol_vprintf(stream, fmt, args);
  va_end(args);
  return ret;
}

data_t * protocol_newline(stream_t *stream) {
  return protocol_write(stream, "\r\n", 2);
}

data_t * protocol_readline(stream_t *stream) {
  char   *line;
  data_t *ret;

  line = stream_readline(stream);
  if (!line) {
    ret = (stream -> error)
          ? data_copy(stream -> error)
          : data_exception(ErrorIOError, "Could not read from IPC channel");
  } else {
    ret = str_to_data(line);
  }
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t * protocol_expect(stream_t *stream, int expected, int numargs, ...) {
  va_list          types;
  servermessage_t *msg = NULL;
  char            *tag = label_for_code(message_codes, expected);
  data_t          *ret = NULL;

  debug(ipc, "Expecting code '%s' with %d parameters", tag, numargs);
  ret = protocol_read_message(stream);
  if (data_is_servermessage(ret)) {
    msg = data_as_servermessage(ret);
    ret = NULL;
  }
  if (!ret && msg) {
    debug(ipc, "Server sent '%s'", servermessage_tostring(msg));
    va_start(types, numargs);
    ret = servermessage_vmatch(msg, expected, numargs, types);
    va_end(types);
  }
  return (ret) ? ret : (data_t *) msg;
}

data_t * protocol_send_handshake(stream_t *stream, mountpoint_t *mountpoint) {
  servermessage_t *msg;
  data_t *version = NULL;
  data_t *ret;
  servermessage_t *hello;

  hello = servermessage_create(OBLSERVER_CODE_HELLO,
      1, uri_path(mountpoint->remote));
  ret = protocol_send_message(stream, hello);
  servermessage_free(hello);
  if (!ret) {
    ret = protocol_expect(stream, OBLSERVER_CODE_WELCOME, 3, String, String, String);
  }
  if (data_is_servermessage(ret)) {
    msg = data_as_servermessage(ret);
    ret = NULL;
    debug(ipc, "Connected to server %s %s on %s",
        data_tostring(data_uncopy(datalist_get(msg -> args, 0))),
        data_tostring(data_uncopy(datalist_get(msg -> args, 1))),
        data_tostring(data_uncopy(datalist_get(msg -> args, 2))));
    version = datalist_get(msg -> args, 1);
  }
  if (!ret) {
    ret = protocol_expect(stream, OBLSERVER_CODE_READY, 0);
  }
  if (!data_is_servermessage(ret)) {
    error("Handshake with server failed: %s", data_tostring(ret));
  } else {
    data_free(ret);
    ret = version;
  }
  return ret;
}

data_t * protocol_send_data(stream_t *stream, int code, data_t *data) {
  servermessage_t *msg = servermessage_create(code, 0);
  data_t          *ret;

  error("Sending data with code %s", code);
  servermessage_set_payload(msg, data);
  ret = protocol_send_message(stream, msg);
  servermessage_free(msg);
  return ret;
}

data_t * protocol_return_result(stream_t *stream, data_t *result) {
  exception_t     *ex;
  data_t          *ret = NULL;
  servermessage_t *msg;
  int              code = OBLSERVER_CODE_DATA;

  debug(ipc, "Returning %s [%s]", data_tostring(result), data_typename(result));
  if (data_is_exception(result)) {
    ex = data_as_exception(result);
    switch (ex -> code) {
      case ErrorExit:
        result = data_copy(ex -> throwable);
        exception_free(ex);
        break;
      case ErrorSyntax:
        code = OBLSERVER_CODE_ERROR_SYNTAX;
        break;
      default:
        code = OBLSERVER_CODE_ERROR_RUNTIME;
        break;
    }
  }

  if (result) {
    msg = servermessage_create(code, 0);
    servermessage_set_payload(msg, result);
    ret = protocol_send_message(stream, msg);
    data_free(result);
  }
  return ret;
}

data_t * protocol_send_message(stream_t *stream, servermessage_t *msg) {
  data_t * ret;

  ret = protocol_printf(stream, servermessage_tostring(msg));
  if (!ret && msg->payload) {
    ret = protocol_write(stream, msg->encoded, msg->payload_size - 2);
    if (!ret) {
      ret = protocol_newline(stream);
    }
  }
  return ret;
}

data_t * protocol_read_message(stream_t *stream) {
  str_t           *line = NULL;
  data_t          *ret;
  servermessage_t *msg;

  ret = protocol_readline(stream);
  if (data_is_string(ret)) {
    line = (str_t *) ret;
    ret = NULL;
  }
  if (!ret) {
    ret = data_parse(ServerMessage, str_chars(line));
  }
  if (data_is_servermessage(ret)) {
    msg = data_as_servermessage(ret);
    if (msg -> payload_size > 0) {
      ret = _protocol_read_payload(stream, msg);
    }
  }
  return ret;
}

name_t * protocol_build_name(char *scriptname) {
  char   *buf;
  char   *ptr;
  name_t *name;

  buf = strdup(scriptname);
  while ((ptr = strchr(buf, '/'))) {
    *ptr = '.';
  }
  if ((ptr = strrchr(buf, '.'))) {
    if (!strcmp(ptr, ".obl")) {
      *ptr = 0;
    }
  }
  name = name_split(buf, ".");
  free(buf);
  return name;
}

