/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtest/gtest.h>

#include "cpu/alu.h"
#include "cpu/controller.h"
#include "cpu/harness.h"
#include "cpu/iochannel.h"
#include "cpu/memory.h"

#include "cpu/microcode.h"

namespace Obelix::CPU {

constexpr word RAM_START = 0x2000;
constexpr word RAM_SIZE = 0x2000;
constexpr word ROM_START = 0x8000;
constexpr word ROM_SIZE = 0x2000;
constexpr word START_VECTOR = ROM_START;
constexpr word RAM_VECTOR = RAM_START;
constexpr int CHANNEL_IN = 0x3;
constexpr int CHANNEL_OUT = 0x5;

class HarnessTest
    : public ::testing::Test
    , public ComponentListener {
public:
    void SetUp() override
    {
        mem = std::make_shared<Memory>(RAM_START, RAM_SIZE, ROM_START, ROM_SIZE);
        c = std::make_shared<Controller>(mc);
        gp_a = std::make_shared<Register>(0x0);
        gp_b = std::make_shared<Register>(0x1);
        gp_c = std::make_shared<Register>(0x2);
        gp_d = std::make_shared<Register>(0x3);

        pc = std::make_shared<AddressRegister>(PC, "PC");
        sp = std::make_shared<AddressRegister>(SP, "SP");
        bp = std::make_shared<AddressRegister>(BP, "BP");
        tx = std::make_shared<AddressRegister>(TX, "TX");
        si = std::make_shared<AddressRegister>(SI, "Si");
        di = std::make_shared<AddressRegister>(DI, "Di");
        alu = std::make_shared<ALU>(RHS, std::make_shared<Register>(LHS, "LHS"));

        system.insert(mem);
        system.insert(c);
        system.insert(gp_a);
        system.insert(gp_b);
        system.insert(gp_c);
        system.insert(gp_d);
        system.insert(pc);
        system.insert(tx);
        system.insert(sp);
        system.insert(bp);
        system.insert(si);
        system.insert(di);
        system.insert(alu);
        system.insert(alu->lhs());
        channelIn = std::make_shared<IOChannel>(CHANNEL_IN, "IN", [this]() {
            return inValue;
        });
        channelOut = std::make_shared<IOChannel>(CHANNEL_OUT, "OUT", [this](byte v) {
            outValue = v;
        });
        system.insertIO(channelIn);
        system.insertIO(channelOut);
        c->setListener(this);
    }

    void componentEvent(Component const* sender, int ev) override
    {
        if (ev == Controller::EV_AFTERINSTRUCTION) {
            if (auto err = system.status_message(c->instruction()); err.is_error())
                fprintf(stderr, "Error in status_message: %s", SystemErrorCode_name(err.error()));
            if (nmiAt == pc->getValue()) {
                nmiHit = true;
            } else if (nmiHit) {
                system.bus().setNmi();
                nmiHit = false;
            }
        }
    }

    void check_cycles(int count)
    {
        auto cycles_or_err = system.run();
        if (cycles_or_err.is_error())
            fprintf(stdout, "system.run() error: %s\n", SystemErrorCode_name(cycles_or_err.error()));
        ASSERT_FALSE(cycles_or_err.is_error());
        ASSERT_EQ(cycles_or_err.value(), count);
        ASSERT_EQ(system.bus().halt(), false);
    }

    void check_memory(word addr, byte value, bool equals = true)
    {
        auto v_or_error = mem->peek(addr);
        ASSERT_FALSE(v_or_error.is_error());
        if (equals)
            ASSERT_EQ(v_or_error.value(), value);
        else
            ASSERT_NE(v_or_error.value(), value);
    }

protected:
    Harness system;
    std::shared_ptr<Memory> mem;
    std::shared_ptr<Controller> c;
    std::shared_ptr<Register> gp_a;
    std::shared_ptr<Register> gp_b;
    std::shared_ptr<Register> gp_c;
    std::shared_ptr<Register> gp_d;
    std::shared_ptr<AddressRegister> pc;
    std::shared_ptr<AddressRegister> tx;
    std::shared_ptr<AddressRegister> sp;
    std::shared_ptr<AddressRegister> bp;
    std::shared_ptr<AddressRegister> si;
    std::shared_ptr<AddressRegister> di;
    std::shared_ptr<ALU> alu;

    std::shared_ptr<IOChannel> channelIn { nullptr };
    std::shared_ptr<IOChannel> channelOut { nullptr };
    byte inValue;
    byte outValue;
    word nmiAt { 0xFFFF };
    bool nmiHit { false };
};

// mov a, #xx      4
// not a           4
// hlt             3
// total          11
constexpr byte unary_op[] = {
    /* 2000 */ MOV_A_IMM,
    0x1F,
    /* 2002 */ NOP,
    /* 2003 */ HLT,
};

// mov a, #xx      4        x2   8
// add a, b        5             5
// hlt             3             3
// total                        16
constexpr byte binary_op[] = {
    /* 2000 */ MOV_A_IMM,
    0x1F,
    /* 2002 */ MOV_B_IMM,
    0xF8,
    /* 2004 */ NOP,
    /* 2005 */ HLT,
};

}
