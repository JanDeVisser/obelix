/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "ConnectedComponent.h"

namespace Obelix::CPU {

class AddressRegister : public ConnectedComponent {
public:
    AddressRegister(int, std::string, bool = true);
    void setValue(word val);
    [[nodiscard]] int getValue() const override { return m_value; }
    [[nodiscard]] std::string to_string() const override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;

private:
    word m_value = 0;
    bool m_xdata { false };
};

}
