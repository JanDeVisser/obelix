/*
 * core.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#ifndef __CORE_H__
#define __CORE_H__

#define TRUE     1
#define FALSE	 0
#define NEW(t)   ( (t *) _new( sizeof(t) ) )

typedef int    (*cmp_t)(void *, void *);
typedef int    (*hash_t)(void *);
typedef char * (*tostring_t)(void *);
typedef void * (*reduce_t)(void *, void *);
typedef void   (*visit_t)(void *);

extern void * _new(int);

#endif /* __CORE_H__ */
