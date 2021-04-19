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
#ifdef HAVE_QUERYPERFORMANCECOUNTER
#include <windows.h>
#endif
#ifdef HAVE_CLOCK_GETTIME
#include <time.h>
#endif

#include <timer.h>
#include <core.h>

typedef struct _timer_impl {
  long            seconds;
  long            microseconds;
#ifdef HAVE_QUERYPERFORMANCECOUNTER
  LARGE_INTEGER   frequency;
  LARGE_INTEGER   start;
  LARGE_INTEGER   end;
#endif
#ifdef HAVE_CLOCK_GETTIME
  struct timespec start;
  struct timespec end;
#endif
} timer_impl_t;

/* ------------------------------------------------------------------------ */


timestamp_t * timer_start(void) {
  timer_impl_t *ret = NEW(timer_impl_t);

#ifdef HAVE_QUERYPERFORMANCECOUNTER
  QueryPerformanceFrequency(&ret -> frequency);
  QueryPerformanceCounter(&ret -> start);
#endif
#ifdef HAVE_CLOCK_GETTIME
  clock_gettime(CLOCK_MONOTONIC, &ret -> start);
#endif
  return (timestamp_t *) ret;
}

timestamp_t * timer_end(timestamp_t *timer) {
  timer_impl_t *t = (timer_impl_t *) timer;
#ifdef HAVE_QUERYPERFORMANCECOUNTER
  LARGE_INTEGER elapsed_microseconds;

  QueryPerformanceCounter(&t -> end);
  elapsed_microseconds.QuadPart = t -> end.QuadPart - t -> start.QuadPart;
  /*
   * https://msdn.microsoft.com/en-us/library/windows/desktop/dn553408(v=vs.85).aspx
   *
   * We now have the elapsed number of ticks, along with the number of
   * ticks-per-second. We use these values to convert to the number of *
   * elapsed microseconds. To guard against loss-of-precision, we convert
   * to microseconds *before* dividing by ticks-per-second.
   */

  elapsed_microseconds.QuadPart *= 1000000;
  elapsed_microseconds.QuadPart /= t -> frequency.QuadPart;
  t -> seconds = (long) (elapsed_microseconds.QuadPart / 1000000);
  t -> microseconds = (long) (elapsed_microseconds.QuadPart % 1000000);
#endif
#ifdef HAVE_CLOCK_GETTIME
  clock_gettime(CLOCK_MONOTONIC, &t -> start);
  t -> microseconds = (t -> end.tv_nsec - t -> start.tv_nsec) / 1000L;
  t -> seconds = t -> end.tv_sec - t -> start.tv_sec;
  if (t -> microseconds < 0) {
    t -> seconds--;
    t -> microseconds = 1000000L - t -> microseconds;
  }
#endif
  return timer;
}
