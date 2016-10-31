/*
 * /obelix/include/resolve.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __RESOLVE_H__
#define __RESOLVE_H__

#include <list.h>
#include <dict.h>
#include <mutex.h>

#define OBL_DIR     "OBL_DIR"

#ifdef HAVE_DLFCN_H
typedef void * lib_handle_t;
typedef int    resolve_error_t;
#elif defined(HAVE_WINDOWS_H)
typedef HMODULE lib_handle_t;
typedef DWORD   resolve_error_t;
#endif /* HAVE_DLFCN_H */

typedef struct _resolve_handle {
  lib_handle_t            handle;
  char                   *image;
  char                   *platform_image;
  struct _resolve_handle *next;
} resolve_handle_t;

typedef struct _resolve {
  resolve_handle_t *images;
  dict_t           *functions;
} resolve_t;

OBLCORE_IMPEXP resolve_t * resolve_get(void);
OBLCORE_IMPEXP void        resolve_free(void);
OBLCORE_IMPEXP resolve_t * resolve_open(resolve_t *, char *);
OBLCORE_IMPEXP void_t      resolve_resolve(resolve_t *, char *);

OBLCORE_IMPEXP int         resolve_library(char *);
OBLCORE_IMPEXP void_t      resolve_function(char *);

  #endif /* __RESOLVE_H__ */
