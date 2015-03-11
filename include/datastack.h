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
#include <list.h>
#include <name.h>

typedef struct _datastack {
  char     *name;
  int       debug;
  list_t   *list;
  list_t   *bookmarks;
  list_t   *counters;
} datastack_t;

extern datastack_t * datastack_create(char *);
extern datastack_t * datastack_set_debug(datastack_t *, int);
extern void          datastack_free(datastack_t *);
extern char *        datastack_tostring(datastack_t *);
extern int           datastack_hash(datastack_t *);
extern int           datastack_cmp(datastack_t *, datastack_t *);
extern int           datastack_depth(datastack_t *);
extern data_t *      datastack_pop(datastack_t *);
extern data_t *      datastack_peek(datastack_t *);
extern datastack_t * datastack_push(datastack_t *, data_t *);
extern datastack_t * datastack_push_int(datastack_t *, long);
extern datastack_t * datastack_push_string(datastack_t *, char *);
extern datastack_t * datastack_push_float(datastack_t *, double);
extern datastack_t * datastack_list(datastack_t *);
extern datastack_t * datastack_clear(datastack_t *);
extern datastack_t * datastack_bookmark(datastack_t *);
extern array_t *     datastack_rollup(datastack_t *);
extern name_t *      datastack_rollup_name(datastack_t *);
extern datastack_t * datastack_new_counter(datastack_t *);
extern datastack_t * datastack_increment(datastack_t *);
extern int           datastack_count(datastack_t *);

#define datastack_empty(l)        (datastack_depth((l)) == 0)
#define datastack_notempty(l)     (datastack_depth((l)) > 0)


#endif /* __DATASTACK_H__ */
