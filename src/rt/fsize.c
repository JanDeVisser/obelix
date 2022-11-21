/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/stat.h>

#include <rt/obelix.h>

int64_t fsize(int fd)
{
    struct stat sb;

    if (fstat(fd, &sb) < 0) {
        return -errno;
    }
    return sb.st_size;
}
