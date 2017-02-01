/*
* obelix/include/threadonce.h - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __THREADONCE_H__
#define __THREADONCE_H__

#include <oblconfig.h>

#ifdef HAVE_PTHREAD_H

#include <pthread.h>

#define THREAD_ONCE(var)  static pthread_once_t var = PTHREAD_ONCE_INIT;
#define ONCE(var, fnc)    pthread_once(&var, fnc);

#elif defined(HAVE_INITONCEEXECUTEONCE)

#include <windows.h>

OBLCORE_IMPEXP BOOL CALLBACK InitHandleFunction(PINIT_ONCE, PVOID, PVOID *);
#define THREAD_ONCE(var)  static INIT_ONCE var = INIT_ONCE_STATIC_INIT;
#define ONCE(var, fnc)    (void) InitOnceExecuteOnce(&var, InitHandleFunction, (fnc), NULL);

#else

#error "Please provide threadonce implementation"

#endif

#endif /* __THREADONCE_H__ */
