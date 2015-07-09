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
#include <str.h>

static listnode_t *     _ln_init(listnode_t *, list_t *, void *);
static listnode_t *     _ln_create(list_t *, void *);
static void             _ln_free(listnode_t *);

#define _ln_datanode(n) (((n) -> next) && ((n) -> prev))

static int              _list_append_fragment(list_t *, listnode_t *, listnode_t *);

static listiterator_t * _li_init(listiterator_t *, list_t *);

static listnode_t  _ProcessEnd;
       listnode_t *ProcessEnd = &_ProcessEnd;

/* -- L I S T N O D E ----------------------------------------------------- */

listnode_t * _ln_init(listnode_t *node, list_t *list, void *data) {
  node -> next = NULL;
  node -> prev = NULL;
  node -> list = list;
  node -> data = data;
  return node;
}

listnode_t * _ln_create(list_t *list, void *data) {
  return _ln_init(NEW(listnode_t), list, data);
}

void _ln_free(listnode_t *node) {
  if (node) {
    if (node -> list -> freefnc && node -> data) {
      node -> list -> freefnc(node -> data);
    }
    free(node);
  }
}

/* -- L I S T ------------------------------------------------------------- */

int _list_append_fragment(list_t *list, listnode_t *start, listnode_t *end) {
  listnode_t *w;
  int         count = 0;

  start -> prev = NULL;
  end -> next = NULL;

  /*
   * Count elements in tail and update listnode_t -> list pointers:
   */
  for (w = start; w; w = w -> next) {
    count++;
    w -> list = list;
  }

  /*
   * Insert tail into return list:
   */
  start -> prev = list -> tail.prev;
  end -> next = &(list -> tail);
  list -> tail.prev -> next = start;
  list -> tail.prev = end;

  /*
   * Update counters:
   */
  list -> size += count;
  return count;
}

list_t * list_create() {
  list_t *ret;

  ret = NEW(list_t);
  if (ret) {
    _ln_init(&ret -> head, ret, NULL);
    _ln_init(&ret -> tail, ret, NULL);
    ret -> head.next = &ret -> tail;
    ret -> tail.prev = &ret -> head;
    ret -> size = 0;
    ret -> freefnc = NULL;
    ret -> cmp = NULL;
    _li_init(&ret -> iter, ret);
  }
  return ret;
}

list_t * _list_set_free(list_t *list, visit_t freefnc) {
  list -> freefnc = freefnc;
  return list;
}

list_t * _list_set_cmp(list_t *list, cmp_t cmp) {
  list -> cmp = cmp;
  return list;
}

list_t * _list_set_tostring(list_t *list, tostring_t tostring) {
  list -> tostring = tostring;
  return list;
}

list_t * _list_set_hash(list_t *list, hash_t hash) {
  list -> hash = hash;
  return list;
}

void list_free(list_t *list) {
  if (list) {
    list_clear(list);
    free(list);
  }
}

unsigned int list_hash(list_t *list) {
  unsigned int hash;
  reduce_ctx *ctx;

  if (!list -> hash) {
    hash = hashptr(list);
  } else {
    ctx = NEW(reduce_ctx);
    ctx -> fnc = (void_t) list -> hash;
    ctx -> longdata = 0;
    list_reduce(list, (reduce_t) collection_hash_reducer, ctx);
    hash = (unsigned int) ctx -> longdata;
    free(ctx);
  }
  return hash;
}

list_t * list_append(list_t *list, void *data) {
  listnode_t *node;

  node = _ln_create(list, data);
  node -> prev = list -> tail.prev;
  node -> next = &list -> tail;
  node -> prev -> next = node;
  list -> tail.prev = node;
  list -> size++;
  return list;
}

list_t * list_unshift(list_t *list, void *data) {
  listnode_t *node;

  node = _ln_create(list, data);
  node -> prev = &list -> head;
  node -> next = list -> head.next;
  node -> next -> prev = node;
  list -> head.next = node;
  list -> size++;
  return list;
}

list_t * list_add_all(list_t *list, list_t *other) {
  reduce_ctx *ctx;

  ctx = NEW(reduce_ctx);
  ctx -> fnc = (void_t) list_append;
  ctx -> obj = list;
  ctx = list_reduce(other, (reduce_t) collection_add_all_reducer, list);
  free(ctx);
  return list;
}

