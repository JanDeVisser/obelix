/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>

#include "obelix.h"

static inline void    _clientpool_init(void);

static data_t *       _clientpool_new(clientpool_t *, va_list);
static void           _clientpool_free(clientpool_t *);
static char *         _clientpool_tostring(clientpool_t *);
static data_t *       _clientpool_resolve(clientpool_t *, char *);

static vtable_t _vtable_ClientPool[] = {
  { .id = FunctionNew,         .fnc = (void_t) _clientpool_new },
  { .id = FunctionFree,        .fnc = (void_t) _clientpool_free },
  { .id = FunctionResolve,     .fnc = (void_t) _clientpool_resolve },
  { .id = FunctionAllocString, .fnc = (void_t) _clientpool_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

int ClientPool = -1;

/* ------------------------------------------------------------------------ */

void _clientpool_init(void) {
  if (ClientPool < 0) {
    typedescr_register(ClientPool, clientpool_t);
  }
}

/* ------------------------------------------------------------------------ */

data_t * _clientpool_new(clientpool_t *pool, va_list args) {
  obelix_t *obelix = va_arg(args, obelix_t *);
  char     *prefix = va_arg(args, char *);
  uri_t    *server = va_arg(args, uri_t *);
  char     *max;
  long      l;

  if (server -> error) {
    return data_copy(server -> error);
  }
  if (!server -> port) {
    server -> port = OBELIX_DEFAULT_PORT;
  }
  pool -> obelix = obelix_copy(obelix);
  pool -> prefix = strdup(prefix);
  pool -> server = uri_copy(server);
  pool -> version = NULL;
  pool -> wait = condition_create();
  pool -> maxclients = 5;
  pool -> current = 0;
  if (server -> query && dict_has_key(server -> query, "maxclients")) {
    max = (char *) dict_get(server -> query, "maxclients");
    if (!strtoint(max, &l)) {
      pool -> maxclients = l;
    } else {
      error("Server URI '%s' has non-integer maxclients value", uri_tostring(server));
    }
  }
  pool -> clients = data_list_create();
  return (data_t *) pool;
}

void _clientpool_free(clientpool_t *pool) {
  if (pool) {
    condition_free(pool -> wait);
    list_free(pool -> clients);
    uri_free(pool -> server);
    free(pool -> prefix);
    free(pool -> version);
    obelix_free(pool -> obelix);
  }
}

char * _clientpool_tostring(clientpool_t *pool) {
  char *buf;

  asprintf(&buf, "Obelix client pool for server %s",
    uri_tostring(pool -> server));
  return buf;
}

data_t * _clientpool_resolve(clientpool_t *pool, char *name) {
  data_t *client;

  if (!strcmp(name, "server")) {
    return data_copy((data_t *) pool -> server);
  } else if (!strcmp(name, "prefix")) {
    return (data_t *) str_copy_chars(pool -> prefix);
  } else if (!strcmp(name, "serverversion")) {
    if (!pool -> version) {
      client = clientpool_checkout(pool);
      if (data_is_oblclient(client)) {
        clientpool_return(pool, (oblclient_t *) client);
      } else {
        return client;
      }
    }
    return (data_t *) str_copy_chars(pool -> version);
  } else {
    return NULL;
  }
}

/* ------------------------------------------------------------------------ */

clientpool_t * clientpool_create(obelix_t *obelix, char *prefix, uri_t *server) {
  _clientpool_init();
  return (clientpool_t *) data_create(ClientPool, obelix, prefix, server);
}

data_t * clientpool_checkout(clientpool_t *pool) {
  data_t *ret = NULL;

  while (!ret) {
    condition_acquire(pool -> wait);
    ret = (data_t *) list_shift(pool -> clients);
    if (!ret && (pool -> current < pool -> maxclients)) {
      ret = oblclient_create(pool);
      if (data_is_oblclient(ret)) {
        pool -> current++;
      }
    }
    if (!ret) {
      condition_sleep(pool -> wait);
    } else {
      condition_release(pool -> wait);
    }
  }
  return ret;
}

clientpool_t * clientpool_return(clientpool_t *pool, oblclient_t *client) {
  condition_acquire(pool -> wait);
  list_append(pool -> clients, client);
  condition_wakeup(pool -> wait);
  return pool;
}

data_t * clientpool_run(clientpool_t *pool, name_t *name, array_t *args, dict_t *kwargs) {
  oblclient_t *client;
  data_t      *ret;

  debug(obelix, "Running %s on client pool %s",
        name_tostring(name),
        data_tostring((data_t *) pool));
  ret = clientpool_checkout(pool);
  if (data_is_oblclient(ret)) {
    client = (oblclient_t *) ret;
    ret = oblclient_run(client, name_tostring(name), args, kwargs);
    clientpool_return(pool, client);
  }
  return ret;
}
