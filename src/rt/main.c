/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include <rt/obelix.h>

extern int obelix_main(int32_t, int8_t**);

void obelix_fatal(token token, char const* msg)
{
    fprintf(stderr, "%s:%d:%d: Runtime error: %s\n", token.file_name, token.line_start, token.column_start, msg);
    exit(-1);
}

int main(int argc, char** argv)
{
    return obelix_main((int32_t) argc, (int8_t **) argv);
}
