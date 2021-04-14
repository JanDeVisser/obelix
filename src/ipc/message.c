/*
 * /obelix/src/ipc/servermessage.c - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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
#include <json.h>

extern void              servermessage_init(void);

static servermessage_t * _servermessage_new(servermessage_t *, va_list);
static data_t *          _servermessage_parse(char *);
static void              _servermessage_free(servermessage_t *);
static char *            _servermessage_tostring(servermessage_t *);
static data_t *          _servermessage_resolve(servermessage_t *, char *);

static vtable_t _vtable_ServerMessage[] = {
  { .id = FunctionNew,          .fnc = (void_t) _servermessage_new },
  { .id = FunctionParse,        .fnc = (void_t) _servermessage_parse },
  { .id = FunctionFree,         .fnc = (void_t) _servermessage_free },
  { .id = FunctionResolve,      .fnc = (void_t) _servermessage_resolve },
  { .id = FunctionAllocString,  .fnc = (void_t) _servermessage_tostring },
  { .id = FunctionNone,         .fnc = NULL }
};

int ServerMessage = -1;
int ErrorProtocol = -1;


code_label_t message_codes[] = {
  { .code = OBLSERVER_CODE_WELCOME,        .label = OBLSERVER_TAG_WELCOME },
  { .code = OBLSERVER_CODE_READY,          .label = OBLSERVER_TAG_READY },
  { .code = OBLSERVER_CODE_DATA,           .label = OBLSERVER_TAG_DATA },
  { .code = OBLSERVER_CODE_ERROR_RUNTIME,  .label = OBLSERVER_TAG_ERROR_RUNTIME },
  { .code = OBLSERVER_CODE_ERROR_SYNTAX,   .label = OBLSERVER_TAG_ERROR_SYNTAX },
  { .code = OBLSERVER_CODE_ERROR_PROTOCOL, .label = OBLSERVER_TAG_ERROR_PROTOCOL },
  { .code = OBLSERVER_CODE_ERROR_INTERNAL, .label = OBLSERVER_TAG_ERROR_INTERNAL },
  { .code = OBLSERVER_CODE_COOKIE,         .label = OBLSERVER_TAG_COOKIE },
  { .code = OBLSERVER_CODE_BYE,            .label = OBLSERVER_TAG_BYE },
  { .code = 0,                             .label = NULL }
};

/* ------------------------------------------------------------------------ */

void servermessage_init(void) {
  if (ServerMessage < 1) {
    typedescr_register(ServerMessage, servermessage_t);
    exception_register(ErrorProtocol);
  }
}

/* ------------------------------------------------------------------------ */

servermessage_t * _servermessage_new(servermessage_t *msg, va_list args) {
  msg -> args = datalist_create(NULL);
  msg -> code = va_arg(args, int);
  msg -> tag = label_for_code(message_codes, msg -> code);
  if (!msg -> tag) {
    msg -> code = 0;
  }
  msg -> args = datalist_create(NULL);
  msg -> payload = NULL;
  msg -> encoded = NULL;
  msg -> payload_size = 0;
  return msg;
}

data_t * _servermessage_parse(char *str) {
  servermessage_t *msg;
  array_t         *words;
  int              sz;
  int              numargs;
  long             l;
  data_t          *ret = NULL;
  int              ix;

  words = array_split(str, " ");
  if (!words || ((sz = array_size(words)) < 2)) {
    ret = data_exception(ErrorProtocol,
        "Expected numeric code and matching tag parsing IPC message, but got '%s'",
        str);
  }
  if (!ret && strtoint(str_array_get(words, 0), &l)) {
    ret = data_exception(ErrorProtocol,
        "Expected numeric code parsing IPC message, but got '%s'",
        str_array_get(words, 0));
  }
  msg = servermessage_create(0, 0);
  msg -> code = (int) l;
  msg -> tag = label_for_code(message_codes, (int) l);
  if (!ret && strcmp(msg -> tag, str_array_get(words, 1))) {
    ret = data_exception(ErrorProtocol,
        "IPC message tag '%s' does not match code '%d'",
        str_array_get(words, 1), l);
  }
  if (!ret) {
    if (sz > 2) {
      if ((sz >= 4) &&
          !strcmp(str_array_get(words, sz - 2), "--") &&
          !strtoint(str_array_get(words, sz - 1), &l)) {
        numargs = array_size(words) - 2;
        msg->payload_size = (int) l;
      } else {
        numargs = sz;
      }
      for (ix = 2; ix < numargs; ix++) {
        if (!strtoint(str_array_get(words, ix), &l)) {
          datalist_push(msg->args, int_to_data(l));
        } else {
          datalist_push(msg->args, str_to_data(str_array_get(words, ix)));
        }
      }
    }
  }
  if (ret) {
    servermessage_free(msg);
  }
  return (ret) ? ret : (data_t *) msg;
}

