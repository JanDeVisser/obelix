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

#include "libipc.h"

extern void           mountpoint_init(void);

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

int ipc_debug = 0;
int Mountpoint = -1;

/* ------------------------------------------------------------------------ */

extern void servermessage_init(void);
extern void remote_init(void);
extern void client_init(void);
extern void server_init(void);

void ipc_init(void) {
  servermessage_init();
  mountpoint_init();
  remote_init();
  client_init();
  server_init();
}

void mountpoint_init(void) {
  if (Mountpoint < 1) {
    logging_register_module(ipc);
    typedescr_register(Mountpoint, mountpoint_t);
  }
}

/* ------------------------------------------------------------------------ */

data_t * _mountpoint_new(mountpoint_t *mp, va_list args) {
  uri_t  *remote = va_arg(args, uri_t *);
  char   *cookie = va_arg(args, char *);
  char   *max;
  long    l;

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
  mp -> _d.str = (cookie) ? strdup(cookie) : strrand(NULL, 32);
  data_set_string_semantics(mp, StrSemanticsStatic);
  if (mp -> remote -> query &&
      dict_has_key(mp -> remote -> query, "maxclients")) {
    max = (char *) dict_get(remote -> query, "maxclients");
    if (!strtoint(max, &l)) {
      mp -> maxclients = (int) l;
    } else {
      error("Server URI '%s' has non-integer maxclients value", uri_tostring(remote));
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
    free(mp -> _d.str);
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

data_t * mountpoint_create(uri_t *remote, char *cookie) {
  ipc_init();
  return data_create(Mountpoint, remote, cookie);
}

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