list_t * list_join(list_t *target, list_t *src) {
  listnode_t *start = src -> head.next;
  listnode_t *end = src -> tail.prev;

  if (list_size(src)) {
    src -> head.next = &src -> tail;
    src -> tail.prev = &src -> head;
    src -> size = 0;
    _list_append_fragment(target, start, end);
  }
  list_free(src);
  return target;
}

void * __list_reduce(list_t *list, reduce_t reducer, void *data, reduce_type_t type) {
  free_t      f;
  listnode_t *node;
  void       *elem;
  listnode_t *next;

  f = (type == RTStrs) ? (free_t) data_free : NULL;
  node = list_head_pointer(list);
  while (node && _ln_datanode(node)) {
    elem = node -> data;
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
    node = node -> next;
  }
  return data;
}

void * _list_reduce(list_t *list, reduce_t reduce, void *data) {
  return __list_reduce(list, (reduce_t) reduce, data, RTObjects);
}

void * _list_reduce_chars(list_t *list, reduce_t reduce, void *data) {
  assert(list -> tostring);
  return __list_reduce(list, (reduce_t) reduce, data, RTChars);
}

void * _list_reduce_str(list_t *list, reduce_t reduce, void *data) {
  assert(list -> tostring);
  return __list_reduce(list, (reduce_t) reduce, data, RTStrs);
}

list_t * _list_visit(list_t *list, visit_t visitor) {
  list_reduce(list, (reduce_t) collection_visitor, visitor);
  return list;
}

void * _list_process(list_t *list, reduce_t reducer, void *data) {
  listprocessor_t *lp = lp_create(list, reducer, data);
  void            *ret = lp_run(lp);
  
  lp_free(lp);
  return ret;
}

list_t * list_clear(list_t *list) {
  listnode_t *node;
  listnode_t *next;

  for (node = list -> head.next; _ln_datanode(node); node = next) {
    next = node -> next;
    _ln_free(node);
  }
  list -> head.next = &list -> tail;
  list -> tail.prev = &list -> head;
  list -> size = 0;
  return list;
}

void * list_head(list_t *list) {
  listnode_t *node = list -> head.next;

  return (_ln_datanode(node)) ? node -> data : NULL;
}

void * list_tail(list_t *list) {
  listnode_t *node = list -> tail.prev;

  return (_ln_datanode(node)) ? node -> data : NULL;
}

listnode_t * list_head_pointer(list_t *list) {
  listnode_t *node = list -> head.next;

  return (_ln_datanode(node)) ? node : NULL;
}

listnode_t * list_tail_pointer(list_t *list) {
  listnode_t *node = list -> tail.prev;

  return (_ln_datanode(node)) ? node : NULL;
}

void * list_shift(list_t *list) {
  listnode_t *node = list -> head.next;
  listnode_t *next;
  void *ret = NULL;

  if (_ln_datanode(node)) {
    next = node -> next;
    ret = node -> data;
    node -> data = NULL;
    _ln_free(node);
    list -> head.next = next;
    next -> prev = &list -> head;
    list -> size--;
  }
  return ret;
}

void * list_pop(list_t *list) {
  listnode_t *node = list -> tail.prev;
  listnode_t *prev;
  void *ret = NULL;

  if (_ln_datanode(node)) {
    prev = node -> prev;
    ret = node -> data;
    node -> data = NULL;
    _ln_free(node);
    list -> tail.prev = prev;
    prev -> next = &list -> tail;
    list -> size--;
  }
  return ret;
}

str_t * list_tostr(list_t *list) {
  str_t *ret;
  str_t *catted;

  ret = str_copy_chars("[");
  catted = str_join(", ", list, _list_reduce_chars);
  if (ret && catted) {
    str_append(ret, catted);
    str_append_chars(ret, "]");
  }
  str_free(catted);
  return ret;
}

listiterator_t * list_start(list_t *list) {
  li_head(&list -> iter);
  return &list -> iter;
}

listiterator_t * list_end(list_t *list) {
  li_tail(&list -> iter);
  return &list -> iter;
}

