/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>

#include <jv80/cpu/emulator.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image file>.bin\n", argv[0]);
    }
    Obelix::JV80::CPU::CPU cpu(argv[1]);
    printf("%d\n", cpu.run());
    return 0;
}
