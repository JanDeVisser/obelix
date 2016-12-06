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
#ifndef OBLNET_IMPEXP
  #define OBLNET_IMPEXP	__DLL_IMPORT__
#endif /* OBLNET_IMPEXP */

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

OBLNET_IMPEXP socket_t *    socket_create(char *, int);
OBLNET_IMPEXP socket_t *    socket_create_byservice(char *, char *);
OBLNET_IMPEXP socket_t *    serversocket_create(int);
OBLNET_IMPEXP socket_t *    serversocket_create_byservice(char *);
OBLNET_IMPEXP int           socket_close(socket_t *);
OBLNET_IMPEXP unsigned int  socket_hash(socket_t *);
OBLNET_IMPEXP int           socket_cmp(socket_t *, socket_t *);
OBLNET_IMPEXP int           socket_listen(socket_t *, service_t, void *);
OBLNET_IMPEXP int           socket_listen_detach(socket_t *, service_t, void *);
OBLNET_IMPEXP socket_t *    socket_interrupt(socket_t *);
OBLNET_IMPEXP socket_t *    socket_nonblock(socket_t *);
OBLNET_IMPEXP int           socket_read(socket_t *, void *, int);
OBLNET_IMPEXP int           socket_write(socket_t *, void *, int);

OBLNET_IMPEXP void *        connection_listener_service(connection_t *);

#define data_is_socket(d)  ((d) && (data_hastype((d), Socket)))
#define data_as_socket(d)  ((socket_t *) (data_is_socket((data_t *) (d)) ? (d) : NULL))
#define socket_free(o)     (data_free((data_t *) (o)))
#define socket_tostring(o) (data_tostring((data_t *) (o)))
#define socket_copy(o)     ((socket_t *) data_copy((data_t *) (o)))

#define socket_set_errno(s)          (((stream_t *) (s)) -> _errno = errno)
#define socket_clear_errno(s)        (((stream_t *) (s)) -> _errno = 0)
#define socket_errno(s)              (((stream_t *) (s)) -> _errno)
#define socket_error(s)              (stream_error((stream_t *) (s)))
#define socket_getchar(s)            (stream_getchar((stream_t *) (s)))
#define socket_readline(s)           (stream_readline((stream_t *) (s)))
#define socket_print(s, f, a, kw)    (stream_print((stream_t *) (s), (f), (a), (kw)))
#define socket_vprintf(s, f, args)   (stream_printf((stream_t *) (s), (f), args))
#define socket_printf(s, ...)        (stream_printf((stream_t *) (s), __VA_ARGS__))

OBLNET_IMPEXP int Socket;

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

OBLNET_IMPEXP uri_t * uri_create(char *);
OBLNET_IMPEXP int     uri_path_absolute(uri_t *);
OBLNET_IMPEXP char *  uri_path(uri_t *);

OBLNET_IMPEXP int URI;

#define data_is_uri(d)     ((d) && data_hastype((d), URI))
#define data_as_uri(d)     (data_is_uri((d)) ? ((uri_t *) (d)) : NULL)
#define uri_copy(u)        ((uri_t *) data_copy((data_t *) (u)))
#define uri_free(u)        (data_free((data_t *) (u)))
#define uri_tostring(u)    (data_tostring((data_t *) (u)))

#ifdef	__cplusplus
}
#endif

#endif /* __NET_H__ */
