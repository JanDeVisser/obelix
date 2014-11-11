/*
 * /obelix/src/stringbuffer.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __STRINGBUFFER_H__
#define __STRINGBUFFER_H__

#include <core.h>

typedef struct _stringbuffer {
  read_t  read_fnc;
  char   *buffer;
  int     pos;
  int     len;
} stringbuffer_t;

extern stringbuffer_t * sb_create(char *);
extern void             sb_free(stringbuffer_t *);
extern int              sb_read(stringbuffer_t *, char *, int);

#endif /* __STRINGBUFFER_H__ */
