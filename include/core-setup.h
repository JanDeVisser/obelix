/*
 * core-setup.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __CORE_SETUP_H__
#define __CORE_SETUP_H__

#include <config.h>

#include <assert.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
#include <stdlib.h>

#if (defined __WIN32__) || (defined _WIN32)
#ifndef __GNUC__
#define __ATTRIBUTE_DLLIMPORT__
#define __ATTRIBUTE_DLLEXPORT__
#define __DLL_IMPORT__           __declspec(dllimport)
#define __DLL_EXPORT__           __declspec(dllexport)
#else
#define __ATTRIBUTE_DLLIMPORT__  __attribute__((dllimport))
#define __ATTRIBUTE_DLLEXPORT__  __attribute__((dllexport))
#define __DLL_IMPORT__           __ATTRIBUTE_DLLIMPORT__ extern
#define __DLL_EXPORT__           __ATTRIBUTE_DLLEXPORT__ extern
#endif
#else /* !__WIN32__ */
#define __ATTRIBUTE_DLLIMPORT__
#define __ATTRIBUTE_DLLEXPORT__
#define __DLL_IMPORT__           extern
#define __DLL_EXPORT__           extern
#endif /* __WIN32__ */

#ifdef __GNUC__
#define _unused_ __attribute__((unused))
#else
#define _unused_
#endif

#ifndef OBLCORE_IMPEXP
#if (defined __WIN32__) || (defined _WIN32)
#ifdef oblcore_EXPORTS
#define OBLCORE_IMPEXP       __DLL_EXPORT__
#elif defined(OBL_STATIC)
#define OBLCORE_IMPEXP       extern
#else /* ! oblcore_EXPORTS */
#define OBLCORE_IMPEXP       __DLL_IMPORT__
#endif
#else /* ! __WIN32__ */
#define OBLCORE_IMPEXP extern
#endif /* __WIN32__ */
#endif /* OBLCORE_IMPEXP */

#ifndef HAVE_INTPTR_T
typedef long long intptr_t; /* FIXME: 32bits! */
#endif /* HAVE_INTPTR_T */

#endif /* __CORE_SETUP_H__ */
