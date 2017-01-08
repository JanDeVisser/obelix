/*
 * /obelix/include/datastack.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __DATASTACK_H__
#define __DATASTACK_H__

#include <data.h>
#include <array.h>
#include <name.h>
#include <str.h>

typedef struct _datastack {
  char    *name;
  int      debug;
  array_t *list;
  array_t *bookmarks;
  array_t *counters;
} datastack_t;

OBLCORE_IMPEXP datastack_t * datastack_create(char *);
OBLCORE_IMPEXP datastack_t * datastack_set_debug(datastack_t *, int);
OBLCORE_IMPEXP void          datastack_free(datastack_t *);
OBLCORE_IMPEXP char *        datastack_tostring(datastack_t *);
OBLCORE_IMPEXP int           datastack_hash(datastack_t *);
OBLCORE_IMPEXP int           datastack_cmp(datastack_t *, datastack_t *);
OBLCORE_IMPEXP int           datastack_depth(datastack_t *);
OBLCORE_IMPEXP data_t *      datastack_pop(datastack_t *);
OBLCORE_IMPEXP data_t *      datastack_peek_deep(datastack_t *, int);
OBLCORE_IMPEXP data_t *      datastack_peek(datastack_t *);
OBLCORE_IMPEXP datastack_t * _datastack_push(datastack_t *, data_t *);
OBLCORE_IMPEXP datastack_t * datastack_list(datastack_t *);
OBLCORE_IMPEXP datastack_t * datastack_clear(datastack_t *);
OBLCORE_IMPEXP data_t *      datastack_find(datastack_t *, cmp_t, void *);
OBLCORE_IMPEXP datastack_t * datastack_bookmark(datastack_t *);
OBLCORE_IMPEXP array_t *     datastack_rollup(datastack_t *);
OBLCORE_IMPEXP name_t *      datastack_rollup_name(datastack_t *);
OBLCORE_IMPEXP datastack_t * datastack_new_counter(datastack_t *);
OBLCORE_IMPEXP datastack_t * datastack_increment(datastack_t *);
OBLCORE_IMPEXP int           datastack_count(datastack_t *);
OBLCORE_IMPEXP int           datastack_current_count(datastack_t *);

OBLCORE_IMPEXP int           datastack_find_type(data_t *, long);

static inline datastack_t * datastack_push(datastack_t *stack, void *data) {
  return _datastack_push(stack, data_as_data(data));
}

static inline datastack_t * datastack_push_int(datastack_t *stack, long l) {
  return datastack_push(stack, int_to_data(l));
}

static inline datastack_t * datastack_push_string(datastack_t * stack, char *s) {
  return datastack_push(stack, str_copy_chars(s));
}

static inline datastack_t * datastack_push_float(datastack_t *stack, double d) {
  return datastack_push(stack, flt_to_data(d));
}

#define datastack_empty(l)            (datastack_depth((l)) == 0)
#define datastack_notempty(l)         (datastack_depth((l)) > 0)
#define datastack_find_bytype(s, t)   (datastack_find((s), (cmp_t) datastack_find_type, (void *)((long) (t))))

#endif /* __DATASTACK_H__ */
