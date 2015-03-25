/*
 * /obelix/include/socket.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __SOCKET_H__
#define	__SOCKET_H__

#include <file.h>

#ifdef	__cplusplus
extern "C" {
#endif
    
typedef struct _socket {
  file_t *sockfile;
  char   *host;
  char   *service;
  char   *str;
  int     refs;
} socket_t;

extern socket_t *    socket_create(char *, int);
extern socket_t *    socket_create_byservice(char *, char *);
extern socket_t *    socket_copy(socket_t *);
extern void          socket_free(socket_t *);
extern int           socket_close(socket_t *);
extern unsigned int  socket_hash(socket_t *);
extern int           socket_cmp(socket_t *, socket_t *);
extern char *        socket_tostring(socket_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* __SOCKET_H__ */
