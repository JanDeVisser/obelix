/*
 * list.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __LIST_H__
#define __LIST_H__

#include <core.h>

struct _list;
struct _str;

typedef struct _listnode {
  struct _listnode *prev;
  struct _listnode *next;
  struct _list     *list;
  void             *data;
} listnode_t;

typedef struct _listiter {
  struct _list *list;
  listnode_t   *current;
} listiterator_t;

typedef struct _list {
  listnode_t      head;
  listnode_t      tail;
  int             size;
  type_t          type;
  listiterator_t  iter;
  char           *str;
} list_t;

typedef struct _listprocessor {
  list_t     *list;
  reduce_t    processor;
  void       *data;
  listnode_t *current;
} listprocessor_t;

OBLCORE_IMPEXP list_t *            list_create();
OBLCORE_IMPEXP list_t *            list_clone(list_t *);
OBLCORE_IMPEXP list_t *            list_set_type(list_t *, type_t *);
OBLCORE_IMPEXP list_t *            _list_set_free(list_t *, visit_t);
OBLCORE_IMPEXP list_t *            _list_set_cmp(list_t *, cmp_t);
OBLCORE_IMPEXP list_t *            _list_set_tostring(list_t *, tostring_t);
OBLCORE_IMPEXP list_t *            _list_set_hash(list_t *, hash_t);
OBLCORE_IMPEXP void                list_free(list_t *);
OBLCORE_IMPEXP list_t *            list_append(list_t *, void *);
OBLCORE_IMPEXP list_t *            list_unshift(list_t *, void *);
OBLCORE_IMPEXP list_t *            list_add_all(list_t *, list_t *);
OBLCORE_IMPEXP list_t *            list_join(list_t *, list_t *);
OBLCORE_IMPEXP unsigned int        list_hash(list_t *);
OBLCORE_IMPEXP void *              list_get(list_t *, int);
OBLCORE_IMPEXP void *              __list_reduce(list_t *, reduce_t, void *, reduce_type_t);
OBLCORE_IMPEXP void *              _list_reduce(list_t *, reduce_t, void *);
OBLCORE_IMPEXP void *              _list_reduce_chars(list_t *, reduce_t, void *);
OBLCORE_IMPEXP void *              _list_reduce_str(list_t *, reduce_t, void *);
OBLCORE_IMPEXP list_t *            _list_visit(list_t *, visit_t);
OBLCORE_IMPEXP void *              _list_process(list_t *, reduce_t, void *);
OBLCORE_IMPEXP list_t *            list_clear(list_t *);
OBLCORE_IMPEXP void *              list_head(list_t *);
OBLCORE_IMPEXP void *              list_tail(list_t *);
OBLCORE_IMPEXP listnode_t *        list_head_pointer(list_t *);
OBLCORE_IMPEXP listnode_t *        list_tail_pointer(list_t *);
OBLCORE_IMPEXP void *              list_shift(list_t *);
OBLCORE_IMPEXP void *              list_pop(list_t *);
OBLCORE_IMPEXP struct _str *       list_tostr(list_t *);
OBLCORE_IMPEXP char *              list_tostring(list_t *);

OBLCORE_IMPEXP listiterator_t *    list_start(list_t *);
OBLCORE_IMPEXP listiterator_t *    list_end(list_t *);
OBLCORE_IMPEXP listiterator_t *    list_position(listnode_t *);
OBLCORE_IMPEXP void *              list_current(list_t *);
OBLCORE_IMPEXP int                 list_has_next(list_t *);
OBLCORE_IMPEXP int                 list_has_prev(list_t *);
OBLCORE_IMPEXP void *              list_next(list_t *);
OBLCORE_IMPEXP void *              list_prev(list_t *);
OBLCORE_IMPEXP void                list_remove(list_t *);
OBLCORE_IMPEXP int                 list_atstart(list_t *);
OBLCORE_IMPEXP int                 list_atend(list_t *);
OBLCORE_IMPEXP list_t *            list_split(list_t *);

#define list_size(l)               ((l) -> size)
#define list_push(l, d)            list_append((l), (d))
#define list_peek(l)               list_tail((l))
#define list_empty(l)              (list_size((l)) == 0)
#define list_notempty(l)           (list_size((l)) > 0)

#define list_set_free(l, f)        (_list_set_free((l), (free_t) (f)))
#define list_set_cmp(l, f)         (_list_set_cmp((l), (cmp_t) (f)))
#define list_set_tostring(l, f)    (_list_set_tostring((l), (tostring_t) (f)))
#define list_set_hash(l, f)        (_list_set_hash((l), (hash_t) (f)))
#define list_reduce(l, r, d)       (_list_reduce((l), (reduce_t) (r), (d)))
#define list_reduce_chars(l, r, d) (_list_reduce_chars((l), (reduce_t) (r), (d)))
#define list_reduce_str(l, r, d)   (_list_reduce_str((l), (reduce_t) (r), (d)))
#define list_visit(l, v)           (_list_visit((l), (visit_t) (v)))
#define list_process(l, r, d)      (_list_process((l), (reduce_t) (r), (d)))

#define str_list_create()          (list_set_type(list_create(), coretype(CTString)))
#define int_list_create()          (list_set_type(list_create(), coretype(CTInteger)))

OBLCORE_IMPEXP listiterator_t *    li_create(list_t *);
OBLCORE_IMPEXP void                li_free(listiterator_t *);
OBLCORE_IMPEXP void                li_head(listiterator_t *);
OBLCORE_IMPEXP void                li_tail(listiterator_t *);
OBLCORE_IMPEXP void                li_position(listiterator_t *, listnode_t *);
OBLCORE_IMPEXP void *              li_current(listiterator_t *);
OBLCORE_IMPEXP listnode_t *        li_pointer(listiterator_t *);
OBLCORE_IMPEXP void                li_replace(listiterator_t *, void *);
OBLCORE_IMPEXP int                 li_insert(listiterator_t *, void *);
OBLCORE_IMPEXP void                li_remove(listiterator_t *);
OBLCORE_IMPEXP int                 li_has_next(listiterator_t *);
OBLCORE_IMPEXP int                 li_has_prev(listiterator_t *);
OBLCORE_IMPEXP void *              li_next(listiterator_t *);
OBLCORE_IMPEXP void *              li_prev(listiterator_t *);
OBLCORE_IMPEXP int                 li_atstart(listiterator_t *);
OBLCORE_IMPEXP int                 li_atend(listiterator_t *);

OBLCORE_IMPEXP listprocessor_t *   lp_create(list_t *, reduce_t, void *);
OBLCORE_IMPEXP void                lp_free(listprocessor_t *);
OBLCORE_IMPEXP void *              lp_run(listprocessor_t *);
OBLCORE_IMPEXP listprocessor_t *   lp_step(listprocessor_t *);
OBLCORE_IMPEXP listprocessor_t *   lp_run_to(listprocessor_t *, void *);
OBLCORE_IMPEXP int                 lp_atstart(listprocessor_t *);
OBLCORE_IMPEXP int                 lp_atend(listprocessor_t *);
OBLCORE_IMPEXP void *              lp_current(listprocessor_t *);

OBLCORE_IMPEXP list_t             *EmptyList;
OBLCORE_IMPEXP listnode_t         *ProcessEnd;

#endif /* __LIST_H__ */
