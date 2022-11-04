/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <rt/obelix.h>

int obl_fputs(int fd, string s)
{
    char *ptr = (char *) s.data;
    size_t len = s.length;
    if (!ptr) {
        ptr = "[[null]]";
        len = strlen(ptr);
    }
    int ret = write(fd, ptr, len);
    if (ret < 0)
        return -errno;
    return ret;
}

int obl_puts(string s)
{
    return obl_fputs(1, s);
}

int obl_eputs(string s)
{
    return obl_fputs(2, s);
}

int putln(string s)
{
    int ret = obl_puts(s);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -errno;
    return ret;
}

int putln_u(uint64_t i)
{
    int ret = putint(i);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -errno;
    return ret;
}

int putln_s(int64_t i)
{
    int ret = putint(i);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -errno;
    return ret;
}

int putint(int64_t num)
{
    return obl_puts(to_string_s(num, 10));
}

int puthex(int64_t num)
{
    return obl_puts(to_string_s(num, 16));
}

int cputs(char *s)
{
    int ret = write(1, s, strlen(s));
    if (ret < 0)
        return -errno;
    return ret;
}

int cputln(int8_t *s)
{
    int ret = cputs((char *) s);
    if (ret < 0)
        return ret;
    ret = write(1, "\n", 1);
    if (ret < 0)
        return -errno;
    return ret;
}
