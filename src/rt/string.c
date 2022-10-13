/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <rt/crt.h>

string string_alloc(char *s)
{
    string ret;
    ret.length = strlen(s);
    ret.data = malloc(ret.length + 1);
    if (!ret.data) {
        ret.length = -1;
        return ret;
    }
    strcpy((char*) ret.data, s);
    return ret;
}

string to_string_s(int64_t num, int radix)
{
    char buf[65];
    buf[64] = 0;
    char *ptr = &buf[64];

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
    return string_alloc(ptr);
}

string to_string_u(uint64_t num, int radix)
{
    char buf[65];
    buf[64] = 0;
    char *ptr = &buf[64];

    if (radix == 0)
        radix = 10;
     do {
        ptr--;
        int digit = num % radix;
        *ptr = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        num /= radix;
    } while (num > 0);
    return string_alloc(ptr);
}

string string_concat(string s1, string s2)
{
    string ret;
    ret.length = strlen((char const*) s1.data) + strlen((char const*) s2.data);
    ret.data = malloc(ret.length + 1);
    if (!ret.data) {
        ret.length = -1;
        return ret;
    }
    strcpy((char *) ret.data, (char const*) s1.data);
    strcat((char *) ret.data, (char const*) s2.data);
    return ret;
}
