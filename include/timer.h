/*
* obelix/include/timer.h - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <oblconfig.h>

#if !defined(HAVE_QUERYPERFORMANCECOUNTER) && !defined(HAVE_CLOCK_GETTIME)
#error "Please provide timer implementation"
#endif

typedef struct _timestamp {
  long seconds;
  long microseconds;
} timestamp_t;

extern timestamp_t * timer_start(void);
extern timestamp_t * timer_end(timestamp_t *);

#endif /* __TIMER_H__ */
