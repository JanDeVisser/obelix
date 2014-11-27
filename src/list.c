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

#include <list.h>

// -------------------
// static declarations

static listnode_t * _ln_create(list_t *, void *);
static int          _ln_datanode(listnode_t *);
static void         _ln_free(listnode_t *);

static list_t *     _list_add_all_reducer(void *, list_t *);
static visit_t      _list_visitor(void *, visit_t);
static void *       _list_reduce(list_t *, reduce_t, void *, reduce_type_t);

// ---------------------------
// listnode_t static functions

listnode_t * _ln_create(list_t *list, void *data) {
  listnode_t *node;
  
  node = NEW(listnode_t);
  if (node) {
    node -> next = NULL;
    node -> prev = NULL;
    node -> list = list;
    node -> data = data;
  }
  return node;
}

int _ln_datanode(listnode_t *node) {
  return node -> next && node -> prev;
}

void _ln_free(listnode_t *node) {
  if (node) {
    if (node -> list -> freefnc && node -> data) {
      node -> list -> freefnc(node -> data);
    }
    free(node);
  }
}

// ------------------------
// list_t static functions

void * _list_reduce(list_t *list, reduce_t reducer, void *data, reduce_type_t type) {
  listiterator_t *iter;
  free_t          f;
  void           *elem;

  iter = li_create(list);
  f = (type == RTStrs) ? (free_t) str_free : NULL;
  while (li_has_next(iter)) {
    elem = li_next(iter);
    switch (type) {
      case RTChars:
        elem =  list -> tostring(elem);
        break;
      case RTStrs:
        elem =  str_wrap(list -> tostring(elem));
        break;
    }
    data = reducer(elem, data);
    if (f) {
      f(elem);
    }
  }
  li_free(iter);
  return data;
}

list_t * _list_add_all_reducer(void *data, list_t *list) {
  list_append(list, data);
  return list;
}

visit_t _list_visitor(void *data, visit_t visitor) {
  visitor(data);
  return visitor;
}

// ------------------------
// list_t public functions

list_t * list_create() {
  list_t *ret;

  ret = NEW(list_t);
  if (ret) {
    ret -> head = _ln_create(ret, NULL);
    ret -> tail = _ln_create(ret, NULL);
    ret -> head -> next = ret -> tail;
    ret -> tail -> prev = ret -> head;
    ret -> size = 0;
    ret -> freefnc = NULL;
    ret -> cmp = NULL;
  } 
  return ret;
}

list_t * list_set_free(list_t *list, visit_t freefnc) {
  list -> freefnc = freefnc;
  return list;
}

list_t * list_set_cmp(list_t *list, cmp_t cmp) {
  list -> cmp = cmp;
  return list;
}

list_t * list_set_tostring(list_t *list, tostring_t tostring) {
  list -> tostring = tostring;
  return list;
}

void list_free(list_t *list) {
  if (list) {
    list_clear(list);
    _ln_free(list -> head);
    _ln_free(list -> tail);
    free(list);
  }
}

list_t * list_append(list_t *list, void *data) {
  listnode_t *node;

  node = _ln_create(list, data);
  node -> prev = list -> tail -> prev;
  node -> next = list -> tail;
  node -> prev -> next = node;
  list -> tail -> prev = node;
  list -> size++;
  return list;
}

list_t * list_unshift(list_t *list, void *data) {
  listnode_t *node;

  node = _ln_create(list, data);
  node -> prev = list -> head;
  node -> next = list -> head -> next;
  node -> next -> prev = node;
  list -> head -> next = node;
  list -> size++;
  return list;
}

list_t * list_add_all(list_t *list, list_t *other) {
  return list_reduce(other, (reduce_t) _list_add_all_reducer, list);
}

int list_size(list_t *list) {
  return list -> size;
}

void * list_reduce(list_t *list, reduce_t reducer, void *data) {
  return _list_reduce(list, reducer, data, RTObjects);
}

