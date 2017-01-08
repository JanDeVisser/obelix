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

#include "libnet.h"

typedef struct _client client_t;

typedef struct _mountpoint {
  data_t       _d;
  uri_t       *remote;
  condition_t *wait;
  char        *prefix;
  char        *version;
  int          maxclients;
  int          current;
  list_t      *clients;
} mountpoint_t;

typedef struct _remote {
  data_t        _d;
  mountpoint_t *mountpoint;
  name_t       *name;
} remote_t;

typedef struct _client {
  data_t        _d;
  mountpoint_t *mountpoint;
  socket_t     *socket;
} oblclient_t;

static inline void    _mountpoint_init(void);

static data_t *       _mountpoint_new(mountpoint_t *, va_list);
static void           _mountpoint_free(mountpoint_t *);
static char *         _mountpoint_tostring(mountpoint_t *);
static data_t *       _mountpoint_resolve(mountpoint_t *, char *);

static vtable_t _vtable_Mountpoint[] = {
  { .id = FunctionNew,         .fnc = (void_t) _mountpoint_new },
  { .id = FunctionFree,        .fnc = (void_t) _mountpoint_free },
  { .id = FunctionResolve,     .fnc = (void_t) _mountpoint_resolve },
  { .id = FunctionAllocString, .fnc = (void_t) _mountpoint_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static data_t *       _remote_new(remote_t *, va_list);
static void           _remote_free(remote_t *);
static char *         _remote_tostring(remote_t *);
static data_t *       _remote_resolve(remote_t *, char *);

static vtable_t _vtable_Remote[] = {
  { .id = FunctionNew,         .fnc = (void_t) _remote_new },
  { .id = FunctionFree,        .fnc = (void_t) _remote_free },
  { .id = FunctionResolve,     .fnc = (void_t) _remote_resolve },
  { .id = FunctionAllocString, .fnc = (void_t) _remote_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};


extern data_t *               client_create(clientpool_t *);
extern data_t *               client_run(oblclient_t *, char *, array_t *, dict_t *);


int cs_debug = 0;
int Mountpoint = -1;
int Remote = -1;
int Client = -1;

#define data_is_mountpoint(d)  ((d) && (data_hastype((data_t *) (d), Mountpoint)))
#define data_as_mountpoint(d)  ((mountpoint_t *) (data_is_mountpoint((data_t *) (d)) ? (d) : NULL))
#define mountpoint_free(o)     (data_free((data_t *) (o)))
#define mountpoint_tostring(o) (data_tostring((data_t *) (o)))
#define mountpoint_copy(o)     ((mountpoint_t *) data_copy((data_t *) (o)))

#define data_is_remote(d)      ((d) && (data_hastype((data_t *) (d), Remote)))
#define data_as_remote(d)      ((remote_t *) (data_is_remote((data_t *) (d)) ? (d) : NULL))
#define remote_free(o)         (data_free((data_t *) (o)))
#define remote_tostring(o)     (data_tostring((data_t *) (o)))
#define remote_copy(o)         ((remote_t *) data_copy((data_t *) (o)))

#define data_is_client(d)      ((d) && data_hastype((d), Client))
#define data_as_client(d)      (data_is_oblclient((d)) ? ((client_t *) (d)) : NULL)
#define client_copy(o)         ((oblclient_t *) data_copy((data_t *) (o)))
#define client_tostring(o)     (data_tostring((data_t *) (o))
#define client_free(o)         (data_free((data_t *) (o)))

/* ------------------------------------------------------------------------ */

void _mountpoint_init(void) {
  if (Mountpoint < 1) {
    logging_register_module(cs);
    typedescr_register(Mountpoint, mountpoint_t);
    typedescr_register(Remote, remote_t);
  }
}

/* ------------------------------------------------------------------------ */

data_t * _mountpoint_new(mountpoint_t *mp, va_list args) {
  uri_t  *remote = va_arg(args, uri_t *);

  if (remote -> error) {
    return remote -> error;
  }
  mp -> remote = uri_copy(remote);
  if (!mp -> remote -> port) {
    mp -> remote -> port = OBELIX_DEFAULT_PORT;
  }
  mp -> version = NULL;
  mp -> wait = condition_create();
  mp -> maxclients = 5;
  mp -> current = 0;
  if (mp -> remote -> query &&
      dict_has_key(mp -> remote -> query, "maxclients")) {
    max = (char *) dict_get(server -> query, "maxclients");
    if (!strtoint(max, &l)) {
      mp -> maxclients = l;
    } else {
      error("Server URI '%s' has non-integer maxclients value", uri_tostring(server));
    }
  }
  mp -> clients = data_list_create();
  return (data_t *) mp;
}

void _mountpoint_free(mountpoint_t *mp) {
  if (mp) {
    uri_free(mp -> remote);
    condition_free(mp -> wait);
    list_free(mp -> clients);
    free(mp -> prefix);
    free(mp -> version);
  }
}

char * _mountpoint_tostring(mountpoint_t *mp) {
  char *buf;

  asprintf(&buf, " --> %s", uri_tostring(mp -> remote));
  return buf;
}

data_t * _mountpoint_resolve(mountpoint_t *mp, char *name) {
  return data_create(Remote, mp, name_create(1, name));
}

/* ------------------------------------------------------------------------ */

data_t * mountpoint_checkout_client(mountpoint_t *mp) {
  data_t *ret = NULL;

  while (!ret) {
    condition_acquire(mp -> wait);
    ret = (data_t *) list_shift(mp -> clients);
    if (!ret && (mp -> current < mp -> maxclients)) {
      ret = client_create(mp);
      if (data_is_client(ret)) {
        mp -> current++;
      }
    }
    if (!ret) {
      condition_sleep(mp -> wait);
    } else {
      condition_release(mp -> wait);
    }
  }
  return ret;
}

mountpoint_t * mountpoint_return_client(mountpoint_t *mp, client_t *client) {
  condition_acquire(mp -> wait);
  list_append(mp -> clients, client);
  condition_wakeup(mp -> wait);
  return mp;
}

/* ------------------------------------------------------------------------ */

data_t * _remote_new(remote_t *remote, va_list args) {
  remote -> mountpoint = mountpoint_copy(va_arg(args, mountpoint_t *));
  remote -> name = va_arg(args, name_t *);
  return (data_t *) remote;
}

void _remote_free(remote_t *remote) {
  if (remote) {
    name_free(remote -> name);
    mountpoint_free(remote -> mountpoint);
  }
}

char * _remote_tostring(remote_t *remote) {
  char *buf;

  asprintf(&buf, "%s / %s",
      uri_tostring(remote -> mountpoint -> remote),
      name_tostring_sep(remote -> name, "/"));
  return buf;
}

data_t * _remote_resolve(remote_t *remote, char *name) {
  name_t *n = name_deepcopy(remote -> name);

  name_extend(n, name);
  return data_create(Remote, remote -> mountpoint, n);
}

data_t * _remote_set(remote_t *remote, char *name, data_t *value) {
  client_t *client;
  data_t   *ret;

  debug(cs, "Setting '%s' := '%s' on mountpoint %s",
        name_tostring(name), data_tostring(value),
        data_tostring((data_t *) remote -> mountpoint));
  ret = mountpoint_checkout_client(remote -> mountpoint);
  if (data_is_client(ret)) {
    client = (client_t *) ret;
    ret = client_set(client, remote -> name, name, value);
    mountpoint_return_client_return(pool, client);
  }
  return ret;
}

data_t * _remote_call(remote_t *remote, array_t *args, dict_t *kwargs) {
  client_t *client;
  data_t   *ret;

  debug(cs, "Running '%s' on mountpoint %s",
        name_tostring(name),
        data_tostring((data_t *) remote -> mountpoint));
  ret = mountpoint_checkout_client(remote -> mountpoint);
  if (data_is_client(ret)) {
    client = (client_t *) ret;
    ret = client_run(client, remote -> name, args, kwargs);
    mountpoint_return_client_return(pool, client);
  }
  return ret;
}

data_t * _remote_get_value(remote_t *remote) {
  client_t *client;
  data_t   *ret;

  debug(cs, "Getting '%s' on mountpoint %s",
        name_tostring(name),
        data_tostring((data_t *) remote -> mountpoint));
  ret = mountpoint_checkout_client(remote -> mountpoint);
  if (data_is_client(ret)) {
    client = (client_t *) ret;
    ret = client_get(client, remote -> name);
    mountpoint_return_client_return(pool, client);
  }
  return ret;
}
