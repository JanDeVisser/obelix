/*
 * /obelix/src/lib/socket.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include "libnet.h"
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif /* HAVE_WS2TCPIP_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

#ifndef HAVE_WINSOCK2_H
#define closesocket(s) close(s)
#endif

#ifndef TEMP_FAILURE_RETRY
#ifndef _MSC_VER
#define TEMP_FAILURE_RETRY(expression) \
    ({ \
        long int _result; \
        do _result = (long int) (expression); \
        while (_result == -1L && errno == EINTR); \
        _result; \
    })
#else
#define TEMP_FAILURE_RETRY(expression)  (expression)
#endif /* _MSC_VER */
#endif /* !TEMP_FAILURE_RETRY */

#ifndef HAVE_ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif

typedef int         (*socket_fnc_t)(SOCKET, struct sockaddr *, socklen_t);

static socket_t *   _socket_create(SOCKET, char *, char *);
static socket_t *   _socket_error(char *, char *, char *, ...);
static socket_t *   _socket_open(char *, char *, socket_fnc_t);
static socket_t *   _socket_new(socket_t *, va_list);
static char *       _socket_allocstring(socket_t *);
static void         _socket_free(socket_t *);
static int          _socket_listen(socket_t *, service_t, void *, int);
static int          _socket_accept_loop(socket_t *);
static int          _socket_accept(socket_t *);
static void *       _socket_connection_handler(connection_t *);
static socket_t *   _socket_setopt(socket_t *, int);

static data_t *     _socket_resolve(socket_t *, char *);
static data_t *     _socket_leave(socket_t *, data_t *);

static data_t *     _socket_close(data_t *, char *, arguments_t *);
static data_t *     _socket_listen_mth(data_t *, char *, arguments_t *);
static data_t *     _socket_interrupt(data_t *, char *, arguments_t *);

/*
 * TODO:
 *   - Add bits from struct addrinfo into socket_t
 *   - Allow UDP connections (and unix streams?)
 */

_unused_ static vtable_t _vtable_Socket[] = {
  { .id = FunctionNew,         .fnc = (void_t) _socket_new },
  { .id = FunctionCmp,         .fnc = (void_t) socket_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _socket_free },
  { .id = FunctionAllocString, .fnc = (void_t) _socket_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) socket_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _socket_resolve },
  { .id = FunctionLeave,       .fnc = (void_t) _socket_leave },
  { .id = FunctionNone,        .fnc = NULL }
};

