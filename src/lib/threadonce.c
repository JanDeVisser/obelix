/*
 * /obelix/src/lib/threadonce.c - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#include "libcore.h"
#include <threadonce.h>

/* ------------------------------------------------------------------------ */

#ifdef HAVE_INITONCEEXECUTEONCE

BOOL CALLBACK InitHandleFunction(PINIT_ONCE InitOnce, PVOID fnc, PVOID *lpContext) {
  void_t f = (void_t) fnc;

  f();
  return TRUE;
}

#endif /* HAVE_INITONCEEXECUTEONCE */