listiterator_t * list_position(listnode_t *node) {
  list_t *list = node -> list;
  li_position(&list -> iter, node);
  return &list -> iter;
}

void * list_current(list_t *list) {
  return li_current(&list -> iter);
}

int list_has_next(list_t *list) {
  return li_has_next(&list -> iter);
}

int list_has_prev(list_t *list) {
  return li_has_prev(&list -> iter);
}

void * list_next(list_t *list) {
  return li_next(&list -> iter);
}

void * list_prev(list_t *list) {
  return li_prev(&list -> iter);
}

void list_remove(list_t *list) {
  li_remove(&list -> iter);
}

int list_atstart(list_t *list) {
  return li_atstart(&list -> iter);
}

int list_atend(list_t *list) {
  return li_atend(&list -> iter);
}

list_t * list_split(list_t *list) {
  listnode_t *start = li_pointer(&list -> iter);
  listnode_t *end = list -> tail.prev;
  listnode_t *w;
  list_t     *ret = list_create();
  int         count = 0;

  list_set_cmp(ret, list -> cmp);
  list_set_free(ret, list -> freefnc);
  list_set_hash(ret, list -> hash);
  list_set_tostring(ret, list -> tostring);
  if (!list_atstart(list) && !list_atend(list)) {
    /*
     * Cut tail out of list:
     */
    start -> prev -> next = &list -> tail;
    list -> tail.prev = start -> prev;

    list -> size -= _list_append_fragment(ret, start, end);
  }
  return ret;
}

/* -- L I S T I T E R A T O R --------------------------------------------- */

listiterator_t * _li_init(listiterator_t *iter, list_t *list) {
  iter -> list = list;
  iter -> current = &list -> head;
  return iter;
}

listiterator_t * li_create(list_t *list) {
  listiterator_t *ret;

  ret = NEW(listiterator_t);
  return _li_init(ret, list);
}

void li_free(listiterator_t *iter) {
  if (iter) {
    free(iter);
  }
}

void li_head(listiterator_t *iter) {
  iter -> current = &iter -> list -> head;
}

void li_tail(listiterator_t *iter) {
  iter -> current = &iter -> list -> tail;
}

void li_position(listiterator_t *iter, listnode_t *node) {
  assert(iter -> list == node -> list);
  iter -> current = node;
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
  return iter -> current -> next && _ln_datanode(iter -> current -> next);
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
  return iter -> current -> prev && _ln_datanode(iter -> current -> prev);
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

int li_atstart(listiterator_t *iter) {
  return !iter -> current -> prev;
}

int li_atend(listiterator_t *iter) {
  return !iter -> current -> next;
}

/* -- L I S T P R O C E S S O R ------------------------------------------- */

listprocessor_t * lp_create(list_t *list, reduce_t processor, void *data) {
  listprocessor_t *ret = NEW(listprocessor_t);
  
  ret -> list = list;
  ret -> processor = processor;
  ret -> data = data;
  return ret;
}

void lp_free(listprocessor_t *lp) {
  if (lp) {
    free(lp);
  }
}

void * lp_run(listprocessor_t *lp) {
  lp -> current = NULL;
  while (lp_step(lp));
  return lp -> data;
}

listprocessor_t * lp_step(listprocessor_t *lp) {
  void       *elem;
  listnode_t *next;
  
  if (lp_atstart(lp)) {
    lp -> current = list_head_pointer(lp -> list);
  }
  if (!lp_atend(lp)) {
    elem = lp -> current -> data;
    next = (listnode_t *) (lp -> processor)(elem, lp -> data);
    if (next) {
      lp -> current = next;
    } else {
      lp -> current = lp -> current -> next;
    }
    return lp;
  } else {
    return NULL;
  }
}

int lp_atstart(listprocessor_t *lp) {
  return lp -> current == NULL;
}

int lp_atend(listprocessor_t *lp) {
  return lp -> current && 
         ((lp -> current == ProcessEnd) || 
          !_ln_datanode(lp -> current));
}

void * lp_current(listprocessor_t *lp) {
  return (!lp_atstart(lp) && !lp_atend(lp)) ? lp -> current -> data : NULL;
}