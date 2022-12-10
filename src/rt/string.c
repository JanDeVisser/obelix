/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STRING_IMPL

typedef enum _string_control_block_type {
    AVAILABLE = 0x00,
    VIEW = 0x01,
    SMALL = 0x02,
    HEAP = 0x04,
    STATIC = 0x08,
} string_control_block_type;

typedef struct _string_control_block {
    union {
        uint32_t count;
        int32_t next;
    };
    string_control_block_type type;
    uint32_t length;
    char* data;
} string_control_block;

typedef struct _string_pool {
    string_control_block strings[4096];
    int32_t first;
    struct _string_pool *next;
} string_pool;

typedef string_control_block* string;

#include <rt/obelix.h>

#define SMALLSZ (sizeof(char*) - 1)
#define IS_AVAILABLE(s) (s->type == AVAILABLE)
#define IS_VIEW(s) (s->type & VIEW)
#define IS_HEAP(s) (s->type & HEAP)
#define IS_SMALL(s) (s->type & SMALL)
#define IS_STATIC(s) (s->type & STATIC)
#define DATA_PTR(s) (IS_SMALL(s) ? (char*) &s->data : s->data)

static string_pool first_pool = { 0 };
static string_pool *last_pool = &first_pool;
static string_control_block empty_string = { .count=1, .type=SMALL | STATIC, .length=0, .data=NULL };

static size_t total_allocations = 0;
static size_t total_allocation_size = 0;
static size_t total_deallocations = 0;
static size_t total_small_string_allocations = 0;
static size_t total_string_view_allocations = 0;

static inline void _str_memclear(void *ptr, size_t sz)
{
    __builtin_memset(ptr, '\0', sz);
}

static inline void _str_memcopy(char *dest, char const* src, size_t num)
{
    __builtin_memcpy(dest, src, num);
}

static inline int _str_memcompare(char const* s1, char const* s2, size_t num)
{
    return __builtin_memcmp(s1, s2, num);
}

static inline char* _str_memalloc(size_t sz)
{
    char* ret = malloc(sz);
    assert(ret != NULL);
    _str_memclear(ret, sz);
    return ret;
}

static inline void _str_memfree(char* str)
{
    if (str)
        free(str);
}

static string _str_find_block()
{
    string_pool *pool = last_pool;
    if (pool->first < 0) {
        pool = &first_pool;
        while (pool && pool->first < 0)
            pool = pool->next;
        if (!pool) {
            pool = (string_pool*) _str_memalloc(sizeof(string_pool));
            last_pool->next = pool;
            last_pool = pool;
        }
    }
    if (pool->first == 0 && pool->strings[0].next == 0) {
        // This is the first time we're here:
        for (int ix = 1; ix < 4096; ix++) {
            pool->strings[ix-1].next = ix;
        }
        pool->strings[4095].next = -1;
        if (getenv("OBELIX_INSPECT_STRING_POOLS"))
            atexit(str_inspect_pools);
    }
    string str = &pool->strings[pool->first];
    assert(IS_AVAILABLE(str));
    assert(str->next < 0 || pool->strings[str->next].type == AVAILABLE);
    pool->first = str->next;
    return str;
}

void str_inspect_pools() {
    int pool_count = 0;
    size_t total_allocated_strings = 0;
    size_t total_allocated = 0;

    for (string_pool *pool = &first_pool; pool; pool = pool->next) {
        pool_count++;
    }
    fprintf(stderr, "\nNumber of string pools: %d\n\n", pool_count);
    pool_count = 0;
    for (string_pool *pool = &first_pool; pool; pool = pool->next) {
        fprintf(stderr, "~~~~~~~~~~~~~~ POOL #%d ~~~~~~~~~~~~~~\n", pool_count);
        int unallocated_string_count = 0;
        if (pool->first < 0) {
            fprintf(stderr, "Pool is FULL\n");
            continue;
        } else {
            int str_ix = pool->first;
            while (str_ix >= 0) {
                str_ix = pool->strings[str_ix].next;
                ++unallocated_string_count;
            }
            fprintf(stderr, "Available slots: %d\n", unallocated_string_count);
            total_allocated_strings += (4096 - unallocated_string_count);
        }
        size_t allocated_by_pool = 0;
        int heap_strings_in_pool = 0;
        int small_strings_in_pool = 0;
        int views_in_pool = 0;
        for (int ix = 0; ix < 4096; ++ix) {
            string s = &pool->strings[ix];
            if (IS_HEAP(s)) {
                ++heap_strings_in_pool;
                allocated_by_pool += s->length + 1;
            }
            if (IS_SMALL(s))
                ++small_strings_in_pool;
            if (IS_VIEW(s))
                ++views_in_pool;

        }
        fprintf(stderr, "Allocated slots: %d\n", 4096 - unallocated_string_count);
        fprintf(stderr, "Heap strings: %d using %zu bytes\n", heap_strings_in_pool, allocated_by_pool);
        fprintf(stderr, "Small strings: %d\n", small_strings_in_pool);
        fprintf(stderr, "String views: %d\n", views_in_pool);
        total_allocated += allocated_by_pool;
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "Leaked slot allocations: %zu\n", total_allocated_strings);
    fprintf(stderr, "Leaked heap usage: %zu bytes\n\n", total_allocated);
    fprintf(stderr, "Total number of strings allocated: %zu\n", total_allocations);
    fprintf(stderr, "Total heap usage: %zu bytes\n", total_allocation_size);
    fprintf(stderr, "Total number of strings deallocated: %zu\n", total_deallocations);
    fprintf(stderr, "Total number of small string allocations: %zu\n", total_small_string_allocations);
    fprintf(stderr, "Total number of string view allocations: %zu\n", total_string_view_allocations);
}

