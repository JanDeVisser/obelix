/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/ConnectedComponent.h>

namespace Obelix::JV80::CPU {

class Register : public ConnectedComponent {
private:
    byte m_value = 0;
    word m_stash = 0;

public:
    explicit Register(int, std::string&& = "");

    void setValue(byte val);
    [[nodiscard]] int getValue() const override { return m_value; }

    [[nodiscard]] std::string to_string() const override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
};

}
