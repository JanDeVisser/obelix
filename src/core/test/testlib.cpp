/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstring>

extern "C" int testlib_variable;
int testlib_variable = 12;

extern "C" size_t testlib_function(const char* message)
{
    return strlen(message);
}