void * list_reduce_chars(list_t *list, reduce_t reducer, void *data) {
  return _list_reduce(list, reducer, data, RTChars);
}

void * list_reduce_str(list_t *list, reduce_t reducer, void *data) {
  return _list_reduce(list, reducer, data, RTStrs);
}

list_t * list_visit(list_t *list, visit_t visitor) {
  list_reduce(list, (reduce_t) _list_visitor, visitor);
  return list;
}

void * list_process(list_t *list, reduce_t reducer, void *data) {
  listnode_t *node;
  void       *elem;
  listnode_t *next;

  node = list_head_pointer(list);
  while (node && _ln_datanode(node)) {
    elem = node -> data;
    next = (listnode_t *) reducer(elem, data);
    if (next) {
      node = next;
    } else {
      node = node -> next;
    }
  }
  return data;
}

list_t * list_clear(list_t *list) {
  listnode_t *node;
  listnode_t *next;

  for (node = list -> head -> next; _ln_datanode(node); node = next) {
    next = node -> next;
    _ln_free(node);
  }
  list -> head -> next = list -> tail;
  list -> tail -> prev = list -> head;
  list -> size = 0;
  return list;
}

void * list_head(list_t *list) {
  listnode_t *node = list -> head -> next;

  return (_ln_datanode(node)) ? node -> data : NULL;
}

void * list_tail(list_t *list) {
  listnode_t *node = list -> tail -> prev;

  return (_ln_datanode(node)) ? node -> data : NULL;
}

listnode_t * list_head_pointer(list_t *list) {
  listnode_t *node = list -> head -> next;

  return (_ln_datanode(node)) ? node : NULL;
}

listnode_t * list_tail_pointer(list_t *list) {
  listnode_t *node = list -> tail -> prev;

  return (_ln_datanode(node)) ? node : NULL;
}

void * list_shift(list_t *list) {
  listnode_t *node = list -> head -> next;
  listnode_t *next;
  void *ret = NULL;

  if (_ln_datanode(node)) {
    next = node -> next;
    ret = node -> data;
    node -> data = NULL;
    _ln_free(node);
    list -> head -> next = next;
    next -> prev = list -> head;
    list -> size--;
  }
  return ret;
}

void * list_pop(list_t *list) {
  listnode_t *node = list -> tail -> prev;
  listnode_t *prev;
  void *ret = NULL;

  if (_ln_datanode(node)) {
    prev = node -> prev;
    ret = node -> data;
    node -> data = NULL;
    _ln_free(node);
    list -> tail -> prev = prev;
    prev -> next = list -> tail;
    list -> size--;
  }
  return ret;
}

str_t * list_tostr(list_t *list) {
  str_t *ret;
  str_t *catted;

  ret = str_copy_chars("[");
  catted = str_join(", ", list, list_reduce_chars);
  if (ret && catted) {
    str_append(ret, catted);
    str_append_chars(ret, "]");
  }
  str_free(catted);
  return ret;
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
  if (iter) {
    free(iter);
  }
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

listnode_t * li_pointer(listiterator_t *iter) {
  return (_ln_datanode(iter -> current))
    ? iter -> current
    : NULL;
}

void li_replace(listiterator_t *iter, void *data) {
  if (_ln_datanode(iter -> current)) {
    if (iter -> list -> freefnc && iter -> current -> data) {
      iter -> list -> freefnc(iter -> current -> data);
    }
    iter -> current -> data = data;
  }
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
    node = _ln_create(iter -> list, data);
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

void li_remove(listiterator_t *iter) {
  void *data = NULL;
  listnode_t *node = NULL;

  if (_ln_datanode(iter -> current)) {
    node = iter -> current;
    node -> prev -> next = node -> next;
    node -> next -> prev = node -> prev;
    iter -> current = node -> next;
    _ln_free(node);
    iter -> list -> size--;
  }
}
