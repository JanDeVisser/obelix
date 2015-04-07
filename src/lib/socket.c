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

#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

#include <socket.h>

typedef int (*socket_fnc_t)(int, struct sockaddr *, socklen_t);

static socket_t *       _socket_create(int, char *, char *);
static socket_t *       _socket_open(char *, char *, socket_fnc_t);

/*
 * TODO:
 *   - Add bits from struct addrinfo into socket_t
 *   - Allow UDP connections (and unix streams?)
 */

/* -- S O C K E T  S T A T I C  F U N C T I O N S  ------------------------ */

socket_t * _socket_create(int fd, char *host, char *service) {
  socket_t *ret = NEW(socket_t);
  
  // TODO: Add other bits from struct addrinfo into socket_t
  ret -> sockfile = file_create(fd);
  ret -> host = (host) ? strdup(host) : NULL;
  ret -> service = strdup(service);
  ret -> str = NULL;
  ret -> refs = 1;
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
  hints.ai_flags = 0;
  hints.ai_protocol = 0;           /* Any protocol */

  s = getaddrinfo(NULL, service, &hints, &result);
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


/* -- S O C K E T  P U B L I C  A P I ------------------------------------- */

socket_t * socket_create(char *host, int port) {
  char service[32];
  
  snprintf(service, 32, "%d", port);
  return socket_create_byservice(host, service);
}

socket_t * socket_create_byservice(char *host, char *service) {
  struct addrinfo  hints;
  struct addrinfo *result;
  struct addrinfo *rp;
  int              sfd;
  int              s;
  socket_t        *ret;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream (TCP) socket */
  hints.ai_flags = 0;
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
    if (connect(sfd, rp -> ai_addr, rp -> ai_addrlen) != -1) {
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

socket_t * socket_copy(socket_t *socket) {
  socket -> refs++;
  return socket;
}

void socket_free(socket_t *socket) {
  if (socket) {
    socket -> refs--;
    if (socket -> refs <= 0) {
      file_close(socket -> sockfile);
      file_free(socket -> sockfile);
      free(socket -> str);
      free(socket -> host);
      free(socket -> service);
      free(socket);
    }
  }
}

int socket_close(socket_t *socket) {
  return file_close(socket -> sockfile);  
}

char * socket_tostring(socket_t *socket) {
  if (!socket) {
    return "socket:NULL";
  } else {
    if (!socket -> str) {
      asprintf(&socket -> str, "%s:%s", socket -> host, socket -> service);
    }
    return socket -> str;
  }
}

int socket_cmp(socket_t *s1, socket_t *s2) {
  return file_cmp(s1 -> sockfile, s2 -> sockfile);
}

unsigned int socket_hash(socket_t *socket) {
  return hashblend(strhash(socket -> host), strhash(socket -> service));
}

socket_t * serversocket_create_byservice(char *service) {
  struct addrinfo  hints;
  struct addrinfo *result;
  struct addrinfo *rp;
  int              sfd;
  int              s;
  socket_t        *ret;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream (TCP) socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0;           /* Any protocol */

  s = getaddrinfo(NULL, service, &hints, &result);
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
    if (bind(listen_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
    if (connect(sfd, rp -> ai_addr, rp -> ai_addrlen) != -1) {
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

int socket_listen(int port, service_t service, void *context) {
  int                listen_fd;
  int                client_fd;
  socket_t          *client_socket;
  socklen_t          sz;
  char               buffer[256];
  struct sockaddr_in server;
  struct sockaddr_in client;
  int                n;
  pthread_t          thr_id;
  char               portstr[32];
  char               hoststr[80];
  void              *service_param;
  
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    error("ERROR opening socket");
    return -1;
  }
  
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  if (bind(listen_fd, (struct sockaddr *) &server, sizeof(server)) < 0) {
    error("ERROR on binding");
    return -1;
  }
  
  listen(listen_fd, 5);
  sz = sizeof(struct sockaddr_in);
  
  while ((client_fd = accept(client_fd, (struct sockaddr *) &client, &sz))) {
    if (getnameinfo(&client, sz, hoststr, 80, portstr, 32, 0)) {
      error("Could not retrieve client name");
      continue;
    }
    client_socket = _socket_create(client_fd, hoststr, portstr);
    service_param = new_ptrarray(2);
    service_param[0] = context;
    service_param[1] = client_socket;
    if (pthread_create(&thr_id, NULL, service, service_param) < 0) {
      error("could not create thread");
      continue;
    }
    if (pthread_detach(thr_id)) {
      error("Could not detach thread");
    }
  }
  close(listen_fd);
  return -1;
}
