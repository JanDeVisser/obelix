/*
 * /obelix/inclure/uri.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __NET_H__
#define __NET_H__

#include <oblconfig.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <data.h>
#include <dictionary.h>
#include <file.h>
#include <name.h>
#include <str.h>
#include <thread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_TYPE_SOCKET
typedef int SOCKET;
#endif

/* ------------------------------------------------------------------------ */

typedef struct _uri {
  data_t   _d;
  data_t  *error;
  char    *scheme;
  char    *user;
  char    *password;
  char    *host;
  int      port;
  name_t  *path;
  dict_t  *query;
  char    *fragment;
} uri_t;

extern uri_t * uri_create(char *);
extern int     uri_path_absolute(uri_t *);
extern char *  uri_path(uri_t *);

extern int URI;

type_skel(uri, URI, uri_t);

/* ------------------------------------------------------------------------ */

typedef struct _connection {
  struct _socket *server;
  struct _socket *client;
  data_t         *context;
  thread_t       *thread;
} connection_t;

typedef void * (*service_t)(connection_t *);

typedef struct _socket {
  stream_t   _stream;
  SOCKET     fh;
  int        af;
  int        socktype;
  char      *host;
  char      *service;
  service_t  service_handler;
  thread_t  *thread;
  void      *context;
} socket_t;

extern socket_t *           socket_create(char *, int);
extern socket_t *           socket_create_byservice(char *, char *);
extern socket_t *           socket_open(uri_t *);
extern socket_t *           serversocket_create(int);
extern socket_t *           serversocket_create_byservice(char *);
extern int                  socket_close(socket_t *);
extern unsigned int         socket_hash(socket_t *);
extern int                  socket_cmp(socket_t *, socket_t *);
extern int                  socket_listen(socket_t *, service_t, void *);
extern int                  socket_listen_detach(socket_t *, service_t, void *);
extern socket_t *           socket_interrupt(socket_t *);
extern socket_t *           socket_nonblock(socket_t *);
extern int                  socket_read(socket_t *, void *, int);
extern int                  socket_write(socket_t *, void *, int);
extern socket_t *           socket_clear_error(socket_t *);
extern socket_t *           socket_set_errormsg(socket_t *, char *, ...);
extern socket_t *           socket_set_error(socket_t *, data_t *);
extern socket_t *           socket_set_errno(socket_t *, char *);

extern void *               connection_listener_service(connection_t *);

extern int Socket;
extern int ErrorSocket;

type_skel(socket, Socket, socket_t);

#define socket_getchar(s)          (stream_getchar((stream_t *) (s)))
#define socket_readline(s)         (stream_readline((stream_t *) (s)))
#define socket_print(s, f, a, kw)  (stream_print((stream_t *) (s), (f), (a), (kw)))
#define socket_vprintf(s, f, args) (stream_printf((stream_t *) (s), (f), args))
#define socket_printf(s, ...)      (stream_printf((stream_t *) (s), __VA_ARGS__))

static inline int socket_errno(socket_t *socket) {
  return ((stream_t *) socket) -> _errno;
}

static inline data_t * socket_error(socket_t *socket) {
  return ((stream_t *) socket) -> error;
}

#ifdef	__cplusplus
}
#endif

#endif /* __NET_H__ */
