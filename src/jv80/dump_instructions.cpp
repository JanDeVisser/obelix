/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstring>
#include <iostream>

#include "alu.h"
#include "controller.h"
#include "src/cpu/microcode.inc"

void cplusplus()
{
    std::cout << "const char * MNEMONIC[256] = {" << std::endl;
    for (int ix = 0; ix < 256; ix++) {
        MicroCode& m = mc[ix];
        if (m.opcode == ix) {
            std::cout << "  \"" << m.instruction << "\"," << std::endl;
        } else {
            std::cout << "  nullptr," << std::endl;
        }
    }
    std::cout << "}" << std::endl;
}

void python()
{
    std::cout << "opcodes = {" << std::endl;
    for (int ix = 0; ix < 256; ix++) {
        MicroCode& m = mc[ix];
        if (m.opcode == ix) {
            std::cout << "  \"" << m.instruction << "\": " << ix << "," << std::endl;
        }
    }
    std::cout << "}" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc > 1 && !strcmp(argv[1], "--python")) {
        python();
    } else {
        cplusplus();
    }
}
