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

#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <data.h>
#include <heap.h>
#include <list.h>
#include <logging.h>

typedef struct _heap_page {
  size_t  index;
  size_t  block_size;
  size_t  page_size;
  size_t  blocks;
  void   *freelist;
  void   *next_page;
} __attribute__((aligned(64))) heap_page_t;

typedef struct _block_ptr {
  void              *block;
  struct _block_ptr *next;
} __attribute__((aligned(16))) block_ptr_t;

typedef struct _heap {
  size_t         pagesize;
  size_t         pages;
  heap_page_t   *first;
  heap_page_t   *last;
  block_ptr_t   *roots;
  unsigned long  allocations_since_last_gc;
  unsigned long  allocated_bytes_since_last_gc;
} __attribute__((aligned(64))) heap_t;

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

#define BLOCK_DEAD           0x00u
#define BLOCK_ALIVE_HERDED   0x01u
#define BLOCK_ALIVE_SHEPARD  0x02u
#define BLOCK_ALIVE_PENNED   0x04u
#define BLOCK_UNPENNED       ~BLOCK_ALIVE_PENNED

#define BLOCK_IS_HERDED(b)   ((b)->is_live & BLOCK_ALIVE_HERDED)
#define BLOCK_IS_SHEPARD(b)  ((b)->is_live & BLOCK_ALIVE_SHEPARD)
#define BLOCK_IS_PENNED(b)   ((b)->is_live & BLOCK_ALIVE_PENNED)
#define BLOCK_UNPEN(b)       ((b)->is_live = ((b)->is_live & BLOCK_UNPENNED)|BLOCK_ALIVE_HERDED)

static void            _heap_lock_init();
static void            _heap_lock();
static void            _heap_unlock();
static void *          _heap_gc_thread(void *);

static heap_page_t *   _heap_page_create(size_t, size_t, size_t);
static void *          _heap_page_checkout(heap_page_t *);
static int             _heap_page_has_block(heap_page_t *, void *);
static void            _heap_page_destroy_block(heap_page_t *, void *);
static heap_block_t  * _heap_page_free_block(heap_page_t *, void *);
static void            _heap_page_unmark(heap_page_t *);
static void            _heap_page_sweep(heap_page_t *);
static heap_page_t *   _heap_page_free(heap_page_t *);
static size_t          _heap_page_report(heap_page_t *);

static void            _heap_init();
static heap_page_t *   _heap_new_page(size_t);
static heap_page_t *   _heap_new_page_for_size(size_t);
static heap_page_t *   _heap_find_page_for(void *block);
static void            _heap_unmark();
static void            _heap_find_and_mark_all_live_blocks();
static void            _heap_sweep();

static void *          _data_add_live_block(heap_block_t *, void *);

static int             heap_debug = 0;
static pthread_mutex_t _mutex;
static pthread_once_t  _init = PTHREAD_ONCE_INIT;
static pthread_t       _gc_thread;

static heap_t the_heap = {
  .pagesize = 0,
  .pages = 0,
  .first = NULL,
  .last = NULL,
  .roots = NULL,
  .allocations_since_last_gc = 0L,
  .allocated_bytes_since_last_gc = 0L,
};

/* ------------------------------------------------------------------------ */

void _heap_lock_init() {
  pthread_mutexattr_t mutex_attr;
  pthread_attr_t      thr_attr;

  logging_register_module(heap);
  debug(heap, "Initializing heap multi-threading infrastructure");
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
  if ((errno = pthread_mutex_init(&_mutex, &mutex_attr))) {
    fprintf(stderr, "Error initializing logging mutex: %s\n", strerror(errno));
    abort();
  }
  pthread_mutexattr_destroy(&mutex_attr);

  pthread_attr_init(&thr_attr);
  pthread_create(&_gc_thread, &thr_attr, _heap_gc_thread, NULL);
  pthread_attr_destroy(&thr_attr);

  atexit(heap_destroy);
}

void _heap_lock() {
  errno = pthread_mutex_lock(&_mutex);
  if (errno) {
    fprintf(stderr, "Error locking heap mutex: %s\n", strerror(errno));
    abort();
  }
}

void _heap_unlock() {
  errno = pthread_mutex_unlock(&_mutex);
  if (errno) {
    fprintf(stderr, "Error locking heap mutex: %s\n", strerror(errno));
    abort();
  }
}

_Noreturn void * _heap_gc_thread(_unused_ void *ctx) {
  struct timespec ts;
  debug(heap, "GC thread started");
  for (;;) {
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    (void) nanosleep(&ts, NULL);
    _heap_init();
    _heap_lock();
    if ((the_heap.pagesize > 0) && (the_heap.allocations_since_last_gc > 1000)) {
      debug(heap, "GC thread: %d allocations since last GC", the_heap.allocations_since_last_gc);
      heap_gc();
    }
    _heap_unlock();
  }
//  return NULL;
}

/* -- F R E E _ B L O C k ------------------------------------------------ */

