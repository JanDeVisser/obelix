/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <rt/crt.h>

int fputs(int fd, string s)
{
    int ret = write(fd, s.data, s.length);
    if (ret < 0)
        return -errno;
    return ret;
}

int puts(string s)
{
    return fputs(1, s);
}

int eputs(string s)
{
    return fputs(2, s);
}

int putln(string s)
{
    int ret = puts(s);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -errno;
    return ret;
}

int putint(int64_t num)
{
    return puts(to_string_s(num, 10));
}

int puthex(int64_t num)
{
    return puts(to_string_s(num, 16));
}

int cputs(char const* s)
{
    int ret = write(1, s, strlen(s));
    if (ret < 0)
        return -errno;
    return ret;
}

int cputln(char const* s)
{
    int ret = cputs(s);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -errno;
    return ret;
}
