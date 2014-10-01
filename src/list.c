/*
 * list.c - Copyright (c) 2014 Jan de Visser <jan@finiandarc.com>  
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

#include <errno.h>
#include <stdlib.h>

#include <core.h>
#include <list.h>

// -------------------
// listnode_t

static listnode_t * _ln_create(void *);

// ---------------------------
// listnode_t static functions

listnode_t * _ln_create(void *data) {
    listnode_t *node;

    node = NEW(listnode_t);
    if (node) {
        node -> next = 0L;
        node -> prev = 0L;
        node -> data = data;
    }
    return node;
}

// -------------------
// list_t

list_t * list_create() {
    list_t *ret;

    ret = NEW(list_t);
    if (ret) {
        ret -> head = 0L;
        ret -> tail = 0L;
        ret -> size = 0;
    } 
    return ret;
}

int list_append(list_t *list, void *data) {
    listnode_t *node;
    int ret = -1;

    node = _ln_create(data);
    if (node) {
        if (!list -> head) {
            list -> head = node;
        }
        node -> prev = list -> tail;
        if (list -> tail) {
            list -> tail -> next = node;
        }
        list -> tail = node;
        list -> size++;
        ret = 0;
    }
    return ret;
}

int list_size(list_t *list) {
    return list -> size;
}

// -------------------
// listiterator_t

listiterator_t * li_create(list_t *list) {
    listiterator_t *ret;

    ret = NEW(listiterator_t);
    if (ret) {
        ret -> list = list;
        ret -> ptr = list -> head;
    }
    return ret;
}

void li_head(listiterator_t *iter) {
    iter -> ptr = iter -> list -> head;
}

void li_tail(listiterator_t *iter) {
    iter -> ptr = iter -> list -> tail;
}

void * li_current(listiterator_t *iter) {
    return (iter -> ptr) ? iter -> ptr -> data : 0L;
}

int li_next(listiterator_t *iter) {
    int ret = -1;
    if (iter -> current) {
        iter -> current = iter -> current -> next;
        ret = 0;
    }
    return ret;
}

int li_hasnext(listiterator_t *iter) {
    int ret = -1;
    if (iter -> current) {
        iter -> current = iter -> current -> next;
        ret = 0;
    }
    return ret;
}

int li_prev(listiterator_t *iter) {
    int ret = -1;
    if (iter -> current) {
        iter -> current = iter -> current -> prev;
        ret = 0;
    }
    return ret;
}

int li_insert(listiterator_t *iter, void *data) {
    int         ret = -1;
    list_t     *list = iter -> list;
    listnode_t *node;
    
    if (!iter -> ptr) {
        ret = list_append(list, data);
    } else {
        node = _ln_create(data);
        if (node) {
            if (!list -> head) {
                list -> head = node;
            }
            node -> prev = list -> tail;
            if (list -> tail) {
                list -> tail -> next = node;
            }
            list -> tail = node;
            list -> size++;
            ret = 0;
        }
    }
    return ret;
}

void * li_remove(listiterator_t *iter) {
    
}



