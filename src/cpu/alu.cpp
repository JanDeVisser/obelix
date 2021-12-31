/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/Format.h"
#include "alu.h"

namespace Obelix::CPU {

ALU::ALU(int ident, std::shared_ptr<Register> lhs)
    : Register(ident)
    , m_lhs(move(lhs))
{
}

std::string ALU::to_string() const
{
    return format("{01x}. LHS {02x}  {01x}. RHS {02x}", lhs()->address(), lhs()->getValue(), address(), getValue());
}

SystemError ALU::onRisingClockEdge()
{
    if (auto err = Register::onRisingClockEdge(); err.is_error())
        return err;

    if (!bus()->xaddr() && (bus()->getAddress() == address())) {
        bus()->putOnAddrBus(0x0);
        bus()->putOnDataBus(bus()->flags());
    }
    return {};
}

SystemError ALU::onHighClock()
{
    static Operator operators[16] = {
        /* 0x0 ADD */ [](ALU* alu) {
            return alu->getValue() + alu->lhs()->getValue();
        },

        /* 0x1 ADC */ [](ALU* alu) { return alu->getValue()
                                         + alu->lhs()->getValue()
                                         + (byte)(alu->bus()->isSet(SystemBus::C)); },

        /* 0x2 SUB */ [](ALU* alu) { return alu->lhs()->getValue() + ~(alu->getValue()) + 1; },

        /* 0x3 SBB */ [](ALU* alu) { return alu->lhs()->getValue()
                                         + ~(alu->getValue() + +(byte)(alu->bus()->isSet(SystemBus::C))) + 1; },
        /* 0x4 AND */ [](ALU* alu) { return alu->getValue() & alu->lhs()->getValue(); },
        /* 0x5 OR  */ [](ALU* alu) { return alu->getValue() | alu->lhs()->getValue(); },
        /* 0x6 XOR */ [](ALU* alu) { return alu->getValue() ^ alu->lhs()->getValue(); },
        /* 0x7 INC */ [](ALU* alu) { return alu->getValue() + 1; },
        /* 0x8 DEC */ [](ALU* alu) { return alu->getValue() - 1; },
        /* 0x9 NOT */ [](ALU* alu) { return ~(alu->getValue()) & 0x00FF; },
        /* 0xA SHL */ [](ALU* alu) {
      word ret = alu -> getValue() << 1;
      if (alu -> bus() -> isSet(SystemBus::C)) {
        ret |= 0x0001;
      }
      return ret & 0x01FF; },
        /* 0xB SHR */ [](ALU* alu) {
      bool carry = (alu -> getValue() & 0x01) != 0;
      word ret = alu -> getValue() >> 1;
      if (alu -> bus() -> isSet(SystemBus::C)) {
        ret |= 0x0080;
      }
      ret &= 0x00FF;
      if (carry) {
        ret |= 0x0100;
      }
      return ret; },
        /* 0xC */ nullptr,
        /* 0xD */ nullptr,
        /* 0xE CLR */ [](ALU*) { return (byte)0; },
        /* 0xF */ nullptr,
    };

    if (auto err = Register::onHighClock(); err.is_error())
        return err;

    if (bus()->putAddress() == address()) {
        if (!bus()->xdata()) {
            m_operator = operators[bus()->opflags()];
            if (m_operator) {
                auto result = m_operator(this);
                byte val = (byte)(result & 0xFF);
                bus()->clearFlags();
                if ((val & 0x00FF) == 0) {
                    bus()->setFlag(SystemBus::Z);
                }
                if (result & 0x0100) {
                    bus()->setFlag(SystemBus::C);
                }
                this->setOverflow(val);
                m_lhs->setValue(val);
                m_operator = nullptr;
            }
        } else if (!bus()->xaddr()) {
            bus()->setFlags(bus()->readDataBus());
        }
    }
    return {};
}

void ALU::setOverflow(word result)
{
    auto s1 = (bool)(lhs()->getValue() & 0x80);
    auto s2 = (bool)(this->getValue() & 0x80);
    auto sr = (bool)(result & 0x0080);
    if ((bus()->opflags() == ADD) || (bus()->opflags() == ADC)) {
        bus()->setFlag(SystemBus::V, !(s1 ^ s2) & (sr ^ s1));
    } else if ((bus()->opflags() == SUB) || (bus()->opflags() == SBB)) {
        bus()->setFlag(SystemBus::V, (s1 ^ s2) & (sr ^ s1));
    }
}

//
// http://teaching.idallen.com/dat2343/10f/notes/040_overflow.txt
//
// Overflow can only happen when adding two numbers of the same sign and
// getting a different sign.  So, to detect overflow we don't care about
// any bits except the sign bits.  Ignore the other bits.
//
// With two operands and one result, we have three sign bits (each 1 or
// 0) to consider, so we have exactly 2**3=8 possible combinations of the
// three bits.  Only two of those 8 possible cases are considered overflow.
// Below are just the sign bits of the two addition operands and result:
//
// ADDITION SIGN BITS
//      num1sign num2sign sumsign
//      -------------------------
//        0          0        0
// *OVER* 0          0        1 (adding two positives should be positive)
//        0          1        0
//        0          1        1
//        1          0        0
//        1          0        1
// *OVER* 1          1        0 (adding two negatives should be negative)
//        1          1        1
//
// We can repeat the same table for subtraction.  Note that subtracting
// a positive number is the same as adding a negative, so the conditions that
// trigger the overflow flag are:
//
// SUBTRACTION SIGN BITS
//      num1sign num2sign sumsign
//     ---------------------------
//         0        0        0
//         0        0        1
//         0        1        0
//  *OVER* 0        1        1 (subtracting a negative is the same as adding a positive)
//  *OVER* 1        0        0 (subtracting a positive is the same as adding a negative)
//         1        0        1
//         1        1        0
//         1        1        1
//

}
