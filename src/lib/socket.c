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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <data.h>
#include <exception.h>
#include <socket.h>
#include <thread.h>

typedef int         (*socket_fnc_t)(int, const struct sockaddr *, socklen_t);

static void         _socket_init(void) __attribute__((constructor));
static socket_t *   _socket_create(int, char *, char *);
static socket_t *   _socket_open(char *, char *, socket_fnc_t);
static char *       _socket_allocstring(socket_t *);
static void         _socket_free(socket_t *);
static int          _socket_listen(socket_t *, service_t, void *, int);
static int          _socket_accept_loop(socket_t *);
static int          _socket_accept(socket_t *);
static void *       _socket_connection_handler(connection_t *);
static socket_t *   _socket_setopt(socket_t *, int);

static data_t *     _socket_resolve(socket_t *, char *);

static data_t *     _socket_close(data_t *, char *, array_t *, dict_t *);
static data_t *     _socket_listen_mth(data_t *, char *, array_t *, dict_t *);
static data_t *     _socket_interrupt(data_t *, char *, array_t *, dict_t *);


/*
 * TODO:
 *   - Add bits from struct addrinfo into socket_t
 *   - Allow UDP connections (and unix streams?)
 */

static vtable_t _vtable_socket[] = {
  { .id = FunctionFactory,     .fnc = (void_t) data_embedded },
  { .id = FunctionCmp,         .fnc = (void_t) socket_cmp },
  { .id = FunctionFree,        .fnc = (void_t) _socket_free },
  { .id = FunctionAllocString, .fnc = (void_t) _socket_allocstring },
  { .id = FunctionHash,        .fnc = (void_t) socket_hash },
  { .id = FunctionResolve,     .fnc = (void_t) _socket_resolve },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methoddescr_socket[] = {
  { .type = -1,     .name = "close",     .method = _socket_close,      .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = -1,     .name = "listen",    .method = _socket_listen_mth, .argtypes = { Callable, NoType, NoType }, .minargs = 1, .varargs = 0 },
  { .type = -1,     .name = "interrupt", .method = _socket_interrupt,  .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
  { .type = NoType, .name = NULL,        .method = NULL,               .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 },
};

int Socket;
int socket_debug;

/* ------------------------------------------------------------------------ */

void _net_init(void) {
  logging_register_category("socket", &socket_debug);
  Socket = typedescr_create_and_register(Socket, "socket",
                                         _vtable_socket, _methoddescr_socket);
}

/* -- S O C K E T  S T A T I C  F U N C T I O N S  ------------------------ */

socket_t * _socket_create(int fh, char *host, char *service) {
  socket_t *ret = data_new(Socket, socket_t);

  // TODO: Add other bits from struct addrinfo into socket_t
  ret -> sockfile = file_create(fh);
  ret -> host = (host) ? strdup(host) : NULL;
  ret -> service = strdup(service);
  ret -> fh = fh;
  return ret;
}

socket_t * _socket_open(char *host, char *service, socket_fnc_t fnc) {
  struct addrinfo  hints;
  struct addrinfo *result;
  struct addrinfo *rp;
  int              sfd;
  int              s;
  socket_t        *ret;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream (TCP) socket */
  hints.ai_flags = (host) ? 0 : AI_PASSIVE;
  hints.ai_protocol = 0;           /* Any protocol */

  s = getaddrinfo(host, service, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return NULL;
  }

  /*
   * getaddrinfo() returns a list of address structures.
   * Try each address until we successfully connect(2). If socket(2) (or
   * connect(2) fails, we close the socket and try the next address.
   */
  for (rp = result; rp != NULL; rp = rp -> ai_next) {
    sfd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
    if (sfd == -1) {
      continue;
    }
    if (fnc(sfd, rp -> ai_addr, rp -> ai_addrlen) != -1) {
      break;
    }
    close(sfd);
  }

  ret = (rp) ? _socket_create(sfd, host, service) : NULL;
  freeaddrinfo(result);
  if (!ret) { /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
  }
  return ret;
}

void _socket_free(socket_t *socket) {
  if (socket) {
    socket_close(socket);
    file_free(socket -> sockfile);
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
  if (!strcmp(attr, "fd")) {
    return data_copy((data_t *) s -> sockfile);
  } else if (!strcmp(attr, "host") && s -> host) {
    return data_create(String, s -> host);
  } else if (!strcmp(attr, "service")) {
    return data_create(String, s -> service);
  } else {
    return NULL;
  }
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
  struct sockaddr    client;
  int                client_fd;
  socklen_t          sz = sizeof(struct sockaddr);
  pthread_t          thr_id;
  socket_t          *client_socket;
  connection_t      *connection;
  char               hoststr[80];
  char               portstr[32];

  client_fd = TEMP_FAILURE_RETRY(accept(socket -> fh, (struct sockaddr *) &client, &sz));
  if (client_fd > 0) {
    if (TEMP_FAILURE_RETRY(getnameinfo(&client, sz, hoststr, 80, portstr, 32, 0))) {
      error("Could not retrieve client name");
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
      error("Could not create connection service thread");
      return -1;
    }
  } else {
    error("Error accepting");
    return -1;
  }
}

int _socket_accept_loop(socket_t *socket) {
  fd_set         set;
  struct timeval timeout;
  int            err;

  while (socket -> service_handler) {
    FD_ZERO (&set);
    FD_SET (socket -> fh, &set);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* select returns 0 if timeout, 1 if input available, -1 if error. */
    err = TEMP_FAILURE_RETRY(select(FD_SETSIZE, &set, NULL, NULL, &timeout));
    if (err < 0) {
      error("Error in connection listener: select()");
      return -1;
    } else if (err > 0) {
      TEMP_FAILURE_RETRY(_socket_accept(socket));
    }
  }
  socket_free(socket);
  return 0;
}

int _socket_listen(socket_t *socket, service_t service, void *context, int async) {
  thread_t  *thread;
  if (TEMP_FAILURE_RETRY(listen(socket -> fh, 5))) {
    error("Error setting up listener");
    return -1;
  } else {
    socket -> service_handler = service;
    socket -> context = context;
    socket_nonblock(socket);
    if (!async) {
      _socket_accept_loop(socket);
    } else {
      socket -> thread = thread_new("Socket Listener Thread",
		                    (threadproc_t) _socket_accept_loop,
                                    socket);
      if (!socket -> thread) {
        error("Could not create listener thread");
        return -1;
      }
    }
    return 0;
  }
}

socket_t * _socket_setopt(socket_t *socket, int opt) {
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
}

/* -- S O C K E T  P U B L I C  A P I ------------------------------------- */

socket_t * socket_create(char *host, int port) {
  char service[32];

  snprintf(service, 32, "%d", port);
  return socket_create_byservice(host, service);
}

socket_t * socket_create_byservice(char *host, char *service) {
  socket_t        *ret;

  ret = _socket_open(host, service, connect);
  return ret;
}

socket_t * serversocket_create(int port) {
  char service[32];

  snprintf(service, 32, "%d", port);
  return serversocket_create_byservice(service);
}

socket_t * serversocket_create_byservice(char *service) {
  return _socket_open(NULL, service, bind);
}

int socket_close(socket_t *socket) {
  socket_interrupt(socket);
  return file_close(socket -> sockfile);
}

int socket_cmp(socket_t *s1, socket_t *s2) {
  return file_cmp(s1 -> sockfile, s2 -> sockfile);
}

unsigned int socket_hash(socket_t *socket) {
  return hashblend(strhash(socket -> host), strhash(socket -> service));
}

int socket_listen(socket_t *socket, service_t service, void *context) {
  _socket_listen(socket, service, context, 0);
}

int socket_listen_detach(socket_t *socket, service_t service, void *context) {
  _socket_listen(socket, service, context, 1);
}

socket_t * socket_interrupt(socket_t *socket) {
  if (socket -> thread) {
    thread_interrupt(socket -> thread);
  }
  socket -> service_handler = NULL;
  return socket;
}

socket_t * socket_nonblock(socket_t *socket) {
  return _socket_setopt(socket, O_NONBLOCK);
}

/* ------------------------------------------------------------------------ */

data_t * _socket_close(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  socket_t *s = (socket_t *) self;

  return data_create(Int, socket_close(s));
}

data_t * _socket_listen_mth(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  socket_t *socket = (socket_t *) self;

  (void) name;
  (void) kwargs;
  if (socket -> host) {
    return data_exception(ErrorIOError, "Cannot listen - socket is not a server socket");
  } else {
    if (socket_listen(socket, connection_listener_service, data_array_get(args, 0))) {
      return data_exception_from_errno();
    } else {
      return data_create(Bool, 1);
    }
  }
}

data_t * _socket_interrupt(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  socket_t *socket = (socket_t *) self;

  (void) name;
  (void) kwargs;
  if (socket -> host) {
    return data_exception(ErrorIOError, "Cannot interrupt - socket is not a server socket");
  } else {
    socket_interrupt(socket);
    return data_create(Bool, 1);
  }
}

/* ------------------------------------------------------------------------ */

void * connection_listener_service(connection_t *connection) {
  data_t   *server = connection -> context;
  socket_t *client = (socket_t* ) connection -> client;
  data_t   *ret;
  array_t  *p;

  p = data_array_create(1);
  array_push(p, data_copy((data_t *) client));
  ret = data_call(server, p, NULL);
  array_free(p);
  return ret;
}

