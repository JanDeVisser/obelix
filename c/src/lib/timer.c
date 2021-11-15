/*
 * /obelix/src/lib/timer.c - Copyright (c) 2017 Jan de Visser <jan@finiandarcy.com>
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

#include <oblconfig.h>
#include <time.h>

#include <timer.h>
#include <core.h>

typedef struct _timer_impl {
  long            seconds;
  long            microseconds;
  struct timespec start;
  struct timespec end;
} timer_impl_t;

/* ------------------------------------------------------------------------ */


timestamp_t * timer_start(void) {
  timer_impl_t *ret = NEW(timer_impl_t);

  clock_gettime(CLOCK_MONOTONIC, &ret -> start);
  return (timestamp_t *) ret;
}

timestamp_t * timer_end(timestamp_t *timer) {
  timer_impl_t *t = (timer_impl_t *) timer;
  clock_gettime(CLOCK_MONOTONIC, &t -> end);
  t -> microseconds = (t -> end.tv_nsec - t -> start.tv_nsec) / 1000L;
  t -> seconds = t -> end.tv_sec - t -> start.tv_sec;
  if (t -> microseconds < 0) {
    t -> seconds--;
    t -> microseconds = 1000000L - t -> microseconds;
  }
  return timer;
}
