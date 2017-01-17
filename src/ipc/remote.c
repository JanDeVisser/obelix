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

extern void     remote_init(void);

static data_t * _remote_new(remote_t *, va_list);
static void     _remote_free(remote_t *);
static char *   _remote_tostring(remote_t *);
static data_t * _remote_resolve(remote_t *, char *);
static data_t * _remote_call(remote_t *, array_t *, dict_t *);

static vtable_t _vtable_Remote[] = {
  { .id = FunctionNew,         .fnc = (void_t) _remote_new },
  { .id = FunctionFree,        .fnc = (void_t) _remote_free },
  { .id = FunctionResolve,     .fnc = (void_t) _remote_resolve },
  { .id = FunctionAllocString, .fnc = (void_t) _remote_tostring },
  { .id = FunctionCall,        .fnc = (void_t) _remote_call },
  { .id = FunctionNone,        .fnc = NULL }
};

int Remote = -1;

/* ------------------------------------------------------------------------ */

void remote_init(void) {
  if (Remote < 1) {
    typedescr_register(Remote, remote_t);
  }
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

data_t * _remote_call(remote_t *remote, array_t *args, dict_t *kwargs) {
  client_t    *client;
  data_t      *ret;
  arguments_t *a;

  debug(ipc, "Running '%s' on mountpoint %s",
      name_tostring(remote->name),
      data_tostring((data_t *) remote->mountpoint));
  ret = mountpoint_checkout_client(remote->mountpoint);
  if (data_is_client(ret)) {
    client = (client_t *) ret;
    a = arguments_create(args, kwargs);
    ret = client_run(client, remote, a);
    arguments_free(a);
    mountpoint_return_client(remote->mountpoint, client);
  }
  return ret;
}

