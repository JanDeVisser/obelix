/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtest/gtest.h>
#include <iostream>

#include <jv80/cpu/alu.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/harness.h>
#include <jv80/cpu/iochannel.h>
#include <jv80/cpu/memory.h>

#include <jv80/cpu/microcode.h>

namespace Obelix::JV80::CPU {

constexpr word RAM_START = 0x2000;
constexpr word RAM_SIZE = 0x2000;
constexpr word ROM_START = 0x8000;
constexpr word ROM_SIZE = 0x2000;
constexpr word START_VECTOR = ROM_START;
constexpr word RAM_VECTOR = RAM_START;
constexpr int CHANNEL_IN = 0x3;
constexpr int CHANNEL_OUT = 0x5;

class TESTNAME : public ::testing::Test
    , public ComponentListener {
protected:
    Harness* system = nullptr;
    Memory* mem = new Memory(RAM_START, RAM_SIZE, ROM_START, ROM_SIZE);
    Controller* c = new Controller(mc);
    Register* gp_a = new Register(0x0);
    Register* gp_b = new Register(0x1);
    Register* gp_c = new Register(0x2);
    Register* gp_d = new Register(0x3);
    AddressRegister* pc = new AddressRegister(PC, "PC");
    AddressRegister* tx = new AddressRegister(TX, "TX");
    AddressRegister* sp = new AddressRegister(SP, "SP");
    AddressRegister* si = new AddressRegister(SI, "Si");
    AddressRegister* di = new AddressRegister(DI, "Di");
    ALU* alu = new ALU(RHS, new Register(LHS));

    IOChannel* channelIn = nullptr;
    IOChannel* channelOut = nullptr;
    byte inValue;
    byte outValue;
    word nmiAt = 0xFFFF;
    bool nmiHit = false;

    void SetUp() override
    {
        system = new Harness();
        system->insert(mem);
        system->insert(c);
        system->insert(gp_a);
        system->insert(gp_b);
        system->insert(gp_c);
        system->insert(gp_d);
        system->insert(pc);
        system->insert(tx);
        system->insert(sp);
        system->insert(si);
        system->insert(di);
        system->insert(alu);
        system->insert(alu->lhs());

        channelIn = new IOChannel(CHANNEL_IN, "IN", [this]() {
            return inValue;
        });
        channelOut = new IOChannel(CHANNEL_OUT, "OUT", [this](byte v) {
            outValue = v;
        });
        system->insertIO(channelIn);
        system->insertIO(channelOut);

        c->setListener(this);
        //    system -> printStatus = true;
    }

    void TearDown() override
    {
        delete system;
    }

    void componentEvent(Component const* sender, int ev) override
    {
        if (ev == Controller::EV_AFTERINSTRUCTION) {
            if (system->printStatus) {
                system->status_message("After Instruction", 0);
            }
            if (nmiAt == pc->getValue()) {
                nmiHit = true;
            } else if (nmiHit) {
                system->bus().setNmi();
                nmiHit = false;
            }
        }
    }
};

extern const byte unary_op[];
extern const byte binary_op[];

}
