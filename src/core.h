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

#include <stdarg.h>

#define TRUE   1
#define FALSE  0
#define NEW(t) ( (t *) new( sizeof(t) ) )

typedef int    (*cmp_t)(void *, void *);
typedef int    (*hash_t)(void *);
typedef char * (*tostring_t)(void *);
typedef void * (*reduce_t)(void *, void *);
typedef void   (*visit_t)(void *);

extern void * new(int);
extern void * new_ptrarray(int);
extern void * resize_block(void *, int);
extern void * resize_ptrarray(void *, int);

extern unsigned int hash(void *, size_t);
extern void         debug(char *, ...);

extern char *       strrand(char *, size_t);
extern unsigned int strhash(char *);

#endif /* __CORE_H__ */