_unused_ static methoddescr_t _methods_Socket[] = {
  { .type = -1,     .name = "close",     .method = _socket_close,      .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "listen",    .method = _socket_listen_mth, .argtypes = { Callable, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "interrupt", .method = _socket_interrupt,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,        .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int Socket = -1;
int socket_debug;
int ErrorSocket = -1;

/* ------------------------------------------------------------------------ */

void socket_init(void) {
#ifdef HAVE_WINSOCK2_H
  WSADATA wsadata;
  int     result;
#endif

  if (Socket < 1) {
    logging_register_category("socket", &socket_debug);
    file_init();
    typedescr_register_with_methods(Socket, socket_t);
    typedescr_assign_inheritance(Socket, Stream);
    exception_register(ErrorSocket);

#ifdef HAVE_WINSOCK2_H
    result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result) {
      fatal("Could not initialize Windows Sockets");
    }
#endif
  }
}

/* -- S O C K E T  S T A T I C  F U N C T I O N S  ------------------------ */

socket_t * _socket_new(socket_t *socket, va_list args) {
  char   *host = va_arg(args, char *);
  char   *service = va_arg(args, char *);

  // TODO: Add other bits from struct addrinfo into socket_t
  socket -> host = (host) ? strdup(host) : NULL;
  socket -> service = strdup(service);
  socket -> fh = -1;
  stream_init((stream_t *) socket,
      (read_t) socket_read,
      (write_t) socket_write);
  return socket;
}

void _socket_free(socket_t *socket) {
  if (socket) {
    socket_close(socket);
    free(socket -> host);
    free(socket -> service);
    thread_free(socket -> thread);
  }
}

char * _socket_allocstring(socket_t *socket) {
  char *buf;

  asprintf(&buf, "%s:%s", socket -> host, socket -> service);
  return buf;
}

data_t * _socket_resolve(socket_t *s, char *attr) {
  if (!strcmp(attr, "host") && s -> host) {
    return str_to_data(s -> host);
  } else if (!strcmp(attr, "service")) {
    return str_to_data(s -> service);
  } else if (!strcmp(attr, "error")) {
    return data_copy(s -> _stream.error);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

socket_t * _socket_create(SOCKET fh, char *host, char *service) {
  socket_t *ret;

  socket_init();
  ret = (socket_t *) data_create(Socket, host, service);
  ret -> fh = fh;
  return ret;
}

socket_t * _socket_error(char *host, char *service, char *fmt, ...) {
  va_list      args;
  socket_t    *ret;

  socket_init();
  va_start(args, fmt);
  ret = socket_set_error((socket_t *) data_create(Socket, host, service),
            (data_t *) exception_vcreate(ErrorSocket, fmt, args));
  va_end(args);
  return ret;
}

socket_t * _socket_open(char *host, char *service, socket_fnc_t fnc) {
  struct addrinfo  hints;
  struct addrinfo *result = NULL;
  struct addrinfo *rp;
  SOCKET           sfd;
  int              s;
  socket_t        *ret = NULL;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream (TCP) socket */
  hints.ai_flags = (host) ? 0 : AI_PASSIVE;
  hints.ai_protocol = 0;           /* Any protocol */

  s = getaddrinfo(host, service, &hints, &result);
  if (s != 0) {
    return _socket_error(host, service, "Error in getaddrinfo('%s', '%s'): %s",
      host, service, gai_strerror(s));
  }

  /*
   * getaddrinfo() returns a list of address structures.
   * Try each address until we successfully connect(2). If socket(2) (or
   * connect(2) fails, we close the socket and try the next address.
   */
  for (rp = result; rp != NULL; rp = rp -> ai_next) {
    /*
     * FIXME We're ignoring IPV6 addresses now...
     */
    if (rp -> ai_family != AF_INET) {
      continue;
    }
    sfd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
    if (sfd == -1) {
      continue;
    }

#ifdef HAVE_SO_REUSEADDR
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
      error("setsockopt(SO_REUSEADDR) failed");
      continue;
    }
#endif
#ifdef HAVE_SO_NOSIGPIPE
    if (setsockopt(sfd, SOL_SOCKET, SO_NOSIGPIPE, &(int){ 1 }, sizeof(int)) < 0) {
      error("setsockopt(SO_NOSIGPIPE) failed");
      continue;
    }
#endif

    if (fnc(sfd, rp -> ai_addr, rp -> ai_addrlen) != -1) {
      ret = _socket_create(sfd, host, service);
      ret -> af = rp -> ai_family;
      ret -> socktype = rp -> ai_socktype;
      break;
    }
    closesocket(sfd);
  }
  freeaddrinfo(result);
  if (!ret) { /* No address succeeded */
    ret = _socket_error(host, service,
      "Could not connect '%s':'%s' on any address", host, service);
  }
  return ret;
}

data_t *_socket_leave(socket_t * socket, data_t *param) {
  int     retval;
  data_t *ret;

  debug(socket, "socket '%s'.leave('%s')", socket_tostring(socket), data_tostring(param));
  retval = socket_close(socket);
  if (retval < 0) {
    ret = data_exception(ErrorIOError, "socket_close() returned %d", int_to_data(retval));
  } else {
    ret = data_null();
  }
  return ret;
}

void * _socket_connection_handler(connection_t *connection) {
  void *ret;

  ret = connection -> server -> service_handler(connection);
  socket_free(connection -> client);
  socket_free(connection -> server);
  thread_free(connection -> thread);
  return ret;
}

int _socket_accept(socket_t *socket) {
  struct sockaddr_storage  client;
  socklen_t                sz = sizeof(struct sockaddr_storage);
  int                      client_fd;
  connection_t            *connection;
  char                     hoststr[80];
  char                     portstr[32];

  client_fd = (int) TEMP_FAILURE_RETRY(accept(socket -> fh, (struct sockaddr *) &client, &sz));
  if (client_fd > 0) {
    if (TEMP_FAILURE_RETRY(getnameinfo((struct sockaddr *) &client, sz,
        hoststr, 80, portstr, 32, 0))) {
      socket_set_errno(socket, "getnameinfo()");
      return -1;
    }
    connection = NEW(connection_t);
    connection -> server = socket_copy(socket);
    connection -> client = _socket_create(client_fd, hoststr, portstr);
    connection -> context = socket -> context;
    connection -> thread = thread_new("Socket Connection Handler",
                                      (threadproc_t) _socket_connection_handler,
                                      connection);
    if (!connection -> thread) {
      socket_set_errormsg(socket, "Could not create connection service thread");
      return -1;
    }
  } else {
    socket_set_errno(socket, "accept()");
    return -1;
  }
  return 0;
}

int _socket_accept_loop(socket_t *socket) {
  fd_set         set;
  struct timeval timeout;
  int            err;

  while (socket -> service_handler) {
    FD_ZERO(&set);
    FD_SET(socket -> fh, &set);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* select returns 0 if timeout, 1 if input available, -1 if error. */
    err = TEMP_FAILURE_RETRY(select(FD_SETSIZE, &set, NULL, NULL, &timeout));
    if (err < 0) {
      socket_set_errno(socket, "select()");
      return -1;
    } else if (err > 0) {
      err = TEMP_FAILURE_RETRY(_socket_accept(socket));
      if (err) {
        break;
      }
    }
  }
  socket_free(socket);
  return 0;
}

int _socket_listen(socket_t *socket, service_t service, void *context, int async) {
  if (TEMP_FAILURE_RETRY(listen(socket->fh, 5))) {
    socket_set_errormsg(socket, "Error setting up listener");
    return -1;
  } else {
    socket->service_handler = service;
    socket->context = context;
    socket_nonblock(socket);
    if (!async) {
      _socket_accept_loop(socket);
    } else {
      socket->thread = thread_new("Socket Listener Thread",
          (threadproc_t) _socket_accept_loop, socket);
      if (!socket->thread) {
        socket_set_errormsg(socket, "Could not create listener thread");
        return -1;
      }
    }
    return 0;
  }
}

socket_t * _socket_setopt(socket_t *socket, int opt) {
#ifndef _WIN32
  int oldflags = fcntl(socket -> fh, F_GETFL, 0);

  if (oldflags == -1) {
    return NULL;
  }
  /* Set just the flag we want to set. */
  /* Turning off would be oldflags &= ~O_NONBLOCK */
  if (fcntl(socket -> fh, F_SETFL, oldflags | opt)) {
    return NULL;
  } else {
    return socket;
  }
#else /* _WIN32 */
  return socket;
#endif
}

/* -- S O C K E T  P U B L I C  A P I ------------------------------------- */

socket_t * socket_create(char *host, int port) {
  char service[32];

  snprintf(service, 32, "%d", port);
  return socket_create_byservice(host, service);
}

socket_t * socket_open(uri_t *uri) {
  if (uri -> port) {
    return socket_create(uri -> host, uri -> port);
  } else {
    return socket_create_byservice(uri -> host, uri -> scheme);
  }
}

socket_t * socket_create_byservice(char *host, char *service) {
  socket_t        *ret;

  ret = _socket_open(host, service, (socket_fnc_t) connect);
  return ret;
}

socket_t * serversocket_create(int port) {
  char service[32];

  snprintf(service, 32, "%d", port);
  return serversocket_create_byservice(service);
}

socket_t * serversocket_create_byservice(char *service) {
  return _socket_open(NULL, service, (socket_fnc_t) bind);
}

/* -- E R R O R  H A N D L I N G ------------------------------------------ */

socket_t * socket_clear_error(socket_t *s) {
  s -> _stream._errno = 0;
  data_free(s -> _stream.error);
  s -> _stream.error = NULL;
  return s;
}

socket_t * socket_set_errormsg(socket_t *socket, char *fmt, ...) {
  data_t  *error;
  va_list  args;

  va_start(args, fmt);
  error = (data_t *) exception_vcreate(ErrorSocket, fmt, args);
  va_end(args);
  return socket_set_error(socket, error);
}

socket_t * socket_set_errno(socket_t *socket, char *msg) {
  char     *error;

#ifdef HAVE_WINSOCK2_H
  s -> _errno = WSAGetLastError();
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR) &error, 0, NULL);
#else
  size_t len;

  socket -> _stream._errno = errno;
  len = strlen(strerror(socket_errno(socket)));
  error = stralloc(len + 1);
  error = strerror_r(socket_errno(socket), error, len + 1);
#endif
  socket_set_errormsg(socket, "%s failed: %s (%d)",
    msg, error, socket_errno(socket));
  return socket;
}

socket_t * socket_set_error(socket_t *socket, data_t *error) {
  socket_clear_error(socket);
  debug(socket, "Setting error on '%s': %s",
    socket_tostring(socket), data_tostring(error))
  socket -> _stream.error = error;
  return socket;
}

/* ------------------------------------------------------------------------ */

int socket_close(socket_t *socket) {
  int ret;

  socket_interrupt(socket);
  if ((ret = closesocket(socket -> fh))) {
    socket_set_errno(socket, "closesocket()");
  }
  return ret;
}

int socket_cmp(socket_t *s1, socket_t *s2) {
  return (s1 -> fh == s2 -> fh) ? 0 : 1;
}

unsigned int socket_hash(socket_t *socket) {
  return hashblend(strhash(socket -> host), strhash(socket -> service));
}

int socket_listen(socket_t *socket, service_t service, void *context) {
  return _socket_listen(socket, service, context, 0);
}

int socket_listen_detach(socket_t *socket, service_t service, void *context) {
  return _socket_listen(socket, service, context, 1);
}

socket_t * socket_interrupt(socket_t *socket) {
  if (socket -> thread) {
    thread_interrupt(socket -> thread);
  }
  socket -> service_handler = NULL;
  return socket;
}

socket_t * socket_nonblock(socket_t *socket) {
  socket_t *ret = NULL;

#ifndef _WIN32
  ret = _socket_setopt(socket, O_NONBLOCK);
#else /* _WIN32 */
  u_long nonblock = 1;
  ret = ioctlsocket(socket -> fh, FIONBIO, &nonblock) ? NULL : socket;
#endif /* _WIN32 */
  if (!ret) {
    socket_set_errno(socket, "nonblock()");
    return NULL;
  } else {
    return socket;
  }
}

int _socket_readblock(socket_t *socket, void *buf, int num) {
  int  ret = 0;
  int  num_remaining = num;
  int  total_numrecv = 0;
  int  numrecv;

  numrecv = TEMP_FAILURE_RETRY(recv(socket -> fh, buf, num_remaining, 0));
  if (numrecv > 0) {
    num_remaining -= numrecv;
    total_numrecv += numrecv;
    ret += numrecv;
    buf = (void *)(((char *) buf) + numrecv);
    debug(socket, "socket_read(%s, %d) = %d", data_tostring((data_t *) socket), num, total_numrecv);
  } else if ((numrecv == 0) ||
#ifdef HAVE_WINSOCK2_H
             (WSAGetLastError() == WSAEWOULDBLOCK)) {
#else
             (errno == EWOULDBLOCK)) {
#endif /* HAVE_WINSOCK2_H */
    debug(socket, "socket_read(%s, %d) Blocked", data_tostring((data_t *) socket), num);
  } else {
    socket_set_errno(socket, "socket_read()->recv()");
    ret = -1;
  }
  return ret;
}

int socket_read(socket_t *socket, void *buf, int num) {
  int            ret = 0;
  fd_set         set;
  struct timeval timeout;
  int            err;

  if (socket_debug) {
    memset(buf, 0, num);
  }
  debug(socket, "socket_read(%s, %d)", data_tostring((data_t *) socket), num);

  ret = _socket_readblock(socket, buf, num);
  if (!ret) {
    do {
      FD_ZERO(&set);
      FD_SET(socket -> fh, &set);
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      debug(socket, "socket_read(%s, %d) select()", data_tostring((data_t *) socket), num);
      err = TEMP_FAILURE_RETRY(select(1, &set, NULL, NULL, &timeout));
      if (err < 0) {
        socket_set_errno(socket, "socket_read()->select()");
        return -1;
      }
    } while (!err);
    debug(socket, "socket_read(%s, %d) _readblock()", data_tostring((data_t *) socket), num);
    ret = _socket_readblock(socket, buf, num);
  }
  return ret;
}

#ifdef HAVE_MSG_NOSIGNAL
#define SOCKET_SEND_FLAGS     MSG_NOSIGNAL
#else
#define SOCKET_SEND_FLAGS     0
#endif /* HAVE_MSG_NOSIGNAL */

int socket_write(socket_t *socket, void *buf, int num) {
  int ret = num;
  int numsend;

  do {
    numsend = TEMP_FAILURE_RETRY(send(socket -> fh, buf, num, SOCKET_SEND_FLAGS));
    if (numsend > 0) {
      num = num - numsend;
      buf = (void *) (((char *) buf) + numsend);
    } else {
      socket_set_errno(socket, "send()");
      ret = -1;
    }
  } while ((ret > 0) && (num > 0));
  return ret;
}

/* ------------------------------------------------------------------------ */

data_t * _socket_close(data_t *self, char *name, arguments_t *args) {
  socket_t *s = (socket_t *) self;

  return (socket_close(s)) ? data_copy(s -> _stream.error) : data_true();
}

data_t * _socket_listen_mth(data_t *self, char _unused_ *name, arguments_t *args) {
  socket_t *socket = (socket_t *) self;

  if (socket -> host) {
    return data_exception(ErrorSocket,
      "Cannot listen - socket '%s' is not a server socket", socket_tostring(socket));
  } else {
    if (socket_listen(socket, connection_listener_service, arguments_get_arg(args, 0))) {
      return data_copy(socket -> _stream.error);
    } else {
      return data_true();
    }
  }
}

data_t * _socket_interrupt(data_t *self, char _unused_ *name, arguments_t _unused_ *args) {
  socket_t *socket = (socket_t *) self;

  if (socket -> host) {
    return data_exception(ErrorSocket,
      "Socket '%s' cannot be interrupted because it is not a server socket",
      socket_tostring(socket));
  } else {
    socket_interrupt(socket);
    return data_true();
  }
}

/* ------------------------------------------------------------------------ */

void * connection_listener_service(connection_t *connection) {
  data_t      *server = connection -> context;
  socket_t    *client = connection -> client;
  data_t      *ret;
  arguments_t *args;

  args = arguments_create_args(1, client);
  ret = data_call(server, args);
  arguments_free(args);
  return ret;
}
