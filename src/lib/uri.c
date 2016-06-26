/*
 * /obelix/src/lib/uri.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <config.h>
#include <data.h>
#include <object.h>

#ifndef __URI_H__
#define __URI_H__

typedef struct _uri {
  data_t                 _d;
  str_t                 *scheme;
  str_t                 *user;
  str_t                 *password;
  str_t                 *host;
  str_t                 *port;
  str_t                 *path;
  str_t                 *query;
  str_t                 *fragment;
} uri_t;

extern uri_t *    uri_create(str_t *uri);
extern str_t *    uri_scheme(uri_t *uri);
extern str_t *    uri_password(uri_t *uri);
extern str_t *    uri_host(uri_t *uri);
extern str_t *    uri_port(uri_t *uri);
extern str_t *    uri_path(uri_t *uri);
extern str_t *    uri_querystring(uri_t *uri);
extern object_t * uri_query(uri_t *uri);
extern str_t *    uri_fragment(uri_t *uri);

extern int URI;

#define data_is_uri(d)     ((d) && data_hastype((d), URI))
#define data_urival(d)     (data_is_uri((d)) ? ((uri_t *) ((d) -> ptrval)) : NULL)
#define uri_copy(u)        ((uri_t *) data_copy((data_t *) (u)))
#define uri_free(u)        (data_free((data_t *) (u)))
#define uri_tostring(u)    (data_tostring((data_t *) (u)))

#endif /* __URI_H__ */

static int uri_debug;
