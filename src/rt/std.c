/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "obelix.h"

typedef int errno;

typedef struct _voidptr_errno {
  bool success;
  union {
    uint8_t* value;
    errno error;
  };
} voidptr_errno;
