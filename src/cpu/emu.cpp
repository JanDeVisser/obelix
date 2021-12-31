/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>

#include "emulator.h"

void usage(char const* executable_name)
{
    fprintf(stderr, "Usage: %s [--help] [--trace] [<image file>]\n", executable_name);
    fprintf(stderr, "Default image is out.bin\n");
    exit(1);
}

int main(int argc, char** argv)
{
    std::string image = "out.bin";
    bool trace = false;

    auto ix = 1;
    while (ix < argc) {
        if (!strcmp(argv[ix], "--trace")) {
            trace = true;
            ix++;
            continue;
        }
        if (!strcmp(argv[ix], "--help")) {
            usage(argv[0]);
        }
        image = argv[ix++];
    }

    Obelix::CPU::CPU cpu(image);
    if (auto ret = cpu.run(trace); ret.is_error()) {
        fprintf(stderr, "ERROR: %s\n", Obelix::CPU::SystemErrorCode_name(ret.error()));
    } else {
        printf("%d\n", ret.value());
    }
    return 0;
}
