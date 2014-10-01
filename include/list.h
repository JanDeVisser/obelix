/*
 * list.h - Copyright (c) 2014 Jan de Visser <jan@finiandarc.com>
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

typedef struct _listnode {
    struct _listnode *prev;
    struct _listnode *next;
    void             *data;
} listnode_t;

typedef struct _list {
    listnode_t *head;
    listnode_t *tail;
    int         size;
} list_t;

extern list_t * list_create();
extern int      list_append(list_t *, void *);
extern int      list_size(list_t *);

typedef struct _listiter {
    list_t     *list;
    listnode_t *current;
} listiterator_t;

extern listiterator_t * li_create(list_t *);
extern void             li_head(listiterator_t *);
extern void             li_tail(listiterator_t *);
extern void *           li_current(listiterator_t *);
extern int              li_insert(listiterator_t *, void *);
extern void *           li_remove(listiterator_t *);
extern int              li_hasnext(listiterator_t *);
extern int              li_hasprev(listiterator_t *);
extern int              li_next(listiterator_t *);
extern int              li_prev(listiterator_t *);

#endif /* __LIST_H__ */
