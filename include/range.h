/*
 * /obelix/include/nvp.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __RANGE_H__
#define __RANGE_H__

#include <data.h>

typedef struct _range {
  data_t  _d;
  data_t *from;
  data_t *to;
  data_t *next;
  int     direction;
} range_t;

OBLCORE_IMPEXP data_t *      range_create(data_t *from, data_t *to);
OBLCORE_IMPEXP int           range_cmp(range_t *, range_t *);
OBLCORE_IMPEXP unsigned int  range_hash(range_t *);
OBLCORE_IMPEXP data_t *      range_iter(range_t *);
OBLCORE_IMPEXP data_t *      range_next(range_t *);
OBLCORE_IMPEXP data_t *      range_has_next(range_t *);

#define data_is_range(d)  ((d) && (data_hastype((d), Name)))
#define data_as_range(d)  ((range_t *) (data_is_range((d)) ? (d) : NULL))
#define range_free(n)     (data_free((data_t *) (n)))
#define range_tostring(n) (data_tostring((data_t *) (n)))
#define range_copy(n)     ((range_t *) data_copy((data_t *) (n)))

#endif /* __RANGE_H__ */
