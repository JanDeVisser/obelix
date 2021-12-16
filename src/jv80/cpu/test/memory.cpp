/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <gtest/gtest.h>

#include <jv80/cpu/harness.h>
#include <jv80/cpu/memory.h>

namespace Obelix::JV80::CPU {

static byte initRam[] = { 0x42, 0x37, 0x55 };
static byte initRom[] = { 0x82, 0x77, 0x95 };

class MemoryTest : public ::testing::Test {
protected:
    Harness* system = nullptr;
    Memory* mem = nullptr;

    void SetUp() override
    {
        mem = new Memory(0x0000, 0x2000, 0x8000, 0x2000);
        mem->add(0x0000, 3, true, initRam);
        mem->add(0x8000, 3, false, initRom);
        system = new Harness(mem);
    }

    void TearDown() override
    {
        delete system;
    }
};

TEST_F(MemoryTest, get)
{
    ASSERT_EQ((*mem)[0], 0x42);
}

TEST_F(MemoryTest, setMemAddress)
{
    SystemError err = system->cycle(true, false, true, 1, Memory::ADDR_ID, 0, 0x01, 0x00);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x0001);
}

TEST_F(MemoryTest, setMemAddressLSB)
{
    mem->setValue(0x5555);
    SystemError err = system->cycle(false, true, true, 1, Memory::ADDR_ID, 0, 0x01, 0x00);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x5501);
}

TEST_F(MemoryTest, setMemAddressMSB)
{
    mem->setValue(0x5555);
    SystemError err = system->cycle(false, true, true, 1, Memory::ADDR_ID, SystemBus::MSB, 0x00, 0x00);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x0055);
}

TEST_F(MemoryTest, setMemAddressLSBAndMSB)
{
    mem->setValue(0x5555);
    SystemError err = system->cycle(false, true, true, 1, Memory::ADDR_ID, 0, 0x01, 0x00);
    ASSERT_EQ(err, NoError);
    err = system->cycle(false, true, true, 1, Memory::ADDR_ID, SystemBus::MSB, 0x00, 0x00);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x0001);
}

TEST_F(MemoryTest, readRAM)
{
    SystemError err = system->cycle(true, false, true, 1, Memory::ADDR_ID, 0, 0x01, 0x00);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x0001);
    err = system->cycle(false, true, Memory::MEM_ID, 1, 0);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(system->bus().readDataBus(), 0x37);
}

TEST_F(MemoryTest, writeRAM)
{
    SystemError err = system->cycle(true, false, true, 1, Memory::ADDR_ID, 0, 0x01, 0x00);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x0001);
    err = system->cycle(false, true, true, 1, Memory::MEM_ID, 0, 0x55);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(system->bus().readDataBus(), 0x55);
}

TEST_F(MemoryTest, readROM)
{
    SystemError err = system->cycle(true, false, true, 1, Memory::ADDR_ID, 0, 0x01, 0x80);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(mem->getValue(), 0x8001);
    err = system->cycle(false, true, Memory::MEM_ID, 1, 0);
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(system->bus().readDataBus(), 0x77);
}

TEST_F(MemoryTest, writeROM)
{
    system->cycle(true, false, true, 1, Memory::ADDR_ID, 0, 0x01, 0x80);
    ASSERT_EQ(mem->getValue(), 0x8001);
    SystemError err = system->cycle(false, true, true, 1, Memory::MEM_ID, 0, 0x55);
    ASSERT_EQ(err, ProtectedMemory);
    ASSERT_EQ((*mem)[0x8001], 0x77);
}

}
