/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdio>
#include <deque>
#include <list>
#include <mutex>
#include <sstream>
#include <string>

#include <jv80/cpu/backplane.h>
#include <jv80/cpu/iochannel.h>

namespace Obelix::JV80::CPU {

using namespace Obelix::JV80::CPU;

class CPU {
public:
    explicit CPU(std::string const&);
    BackPlane* getSystem() { return m_system; }
    void openImage(std::string const&, word addr = 0, bool writable = true);
    void openImage(FILE*, word addr = 0, bool writable = true);
    uint16_t run(word = 0xFFFF);

private:
    BackPlane* m_system;
    IOChannel* m_keyboard;
    IOChannel* m_terminal;
    std::stringstream m_status {};
    std::list<byte> m_queuedKeys;
};

}
