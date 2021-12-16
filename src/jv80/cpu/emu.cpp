/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
#include <jv80/cpu/backplane.h>

int main(int argc, char** argv)
{
    auto* system = new Obelix::JV80::CPU::BackPlane();
    system->defaultSetup();
    system->run();
    return 0;
}
