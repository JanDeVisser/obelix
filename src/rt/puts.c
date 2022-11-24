/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <unistd.h>

#include <rt/obelix.h>

int $fputs(int fd, string s)
{
    char *ptr = str_data(s);
    size_t len = str_length(s);
    if (!ptr) {
        ptr = "[[null]]";
        len = strlen(ptr);
    }
    int ret = write(fd, ptr, len);
    if (ret < 0)
        return -_stdlib_errno;
    return ret;
}

int $puts(string s)
{
    return $fputs(1, s);
}

int $eputs(string s)
{
    return $fputs(2, s);
}

int putln(string s)
{
    int ret = $puts(s);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -_stdlib_errno;
    return ret;
}

int putln_u(uint64_t i)
{
    int ret = putint(i);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -_stdlib_errno;
    return ret;
}

int putln_s(int64_t i)
{
    int ret = putint(i);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -_stdlib_errno;
    return ret;
}

int putint(int64_t num)
{
    return $puts(to_string_s(num, 10));
}

int puthex(int64_t num)
{
    return $puts(to_string_s(num, 16));
}

int cputs(char *s)
{
    int ret = write(1, s, strlen(s));
    if (ret < 0)
        return -_stdlib_errno;
    return ret;
}

int cputln(int8_t *s)
{
    int ret = cputs((char *) s);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -_stdlib_errno;
    return ret;
}
