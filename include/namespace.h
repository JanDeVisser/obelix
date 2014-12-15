/*
 * /obelix/include/namespace.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <object.h>
#include <script.h>

typedef struct _namespace {
  int       native_allowed;
  object_t *root;
} ns_t;

extern ns_t *     ns_create(script_t *);
extern void       ns_free(ns_t *);
extern object_t * ns_get(ns_t *, array_t *);

#endif /* __NAMESPACE_H__ */
