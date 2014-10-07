/*
 * list.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include "core.h"
#include "list.h"

// -------------------
// listnode_t

static listnode_t * _ln_create(void *);
static int          _ln_datanode(listnode_t *);

// ---------------------------
// listnode_t static functions

listnode_t * _ln_create(void *data) {
  listnode_t *node;
  
  node = NEW(listnode_t);
  if (node) {
    node -> next = 0L;
    node -> prev = 0L;
    node -> data = data ? data : "";
  }
  return node;
}

int _ln_datanode(listnode_t *node) {
  return node -> next && node -> prev;
}

// -------------------
// list_t

list_t * list_create() {
  list_t *ret;

  ret = NEW(list_t);
  if (ret) {
    ret -> head = _ln_create(NULL);
    ret -> tail = _ln_create(NULL);
    ret -> head -> next = ret -> tail;
    ret -> tail -> prev = ret -> head;
    ret -> size = 0;
  } 
  return ret;
}

void list_free(list_t *list, visit_t free_fnc) {
  list_clear(list, free_fnc);
  free(list -> head);
  free(list -> tail);
  free(list);
}

int list_append(list_t *list, void *data) {
  listnode_t *node;
  int ret = FALSE;

  node = _ln_create(data);
  if (node) {
    node -> prev = list -> tail -> prev;
    node -> next = list -> tail;
    list -> tail -> prev -> next = node;
    list -> tail -> prev = node;
    list -> size++;
    ret = TRUE;
  }
  return ret;
}

int list_size(list_t *list) {
  return list -> size;
}

void * list_reduce(list_t *list, reduce_t reducer, void *data) {
  listiterator_t *iter = li_create(list);

  while (li_has_next(iter)) {
    data = reducer(li_next(iter), data);
  }
  li_free(iter);
  return data;
}

void list_visit(list_t *list, visit_t visitor) {
  listiterator_t *iter = li_create(list);

  while (li_has_next(iter)) {
    visitor(li_next(iter));
  }
  li_free(iter);
}

void list_clear(list_t *list, visit_t free_fnc) {
  listnode_t *node = list -> head -> next;
  listnode_t *next;
  void *data;

  while (_ln_datanode(node)) {
    next = node -> next;
    if (free_fnc) {
      free_fnc(node -> data);
    }
    free(node);
    node = next;
  }
  list -> head -> next = list -> tail;
  list -> tail -> prev = list -> head;
  list -> size = 0;
}

// -------------------
// listiterator_t

listiterator_t * li_create(list_t *list) {
  listiterator_t *ret;

  ret = NEW(listiterator_t);
  if (ret) {
    ret -> list = list;
    ret -> current = list -> head;
  }
  return ret;
}

void li_free(listiterator_t *iter) {
  free(iter);
}

void li_head(listiterator_t *iter) {
  iter -> current = iter -> list -> head;
}

void li_tail(listiterator_t *iter) {
  iter -> current = iter -> list -> tail;
}

void * li_current(listiterator_t *iter) {
  return (_ln_datanode(iter -> current))
    ? iter -> current -> data 
    : NULL;
}

int li_has_next(listiterator_t *iter) {
  return iter -> current -> next &&
    _ln_datanode(iter -> current -> next);
}

void * li_next(listiterator_t *iter) {
  void * ret = NULL;
  if (li_has_next(iter)) {
    iter -> current = iter -> current -> next;
    ret = li_current(iter);
  }
  return ret;
}

int li_has_prev(listiterator_t *iter) {
  return iter -> current -> prev &&
    _ln_datanode(iter -> current -> prev);
}

void * li_prev(listiterator_t *iter) {
  void * ret = NULL;
  if (li_has_prev(iter)) {
    iter -> current = iter -> current -> prev;
    ret = li_current(iter);
  }
  return ret;
}

int li_insert(listiterator_t *iter, void *data) {
  int         ret = FALSE;
  list_t     *list = iter -> list;
  listnode_t *node;
    
  /* If we're at the tail we can't insert a new node: */
  if (iter -> current -> next) {
    /* Not at end. Create node: */
    node = _ln_create(data);
    if (node) {
      /* Insert node after current node: */
      node -> prev = iter -> current;
      node -> next = iter -> current -> next;
      node -> prev -> next = node;
      node -> next -> prev = node;
      /* List is longer now: */
      list -> size++;
      /* Set iterator to new node: */
      iter -> current = node;
      ret = TRUE;
    }
  }
  return ret;
}

void * li_remove(listiterator_t *iter) {
  void *ret = NULL;
  listnode_t *node = NULL;

  if (_ln_datanode(iter -> current)) {
    node = iter -> current;
    ret = node -> data;
    node -> prev -> next = node -> next;
    node -> next -> prev = node -> prev;
    iter -> list -> size--;
    iter -> current = node -> next;
    free(node);
  }
  return ret;
}

#define __LIST_TEST__
#ifdef __LIST_TEST__
#include <stdio.h>

static void     test_raw_print_list(list_t *, char *);
static void     test_print_list(list_t *, char *);
static list_t * test_create_list();
static void     test_list_append(list_t *, char *);
extern void     test_suite();

void test_print_list(list_t *list, char *header) {
  listiterator_t *iter = li_create(list);

  printf("%s:\n{ ", header);
  while (li_has_next(iter)) {
    printf("[%s] ", (char *) li_next(iter));
  }
  printf(" } (%d)\n", list_size(list));
} 

void test_raw_print_list(list_t *list, char *header) {
  listnode_t *node = list -> head;

  printf("%s:\n{ ", header);
  while (node) {
    if (!node -> prev) {
      printf("[H%s] -> ", (char *) node -> data);
    } else if (!node -> next) {
      printf("[T%s]", (char *) node -> data);
    } else {
      printf("\"%s\" -> ", (char *) node -> data);
    }
    node = node -> next;
  }
  printf(" } (%d)\n", list_size(list));
} 

list_t * test_create_list() {
  list_t *list = list_create();
  test_print_list(list, "After create");
  return list;
}

void test_list_append(list_t *list, char *data) {
  list_append(list, data);
  test_print_list(list, "After list_append");  
}

void test_suite() {
  list_t *list = test_create_list();
  test_list_append(list, "test1");
  test_raw_print_list(list, "Raw print");
  test_list_append(list, "test2");
}

#endif /* __LIST_TEST__ */
