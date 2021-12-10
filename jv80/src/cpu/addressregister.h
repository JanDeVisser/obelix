/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>
#include <utility>

#include <cpu/systembus.h>

namespace Obelix::JV80::CPU {

class AddressRegister : public ConnectedComponent {
private:
    word value = 0;

public:
    AddressRegister(int, std::string);
    void setValue(word val);
    int getValue() const override { return value; }
    std::ostream& status(std::ostream&) override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
};

}
