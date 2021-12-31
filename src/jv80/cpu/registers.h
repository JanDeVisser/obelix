/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Obelix::JV80::CPU {

constexpr static int GP_A = 0x00;
constexpr static int GP_B = 0x01;
constexpr static int GP_C = 0x02;
constexpr static int GP_D = 0x03;
constexpr static int LHS = 0x04;
constexpr static int RHS = 0x05;
constexpr static int IR = 0x06;
constexpr static int MEM = 0x07;

constexpr static int PC = 0x08;
constexpr static int SP = 0x09;
constexpr static int BP = 0x0A;
constexpr static int SI = 0x0B;
constexpr static int DI = 0x0C;
constexpr static int TX = 0x0D;
constexpr static int CONTROLLER = 0x0E;
constexpr static int MEMADDR = 0x0F;

constexpr static int DEREFCONTROLLER = 0x10;

}
