/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include "register.h"

namespace Obelix::CPU {

class ALU;

using Operator = std::function<word(ALU*)>;

class ALU : public Register {
public:
    enum Operations {
        ADD = 0x00,
        ADC = 0x01,
        SUB = 0x02,
        SBB = 0x03,
        AND = 0x04,
        OR = 0x05,
        XOR = 0x06,
        INC = 0x07,
        DEC = 0x08,
        NOT = 0x09,
        SHL = 0x0A,
        SHR = 0x0B,
        CLR = 0x0E,
        CMP = 0x0F,
    };

    ALU(int, std::shared_ptr<Register> lhs);
    [[nodiscard]] std::shared_ptr<Register> const& lhs() const { return m_lhs; }

    [[nodiscard]] std::string to_string() const override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;

private:
    void setOverflow(word);

    Operator m_operator = nullptr;
    std::shared_ptr<Register> m_lhs;
};

}
