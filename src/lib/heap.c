/* 
 * This file is part of the obelix distribution (https://github.com/JanDeVisser/obelix).
 * Copyright (c) 2021 Jan de Visser.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <data.h>
#include <heap.h>
#include <list.h>
#include <logging.h>

typedef struct _free_block {
  unsigned char       is_live;
  unsigned char       marked;
  unsigned short int  cookie;
  void               *next;
} free_block_t;
 
typedef struct _heap_page {
  size_t  index;
  size_t  block_size;
  size_t  page_size;
  void   *freelist;
  void   *next_page;
} heap_page_t;

typedef struct _block_ptr {
  void              *block;
  struct _block_ptr *next;
} block_ptr_t;

typedef struct _heap {
  size_t        pagesize;
  size_t        pages;
  heap_page_t  *first;
  heap_page_t  *last;
  block_ptr_t  *roots;
} heap_t;

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

static heap_page_t *   _heap_page_create(size_t, size_t, size_t);
static void *          _heap_page_checkout(heap_page_t *);
static int             _heap_page_has_block(heap_page_t *, void *);
static free_block_t  * _heap_page_free_block(heap_page_t *, void *);
static void            _heap_page_unmark(heap_page_t *);
static void            _heap_page_sweep(heap_page_t *);

static void            _heap_init();
static heap_page_t *   _heap_new_page(size_t);
static heap_page_t *   _heap_new_page_for_size(size_t);
static heap_page_t *   _heap_find_page_for(void *block);
static void            _heap_unmark();
static void            _heap_find_and_mark_all_live_blocks();
static void            _heap_sweep();

static void *          _data_add_live_block(void *, void *);

static int heap_debug = 0;

/* -- F R E E _ B L O C k ------------------------------------------------ */

/* -- H E A P _ P A G E -------------------------------------------------- */

static heap_page_t * _heap_page_create(size_t index, size_t pagesize, size_t blocksize) {
  void         *raw_page;
  heap_page_t  *page;
  size_t        blocks = pagesize / blocksize - 1;
  free_block_t *blockptr, *prev_blockptr;
  int           ix;

  assert(blocksize >= sizeof(data_t));
  debug(heap, "Creating page %d with blocksize %d, %d blocks", index, blocksize, blocks);
  raw_page = malloc(pagesize);
  if (!raw_page) {
    return NULL;
  }
  page = (heap_page_t *) raw_page;
  page->index = index;
  page->page_size = pagesize;
  page->block_size = blocksize;

  for (prev_blockptr = NULL, ix = 0, blockptr = (free_block_t *) (raw_page + blocksize);
       ix < blocks;
       prev_blockptr = blockptr, ix++, blockptr = ((void *) blockptr) + blocksize) {
    blockptr->is_live = 0;
    blockptr->marked = 0;
    blockptr->cookie = FREEBLOCK_COOKIE;
    if (prev_blockptr) {
      prev_blockptr->next = blockptr;
    } else {
      page->freelist = blockptr;
    }
    blockptr->next = NULL;
  }
  return page;
}

static void * _heap_page_checkout(heap_page_t *page) {
  free_block_t *ret = page->freelist;

  debug(heap, "Checking out block %p from page %d, blocksize %d", ret, page->index, page->block_size);
  if (ret) {
    assert(ret->cookie == FREEBLOCK_COOKIE);
    page->freelist = ret->next;
    ret->cookie = 0;
    ret->is_live = 1;
  }
  return ret;
}

static int _heap_page_has_block(heap_page_t *page, void *block) {
  void  *pv = page;

  if ((block < (pv + page->block_size)) || (block > (pv + (page->page_size - page->block_size)))) {
    return 0;
  }
  if ((block - pv) % page->block_size) {
    return 0;
  }
  return 1;
}

static free_block_t  * _heap_page_free_block(heap_page_t *page, void *block) {
  free_block_t *free_block;
  data_t       *data;

  assert(_heap_page_has_block(page, block));
  debug(heap, "Returning block %p to page %d, blocksize %d", block, page->index, page->block_size);
  if (data_is_data(block)) {
    data = data_as_data(block);
    if (data -> data_semantics & DataSemanticsConstant) {
      return free_block;
    }
    debug(heap, "Block is a non-constant %s, destroying", data_typename(data));
    data_destroy(data_as_data(block));
  }
  free_block = (free_block_t *) block;
  free_block->is_live = 0;
  free_block->marked = 0;
  free_block->cookie = FREEBLOCK_COOKIE;
  free_block->next = page->freelist;
  page->freelist = free_block;
  return free_block;
}

