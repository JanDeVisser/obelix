/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define STRING_IMPL

typedef struct _string_control_block {
    union {
        int32_t count;
        int32_t next;
    };
    bool is_view;
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

static string_pool first_pool = { 0 };
static string_pool *last_pool = &first_pool;

static string _str_find_block()
{
    string_pool *pool = last_pool;
    if (pool->first <= 0) {
        pool = &first_pool;
        while (pool && pool->first < 0)
            pool = pool->next;
        if (!pool) {
            pool = (string_pool*) malloc(sizeof(string_pool));
            assert(pool != NULL);
            memset(pool, '\0', sizeof(string_pool));
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
    }
    string str = &pool->strings[pool->first];
    pool->first = str->next;
    return str;
}

string str_view_for(char const* s)
{
    string str = _str_find_block();
    str->count = 1;
    str->is_view = true;
    str->length = strlen(s);
    str->data = (char*) s;
    return str;
}

string str_allocate(char const* s)
{
    string str = _str_find_block();
    str->count = 1;
    str->is_view = false;
    str->length = strlen(s);
    str->data = strdup(s);
    assert(str->data);
    return str;
}

string str_adopt(char* s)
{
    string str = _str_find_block();
    str->count = 1;
    str->is_view = false;
    str->length = strlen(s);
    str->data = s;
    return str;
}

string str_copy(string s)
{
    s->count++;
    return s;
}

void str_free(string s)
{
    if (--s->count == 0) {
        if (!s->is_view) free(s->data);
        s->data = NULL;
        s->length = 0;
        string_pool *pool = &first_pool;
        while (pool && (s < pool->strings || s > (pool->strings + 4096)))
            pool = pool->next;
        assert(pool != NULL);
        s->next = pool->first;
        pool->first = s - pool->strings;
    }
}

string str_concat(string s1, string s2)
{
    char *s = (char*) malloc(s1->length + s2->length + 1);
    assert(s != NULL);
    strncpy(s, s1->data, s1->length);
    strncpy(s + s1->length, s2->data, s2->length);
    s[s1->length + s2->length] = '\0';
    return str_adopt(s);
}

string str_multiply(string str, uint32_t count)
{
    char *s = (char*) malloc(str->length * count + 1);
    assert(s != NULL);
    for (uint32_t ix = 0; ix < count; ++ix) {
        strncpy(s + ix * str->length, str->data, str->length);
    }
    s[str->length * count] = '\0';
    return str_adopt(s);
}

char * str_data(string s)
{
    return s->data;
}

uint32_t str_length(string s)
{
    return s->length;
}

int str_compare(string s1, string s2)
{
    return strcmp(s1->data, s1->data);
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
