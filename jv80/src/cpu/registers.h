/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Obelix::JV80::CPU {

constexpr static int GP_A = 0;
constexpr static int GP_B = 1;
constexpr static int GP_C = 2;
constexpr static int GP_D = 3;
constexpr static int LHS = 4;
constexpr static int RHS = 5;
constexpr static int IR = 6;
constexpr static int MEM = 7;
constexpr static int PC = 8;
constexpr static int SP = 9;
constexpr static int SI = 10;
constexpr static int DI = 11;
constexpr static int TX = 12;
constexpr static int CONTROLLER = 13;
constexpr static int DEREFCONTROLLER = 14;
constexpr static int MEMADDR = 15;

}
