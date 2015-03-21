/*
 * /
#include "data.h"obelix/src/stdlib/net.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <data.h>
#include <socket.h>

static void          _net_init(void) __attribute__((constructor));

/* -------------------------------------------------------------------------*/

int    Socket;

static data_t *      _socket_new(data_t *, va_list);
static data_t *      _socket_copy(data_t *, data_t *);
static int           _socket_cmp(data_t *, data_t *);
static char *        _socket_tostring(data_t *);
static unsigned int  _socket_hash(data_t *);
static data_t *      _socket_resolve(data_t *, char *);

static data_t *      _socket_close(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_list[] = {
  { .id = FunctionNew,      .fnc = (void_t) _socket_new },
  { .id = FunctionCopy,     .fnc = (void_t) _socket_copy },
  { .id = FunctionCmp,      .fnc = (void_t) _socket_cmp },
  { .id = FunctionFree,     .fnc = (void_t) socket_free },
  { .id = FunctionToString, .fnc = (void_t) _socket_tostring },
  { .id = FunctionHash,     .fnc = (void_t) _socket_hash },
  { .id = FunctionResolve,  .fnc = (void_t) _socket_resolve },
  { .id = FunctionNone,     .fnc = NULL }
};

static typedescr_t _typedescr_socket =   {
  .type      = -1,
  .type_name = "socket",
  .vtable    = _vtable_list,
};

static methoddescr_t _methoddescr_socket[] = {
  { .type = -1,     .name = "close", .method = _socket_close, .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,    .method = NULL,          .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

#define data_is_socket(d) ((d) && (data_type((d)) == Socket))
#define data_socketval(d) ((socket_t *) ((data_is_socket((d)) ? (d) -> ptrval : NULL)))


/* -------------------------------------------------------------------------*/

void _net_init(void) {
  int ix;

  Socket = typedescr_register(&_typedescr_socket);
  if (file_debug) {
    debug("File type initialized");
  }
  for (ix = 0; _methoddescr_socket[ix].type != NoType; ix++) {
    if (_methoddescr_socket[ix].type == -1) {
      _methoddescr_socket[ix].type = Socket;
    }
  }
  typedescr_register_methods(_methoddescr_socket);
}

/* -------------------------------------------------------------------------*/

data_t * _function_connect(char *name, array_t *params, dict_t *kwargs) {
  data_t  *host;
  data_t  *service;
  data_t  *socket;

  assert(params && (array_size(params) >= 2));
  host = (data_t *) array_get(params, 0);
  service = (data_t *) array_get(params, 1);
  
  socket = data_create(Socket, data_tostring(host), data_tostring(service));
  return socket;
}

/* -------------------------------------------------------------------------*/

data_t * _socket_new(data_t *target, va_list args) {
  socket_t *socket;
  char     *host;
  char     *service;
  
  host = va_arg(args, char *);
  service = va_arg(args, char *);
  socket = socket_create_byservice(host, service);
  target -> ptrval = socket;
  return target;
}

data_t * _socket_copy(data_t *target, data_t *src) {
  target -> ptrval = socket_copy(data_socketval(src));
}

int _socket_cmp(data_t *d1, data_t *d2) {
  return socket_cmp(data_socketval(d1), data_socketval(d2));
}

char * _socket_tostring(data_t *data) {
  return socket_tostring(data_socketval(data));
}

unsigned int _socket_hash(data_t *data) {
  return socket_hash(data_socketval(data));
}

data_t * _socket_resolve(data_t *data, char *attr) {
  data_t      *ret;
  typedescr_t *filetype;
  file_t      *f;
  
  if (!strcmp(attr, "fd")) {
    filetype = typedescr_get_byname("file");
    assert(filetype);
    ret = data_create(filetype -> type, NULL);
    f = (file_t *) ret -> ptrval;
    file_free(f);
    ret -> ptrval = file_copy(data_socketval(data) -> sockfile);
    return ret;
  } else {
    return NULL;
  }
}

data_t * _socket_close(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  return data_create(Int, socket_close(data_socketval(self)));
}

