/*
 * /obelix/src/parser/libparser.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LIBNET_H__
#define __LIBNET_H__

#ifndef oblnet_EXPORTS
  #define OBLNET_IMPEXP extern
  #define OBL_STATIC
#endif

#include <config.h>
#include <stdio.h>
#include <data.h>
#include <net.h>

extern void net_init(void);

extern int net_debug;

#endif /* __LIBSQL_H__ */
