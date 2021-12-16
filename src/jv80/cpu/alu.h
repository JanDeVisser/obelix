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
#include <jv80/cpu/register.h>

namespace Obelix::JV80::CPU {

class ALU;

typedef std::function<word(ALU*)> Operator;

class ALU : public Register {
    Operator m_operator = nullptr;
    Register* m_lhs;

private:
    void setOverflow(word);

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

    ALU(int, Register* lhs);
    Register* lhs() const { return m_lhs; }

    std::ostream& status(std::ostream&) override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
};

}