string str_view_for(char const* s)
{
    assert(s);
    ++total_string_view_allocations;
    if (*s == '\0') {
        return &empty_string;
    }
    string str = _str_find_block();
    str->count = 1;
    str->type = VIEW;
    str->length = strlen(s);
    str->data = (char*) s;
    ++total_allocations;
    return str;
}

string str_allocate(char const* s)
{
    assert(s);
    if (*s == '\0') {
        return &empty_string;
    }
    string str = _str_find_block();
    str->count = 1;
    str->length = strlen(s);
    if (str->length <= SMALLSZ) {
        ++total_small_string_allocations;
        str->type = SMALL;
        intptr_t* buffer = (intptr_t*) &str->data;
        *buffer = *(intptr_t*) s;
    } else {
        str->type = HEAP;
        str->data = _str_memalloc(str->length);
        _str_memcopy(str->data, s, str->length);
        total_allocation_size += str->length;
    }
    ++total_allocations;
    return str;
}

static string _str_adopt(char* s, bool free_small_string)
{
    assert(s);
    if (*s == '\0') {
        free(s);
        return &empty_string;
    }
    string str = _str_find_block();
    str->count = 1;
    str->length = strlen(s);
    if (str->length <= SMALLSZ) {
        ++total_small_string_allocations;
        str->type = SMALL;
        intptr_t* buffer = (intptr_t*) &str->data;
        *buffer = *(intptr_t*) s;
        if (free_small_string)
            free(s);
    } else {
        str->type = HEAP;
        str->data = s;
    }
    return str;
}

string str_adopt(char* s)
{
    return _str_adopt(s, true);
}

string str_copy(string s)
{
    if (!IS_STATIC(s))
        s->count++;
    return s;
}

void str_free(string s)
{
    if (IS_STATIC(s))
        return;
    if (--s->count == 0) {
        if (IS_HEAP(s)) _str_memfree(s->data);
        s->type = AVAILABLE;
        s->data = NULL;
        s->length = 0;
        string_pool *pool = &first_pool;
        while (pool && (s < pool->strings || s > (pool->strings + 4096)))
            pool = pool->next;
        assert(pool != NULL);
        s->next = pool->first;
        pool->first = s - pool->strings;
        ++total_deallocations;
    }
}

string str_concat(string s1, string s2)
{
    char *s;
    intptr_t buffer = 0;
    if (s1->length + s2->length > SMALLSZ) {
        s = _str_memalloc(s1->length + s2->length);
    } else {
        s = (char*) &buffer;
    }
    _str_memcopy(s, DATA_PTR(s1), s1->length);
    _str_memcopy(s + s1->length, DATA_PTR(s2), s2->length);
    return _str_adopt(s, false);
}

string str_multiply(string str, uint32_t count)
{
    char *s;
    intptr_t buffer = 0;
    if (str->length * count > SMALLSZ) {
        s = _str_memalloc(str->length * count);
    } else {
        s = (char*) &buffer;
    }
    for (uint32_t ix = 0; ix < count; ++ix) {
        _str_memcopy(s + ix * str->length, DATA_PTR(str), str->length);
    }
    s[str->length * count] = '\0';
    return _str_adopt(s, false);
}

char * str_data(string s)
{
    return DATA_PTR(s);
}

uint32_t str_length(string s)
{
    return s->length;
}

int str_compare(string s1, string s2)
{
    int ret = _str_memcompare(DATA_PTR(s1), DATA_PTR(s2), (s1->length > s2->length) ? s2->length : s1->length);
    if (!ret)
        ret = s1->length - s2->length;
    return ret;
}


string to_string_s(int64_t num, int radix)
{
    char buf[65];
    buf[64] = 0;
    char* ptr = &buf[64];

    int sign = num < 0;
    if (sign)
        num = -num;
    if (radix == 0)
        radix = 10;
    do {
        uint32_t digit = num % radix;
        *(--ptr) = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        num /= radix;
    } while (num > 0);
    if (sign)
        *(--ptr) = '-';
    return str_allocate(ptr);
}

string to_string_u(uint64_t num, int radix)
{
    char buf[65];
    buf[64] = 0;
    char* ptr = &buf[64];

    if (radix == 0)
        radix = 10;
    do {
        ptr--;
        int digit = num % radix;
        *ptr = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        num /= radix;
    } while (num > 0);
    return str_allocate(ptr);
}
