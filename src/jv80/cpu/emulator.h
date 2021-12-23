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

class CPU : public ComponentListener {
public:
    explicit CPU(std::string const&);
    ~CPU() override;
    BackPlane& getSystem() { return m_system; }
    void openImage(std::string const&, word addr = 0, bool writable = true);
    void openImage(FILE*, word addr = 0, bool writable = true);
    ErrorOr<uint16_t, SystemErrorCode> run(bool trace = false, word = 0xFFFF);
    void componentEvent(Component const* sender, int ev) override;

private:
    BackPlane m_system {};
    std::shared_ptr<IOChannel> m_keyboard;
    std::shared_ptr<IOChannel> m_terminal;
    std::list<byte> m_queuedKeys;
    bool m_trace { false };
};

}
