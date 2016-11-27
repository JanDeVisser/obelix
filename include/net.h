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

#include <data.h>
#include <dictionary.h>
#include <name.h>
#include <str.h>

#ifndef OBLNET_IMPEXP
  #if (defined __WIN32__) || (defined _WIN32)
    #ifdef OBLNET_EXPORTS
      #define OBLNET_IMPEXP	__DLL_EXPORT__
    #elif defined(OBL_STATIC)
      #define OBLNET_IMPEXP extern
    #else /* ! OBLNET_EXPORTS */
      #define OBLNET_IMPEXP	__DLL_IMPORT__
    #endif
  #else /* ! __WIN32__ */
    #define OBLNET_IMPEXP extern
  #endif /* __WIN32__ */
#endif /* OBLNET_IMPEXP */

typedef struct _uri {
  data_t  _d;
  data_t *error;
  char   *scheme;
  char   *user;
  char   *password;
  char   *host;
  int     port;
  name_t *path;
  dict_t *query;
  char   *fragment;
} uri_t;

OBLNET_IMPEXP uri_t * uri_create(char *);
OBLNET_IMPEXP int URI;

#define data_is_uri(d)     ((d) && data_hastype((d), URI))
#define data_as_uri(d)     (data_is_uri((d)) ? ((uri_t *) (d)) : NULL)
#define uri_copy(u)        ((uri_t *) data_copy((data_t *) (u)))
#define uri_free(u)        (data_free((data_t *) (u)))
#define uri_tostring(u)    (data_tostring((data_t *) (u)))

#endif /* __NET_H__ */
