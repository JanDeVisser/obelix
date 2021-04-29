/* 
 * This file is part of the obelix distribution (https://github.com/JanDeVisser/obelix).
 * Copyright (c) 2021 Jan de Visser.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HEAP_H__
#define __HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FREEBLOCK_COOKIE   ((unsigned short int) 0xDEADBEEF)

extern void *heap_allocate(size_t);
extern void heap_unpen(void *);
extern void heap_deallocate(void *);
extern void heap_register_root(void *);
extern void heap_unregister_root(void *);
extern void heap_gc();
extern void heap_destroy();
extern void heap_report();

#ifdef __cplusplus
}
#endif

#endif /* __HEAP_H__ */