void _servermessage_free(servermessage_t *msg) {
  if (msg) {
    /* Note: msg -> tag points to entry in message_codes so is not freed. */
    datalist_free(msg -> args);
    data_free(msg -> payload);
    free(msg -> encoded);
  }
}

char * _servermessage_tostring(servermessage_t *msg) {
  str_t *str = str_printf("%d %s", msg -> code, msg -> tag);
  int    ix;

  for (ix = 0; ix < datalist_size(msg -> args); ix++) {
    str_append_char(str, ' ');
    str_append_chars(str, data_tostring(datalist_get(msg -> args, ix)));
  }
  if (msg -> payload) {
    str_append_chars(str, " -- ");
    str_append_chars(str, data_tostring(int_to_data(msg -> payload_size)));
  }
  return str_reassign(str);
}

data_t * _servermessage_resolve(servermessage_t *msg, char *name) {
  if (!strcmp(name, "code")) {
    return int_to_data(msg -> code);
  } else if (!strcmp(name, "tag")) {
    return (msg -> tag) ? str_to_data(msg -> tag) : data_null();
  } else if (!strcmp(name, "args")) {
    return data_copy((data_t *) msg -> args);
  } else {
    return NULL;
  }
}

servermessage_t * servermessage_create(int code, int numargs, ...) {
  servermessage_t *ret;
  va_list          args;
  int              ix;
  char            *arg;
  long             l;

  ipc_init();
  ret = data_as_servermessage(data_create(ServerMessage, code));
  if (numargs > 0) {
    va_start(args, numargs);
    for (ix = 0; ix < numargs; ix++) {
      arg = va_arg(args, char *);
      if (!strtoint(arg, &l)) {
        datalist_push(ret->args, int_to_data(l));
      } else {
        datalist_push(ret->args, str_to_data(arg));
      }
    }
  }
  return ret;
}

data_t * servermessage_vmatch(servermessage_t *msg, int expected, int numargs, va_list types) {
  data_t  *ret = NULL;
  int      ix;
  int      type;

  if (msg -> code != expected) {
    ret = data_exception(ErrorProtocol,
        "Expected IPC message with code %d and tag '%s' but got %s",
        expected, label_for_code(message_codes, expected), data_tostring(msg));
  }
  if (!ret && datalist_size(msg -> args) != numargs) {
    ret = data_exception(ErrorProtocol,
        "Expected IPC message with tag '%s' and %d arguments but got %d arguments",
        msg -> tag, numargs, datalist_size(msg -> args));
  }
  if (!ret && (numargs > 0)) {
    for (ix = 0; ix < numargs; ix++) {
      type = va_arg(types, int);
      if (type != data_type(datalist_get(msg -> args, ix))) {
        ret = data_exception(ErrorProtocol,
                             "Expected IPC message with tag '%s' and argument %d of type '%s' but got %s",
                             msg -> tag, ix,
                             type_name(typedescr_get(type)),
                             data_tostring(datalist_get(msg -> args, ix)));
        break;
      }
    }
  }
  return ret;
}

data_t * servermessage_match(servermessage_t *msg, int expected, int numargs, ...) {
  va_list  types;
  data_t  *ret;

  va_start(types, numargs);
  ret = servermessage_vmatch(msg, expected, numargs, types);
  va_end(types);
  return ret;
}

data_t * servermessage_match_payload(servermessage_t *msg, int type) {
  if (!msg -> payload) {
    return data_exception(ErrorProtocol,
                          "Expected IPC message with tag '%s' and payload of type '%s' but there is no payload",
                          msg->tag, type_name(typedescr_get(type)));
  } else if (!data_hastype(msg -> payload, type)) {
    return data_exception(ErrorProtocol,
                          "Expected IPC message with tag '%s' and payload of type '%s' but the payload has type '%s'",
                          msg->tag, type_name(typedescr_get(type)), data_typename(msg->payload));
  } else {
    return NULL;
  }
}

servermessage_t * servermessage_push(servermessage_t *msg, char *arg) {
  datalist_push(msg -> args, str_to_data(arg));
  return msg;
}

servermessage_t * servermessage_push_int(servermessage_t *msg, int arg) {
  datalist_push(msg -> args, int_to_data(arg));
  return msg;
}

servermessage_t * servermessage_set_payload(servermessage_t *msg, data_t *payload) {
  data_free(msg -> payload);
  msg -> payload = NULL;
  free(msg -> encoded);
  msg -> encoded = NULL;
  if (payload) {
    msg->payload = data_copy(payload);
    msg->encoded = json_encode(payload);
    msg->payload_size = (int) strlen(msg->encoded) + 2;
  }
  return msg;
}