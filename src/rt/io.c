/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "obelix.h"

typedef int errno;

// FIXME Find a way to not have to specify these structs here.
typedef struct u32_errno {
  bool success;
  union {
    uint32_t value;
    errno error;
  };
} u32_errno;

typedef struct _bool_errno {
  bool success;
  union {
    bool value;
    errno error;
  };
} bool_errno;

typedef struct u64_errno {
  bool success;
  union {
    uint64_t value;
    errno error;
  };
} u64_errno;

u32_errno $open(string path, uint32_t flags)
{
    int fh = open(str_data(path), flags);
    u32_errno ret;

    if ((ret.success = (fh >= 0))) {
        ret.value = fh;
    } else {
        ret.error = _stdlib_errno;
    }
    return ret;
}

bool_errno $close(uint32_t fh)
{
    bool_errno ret;
    if ((ret.success = (close(fh) >= 0))) {
        ret.value = true;
    } else {
        ret.error = _stdlib_errno;
    }
    return ret;
}

u64_errno $read(uint32_t fh, char* buffer, uint64_t bytes)
{
    u64_errno ret;
    ssize_t bytes_read = read(fh, buffer, bytes);
    if ((ret.success = (bytes_read >= 0))) {
        ret.value = bytes_read;
    } else {
        ret.error = _stdlib_errno;
    }
    return ret;
}

u64_errno $write(uint32_t fh, char const* buffer, uint64_t bytes)
{
    u64_errno ret;
    ssize_t bytes_written = write(fh, buffer, bytes);
    if ((ret.success = (bytes_written >= 0))) {
        ret.value = bytes_written;
    } else {
        ret.error = _stdlib_errno;
    }
    return ret;
}
