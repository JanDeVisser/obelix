/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <rt/crt.h>

extern int obelix_main(int32_t, int8_t**);

int main(int argc, char** argv)
{
    return obelix_main((int32_t) argc, (int8_t **) argv);
}