static void _heap_page_unmark(heap_page_t *page) {
  void *block;
  void *vp = (void *) page;
  
  for (block = vp + page->block_size; block < vp + page->page_size; block += page->block_size) {
    ((free_block_t *) block)->marked = 0;  
  }
}

static void _heap_page_sweep(heap_page_t *page) {
  void         *block;
  void         *vp = (void *) page;
  free_block_t *free_block;

  for (block = vp + page->block_size; block < vp + page->page_size; block += page->block_size) {
    free_block = (free_block_t *) block;
    if (free_block->is_live && !free_block->marked) {
      _heap_page_free_block(page, free_block);
    }
  }
}

/* -- H E A P ------------------------------------------------------------ */

static heap_t the_heap = { .pagesize = 0, .pages = 0, .first = NULL, .last = NULL };

static void _heap_init() {
  size_t       min_blocksize = 1;
  size_t       blocksize;
  int          ix;

  if (the_heap.pagesize > 0) {
    return;
  }
  the_heap.pagesize = (16 * 1024);
  logging_register_module(heap);
  debug(heap, "Initalizing the heap");
  for (; min_blocksize < sizeof(data_t); min_blocksize *= 2);
  for (blocksize = min_blocksize; blocksize <= 256; blocksize *= 2) {
    for (ix = 0; ix < 4; ix++) {
      _heap_new_page(blocksize);
    }
  }
}

static heap_page_t * _heap_new_page(size_t blocksize) {
  heap_page_t *page;

  page = _heap_page_create((the_heap.last) ? the_heap.last->index + 1 : 0,
    the_heap.pagesize, blocksize);
  assert(page);
  if (!the_heap.first) {
    the_heap.first = page;
  }
  if (the_heap.last) {
    the_heap.last->next_page = page;
  }
  the_heap.last = page;
  the_heap.pages++;
  return page;
}

static heap_page_t * _heap_new_page_for_size(size_t size) {
  size_t blocksize;

  for (blocksize = 2; blocksize < size; blocksize *= 2);
  return _heap_new_page(blocksize);
}

static heap_page_t * _heap_find_page_for(void *block) {
  heap_page_t *page;

  for (page = the_heap.first; page; page = page->next_page) {
    if (_heap_page_has_block(page, block)) {
      return page;
    }
  }
  return NULL;
}

static void * _data_add_live_block(void *data, _unused_ void *ctx) {
  if (data && (data_is_data(data) || _heap_find_page_for(data))) {
    ((free_block_t *) data)->marked = 1;
  }
  return NULL;
}

static void _heap_find_and_mark_all_live_blocks() {
  block_ptr_t  *ptr;
  free_block_t *block;
  data_t       *data;

  for (ptr = the_heap.roots; ptr; ptr = ptr->next) {
    block = (free_block_t *) ptr->block;
    if (block->is_live) {
      block->marked = 1;
      if (data_is_data(block)) {
        data = data_as_data(block);
        data_reduce_children(data, (reduce_t) _data_add_live_block, NULL);
      }
    }
  }
}

static void _heap_sweep() {
  heap_page_t *page;
  
  for (page = the_heap.first; page; page = page->next_page) {
    _heap_page_sweep(page);
  }
}

static void _heap_unmark() {
  heap_page_t *page;

  for (page = the_heap.first; page; page = page->next_page) {
    _heap_page_unmark(page);
  }
}

/* -- P U B L I C  I N T E R F A C E ------------------------------------- */

extern void * heap_allocate(size_t size) {
  heap_page_t *page;
  void        *ret = NULL;
  size_t       smallest = 16*1024;
  heap_page_t *smallest_page = NULL;

  _heap_init();
  assert(size >= sizeof(data_t));
  debug(heap, "Allocating block of size %d", size);
  for (page = the_heap.first; page; page = page->next_page) {
    if (page->freelist && (page->block_size >= size) && (page->block_size < smallest)) {
      smallest_page = page;
      break;
    }
  }
  if (smallest_page) {
    page = smallest_page;
  } else {
    page = _heap_new_page_for_size(size);
  }
  ret = _heap_page_checkout(page);
  return ret;
}

extern void heap_deallocate(void *block) {
  heap_page_t  *page;
  free_block_t *free_block;

  _heap_init();
  debug(heap, "Deallocating block %p", block);
  page = _heap_find_page_for(block);
  if (page) {
    _heap_page_free_block(page, block);
  }
}

extern void heap_register_root(void *block) {
  block_ptr_t *root;
  
  _heap_init();
  root = NEW(block_ptr_t);
  root->block = block;
  root->next = the_heap.roots;
  the_heap.roots = root;
}

extern void heap_gc() {
  _heap_init();
  _heap_unmark();
  _heap_find_and_mark_all_live_blocks();
  _heap_sweep();
}