/* -- H E A P _ P A G E -------------------------------------------------- */

static heap_page_t * _heap_page_create(size_t index, size_t pagesize, size_t blocksize) {
  void         *raw_page;
  heap_page_t  *page;
  size_t        blocks = pagesize / blocksize - 1;
  heap_block_t *blockptr, *prev_blockptr;
  int           ix;

  assert(blocksize >= sizeof(data_t));
  debug(heap, "Creating page %d with blocksize %d, %d blocks", index, blocksize, blocks);
  raw_page = malloc(pagesize);
  if (!raw_page) {
    return NULL;
  }
  memset(raw_page, 0, pagesize);
  page = (heap_page_t *) raw_page;
  page->index = index;
  page->page_size = pagesize;
  page->block_size = blocksize;
  page->blocks = blocks;
  page->next_page = NULL;

  for (prev_blockptr = NULL, ix = 0, blockptr = (heap_block_t *) (raw_page + blocksize);
       ix < blocks;
       prev_blockptr = blockptr, ix++, blockptr = ((void *) blockptr) + blocksize) {
    blockptr->is_live = BLOCK_DEAD;
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
  heap_block_t *ret = page->freelist;

  debug(heap, "Checking out block %p from page %d, blocksize %d", ret, page->index, page->block_size);
  if (ret) {
    assert(ret->cookie == FREEBLOCK_COOKIE);
    page->freelist = ret->next;
    ret->cookie = 0;
    ret->is_live = BLOCK_ALIVE_PENNED;
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

static void _heap_page_destroy_block(heap_page_t *page, void *block) {
  data_t *data;

  if (block && ((heap_block_t *) block)->is_live && data_is_data(block)) {
    data = data_as_data(block);
    debug(heap, "Destroying data object of type %s", data_typename(data));
    if (data -> data_semantics & DataSemanticsConstant) {
      return;
    }
    data_destroy(data_as_data(block));
  }
}

static heap_block_t  * _heap_page_free_block(heap_page_t *page, void *block) {
  heap_block_t *free_block;

  assert(_heap_page_has_block(page, block));
  free_block = (heap_block_t *) block;
  if (!free_block->is_live) {
    return free_block;
  }
  debug(heap, "Returning block %p to page %d, blocksize %d", block, page->index, page->block_size);
  _heap_page_destroy_block(page, block);
  free_block = (heap_block_t *) block;
  free_block->is_live = BLOCK_DEAD;
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
    ((heap_block_t *) block)->marked = 0;
  }
}

static void _heap_page_sweep(heap_page_t *page) {
  void         *block;
  void         *vp = (void *) page;
  heap_block_t *free_block;

  for (block = vp + page->block_size; block < vp + page->page_size; block += page->block_size) {
    free_block = (heap_block_t *) block;
    if ((BLOCK_IS_HERDED(free_block) && !free_block->marked) || BLOCK_IS_PENNED(free_block)) {
      _heap_page_free_block(page, free_block);
    }
  }
}

static heap_page_t * _heap_page_free(heap_page_t *page) {
  void         *block;
  void         *vp = (void *) page;
  heap_block_t *free_block;
  heap_page_t  *next = page->next_page;

  debug(heap, "Freeing page #%d", page->index);
  for (block = vp + page->block_size; block < vp + page->page_size; block += page->block_size) {
    _heap_page_destroy_block(page, block);
  }
  free(page);
  return next;
}

static size_t _heap_page_report(heap_page_t *page) {
  size_t        allocated = 0;
  void         *block;
  void         *vp = (void *) page;
  heap_block_t *free_block;

  for (block = vp + page->block_size; block < vp + page->page_size; block += page->block_size) {
    free_block = (heap_block_t *) block;
    if (free_block->is_live) allocated++;
  }
  info("%-4d %6d %6d %6d %6d %6d",
       page->index, page->block_size, allocated, allocated * page->block_size,
       page->blocks - allocated, (page->blocks - allocated)*page->block_size);
  return allocated;
}

/* -- H E A P ------------------------------------------------------------ */

static void _heap_init() {
  size_t min_blocksize = 1;
  size_t blocksize;
  int    ix;

  if (the_heap.pagesize > 0) {
    return;
  }

  pthread_once(&_init, _heap_lock_init);
  _heap_lock();
  the_heap.pagesize = (16 * 1024);
  debug(heap, "Initializing the heap");
  for (; min_blocksize < sizeof(data_t); min_blocksize *= 2);
  for (blocksize = min_blocksize; blocksize <= 256; blocksize *= 2) {
    for (ix = 0; ix < 4; ix++) {
      _heap_new_page(blocksize);
    }
  }
  _heap_unlock();
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

static void * _data_add_live_block(heap_block_t *block, _unused_ void *ctx) {
  data_t *data;

  if (block && (data_is_data(block) || _heap_find_page_for(block))) {
    if (!block->marked) {
      block->marked = 1;
      BLOCK_UNPEN(block);
      block->is_live |= BLOCK_ALIVE_HERDED;
      if (data_is_data(block)) {
        data = data_as_data(block);
        ctx = data_reduce_children(data, (reduce_t) _data_add_live_block, ctx);
      }
    }
  }
  return ctx;
}

static void _heap_find_and_mark_all_live_blocks() {
  block_ptr_t  *ptr;
  heap_block_t *block;
  data_t       *data;

  for (ptr = the_heap.roots; ptr; ptr = ptr->next) {
    block = (heap_block_t *) ptr->block;
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
  _heap_lock();
  if (the_heap.allocations_since_last_gc > 1000) {
    heap_gc();
  }
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
  the_heap.allocations_since_last_gc++;
  the_heap.allocated_bytes_since_last_gc += page->block_size;
  _heap_unlock();
  return ret;
}

extern void heap_unpen(void *block) {
  heap_block_t *free_block = (heap_block_t *) block;

  if (free_block) {
    BLOCK_UNPEN(free_block);
  }
}

extern void heap_deallocate(void *block) {
  heap_page_t  *page;
  heap_block_t *free_block;

  _heap_init();
  debug(heap, "Deallocating block %p", block);
  _heap_lock();
  heap_unregister_root(block);
  page = _heap_find_page_for(block);
  if (page) {
    _heap_page_free_block(page, block);
  }
  _heap_unlock();
}

extern void heap_register_root(void *block) {
  block_ptr_t   *root;
  heap_block_t *free_block = (heap_block_t *) block;
  
  _heap_init();
  free_block->is_live |= BLOCK_ALIVE_SHEPARD;
  BLOCK_UNPEN(free_block);
  root = NEW(block_ptr_t);
  root->block = block;
  _heap_lock();
  root->next = the_heap.roots;
  the_heap.roots = root;
  _heap_unlock();
}

extern void heap_unregister_root(void *block) {
  block_ptr_t   *cur;
  block_ptr_t  **prev = NULL;
  heap_block_t  *free_block = (heap_block_t *) block;

  /*
   * Syntactically more complex than it needs to be. Proof of context eliminating
   * an if block.
   */
  _heap_init();
  if (!BLOCK_IS_SHEPARD(free_block)) {
    return;
  }
  _heap_lock();
  for (prev = &the_heap.roots; *prev; prev = &(cur->next)) {
    cur = *prev;
    if (cur->block == block) {
      *prev = cur->next;
      free(cur);
      break;
    }
  }
  _heap_unlock();
  free_block->is_live &= ~BLOCK_ALIVE_SHEPARD;
}

extern void heap_gc() {
  log_timestamp_t *ts;

  _heap_init();
  info("Garbage collection");
  ts = log_timestamp_start(heap);
  _heap_lock();
  if (the_heap.pagesize > 0) {
    _heap_unmark();
    _heap_find_and_mark_all_live_blocks();
    _heap_sweep();
    the_heap.allocations_since_last_gc = 0;
    the_heap.allocated_bytes_since_last_gc = 0;
  }
  _heap_unlock();
  log_timestamp_end(heap, ts, "GC Sweep took ")
}

extern void heap_destroy() {
  block_ptr_t *ptr;
  heap_page_t *page;

  if (the_heap.pagesize == 0) {
    return;
  }
  info("Destroying heap");
  if (heap_debug) {
    heap_report();
  }

  _heap_lock();
  while (the_heap.roots) {
    ptr = the_heap.roots;
    the_heap.roots = ptr->next;
    free(ptr);
  }

  for (page = the_heap.first; page; ) {
    page = _heap_page_free(page);
  }
  the_heap.first = NULL;
  the_heap.last = NULL;
  the_heap.roots = NULL;
  the_heap.pages = 0;
  the_heap.pagesize = 0;
  the_heap.allocations_since_last_gc = 0L;
  the_heap.allocated_bytes_since_last_gc = 0L;
  _heap_unlock();
}

extern void heap_report() {
  size_t       allocated_blocks = 0;
  size_t       total_allocated_blocks = 0;
  size_t       total_allocated_bytes = 0;
  size_t       unallocated_blocks = 0;
  size_t       unallocated_bytes = 0;
  heap_page_t *page;

  _heap_init();
  info("       H E A P  S U M M A R Y");
  info("=======================================");
  info("Block Size  Alloc  Alloc# Avail  Avail#");
  info("---------------------------------------");
  for (page = the_heap.first; page; page=page->next_page) {
    allocated_blocks = _heap_page_report(page);
    total_allocated_blocks += allocated_blocks;
    total_allocated_bytes += (size_t) (allocated_blocks * page->block_size);
    unallocated_blocks += page->blocks - allocated_blocks;
    unallocated_bytes += (page->blocks - allocated_blocks) * page->block_size;
  }
  info("%-11.11s %6d %6d %6d %6d",
       "TOTAL", total_allocated_blocks, total_allocated_bytes,
       unallocated_blocks, unallocated_bytes);
  info("---------------------------------------");
}