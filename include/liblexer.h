/*
 * libparser.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __LIBLEXER_H__
#define __LIBLEXER_H__

#include <config.h>
#include <core.h>

#if (defined __WIN32__) || (defined _WIN32)
#ifdef obllexer_EXPORTS
#define OBLLEXER_IMPEXP	__DLL_EXPORT__
#else /* ! oblcore_EXPORTS */
#define OBLLEXER_IMPEXP	__DLL_IMPORT__
#endif
#else /* ! __WIN32__ */
#define OBLLEXER_IMPEXP extern
#endif /* __WIN32__ */

#endif /* __LIBLEXER_H__ */
