/*
 * /obelix/src/lib/libcore.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __LIBCORE_H__
#define __LIBCORE_H__

#include <oblconfig.h>

#define LIBCORE_IMPLEMENTING
#define OBLCORE_IMPEXP       __DLL_EXPORT__

#include <core.h>

extern void     any_init(void);
extern void     str_init(void);
extern void     int_init(void);
extern void     float_init(void);
extern void     list_init(void);
extern void     exception_init(void);
extern void     ptr_init(void);
extern void     file_init(void);
extern void     mutex_init(void);
extern void     name_init(void);
extern void     hierarchy_init(void);

extern int  data_debug;
extern int  name_debug;

#endif /* __LIBCORE_H__ */
