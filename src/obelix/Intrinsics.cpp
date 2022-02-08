/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Intrinsics.h>

namespace Obelix {

static std::unordered_map<std::string, std::shared_ptr<FunctionDecl>> s_intrinsics;

std::shared_ptr<FunctionDecl> register_intrinsic(std::shared_ptr<FunctionDecl> intrinsic)
{
    s_intrinsics[intrinsic->name()] = intrinsic;
    return intrinsic;
}

std::shared_ptr<FunctionDecl> get_intrinsic(std::string name)
{
    if (s_intrinsics.contains(name))
        return s_intrinsics.at(name);
    return nullptr;
}

bool is_intrinsic(std::string name)
{
    return s_intrinsics.contains(name);
}

auto s_allocate = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "allocate", ObelixType::TypePointer },
    Symbols {
        Symbol { "size", ObelixType::TypeInt } }));
auto s_close = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "close", ObelixType::TypeInt },
    Symbols {
        Symbol { "fh", ObelixType::TypeInt } }));
auto s_eputs = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "eputs", ObelixType::TypeInt },
    Symbols {
        Symbol { "s", ObelixType::TypeString } }));
auto s_exit = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "exit", ObelixType::TypeInt },
    Symbols {
        Symbol { "code", ObelixType::TypeInt } }));
auto s_fputs = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "fputs", ObelixType::TypeInt },
    Symbols {
        Symbol { "fd", ObelixType::TypeInt },
        Symbol { "s", ObelixType::TypeString } }));
auto s_fsize = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "fsize", ObelixType::TypeInt },
    Symbols {
        Symbol { "fd", ObelixType::TypeInt } }));
auto s_itoa = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "itoa", ObelixType::TypeInt },
    Symbols {
        Symbol { "n", ObelixType::TypeInt } }));
auto s_open = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "open", ObelixType::TypeInt },
    Symbols {
        Symbol { "path", ObelixType::TypeString },
        Symbol { "flags", ObelixType::TypeInt } }));
auto s_putchar = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "putchar", ObelixType::TypeInt },
    Symbols {
        Symbol { "c", ObelixType::TypeInt } }));
auto s_puts = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "puts", ObelixType::TypeInt },
    Symbols {
        Symbol { "s", ObelixType::TypeString } }));
auto s_read = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "read", ObelixType::TypeInt },
    Symbols {
        Symbol { "fd", ObelixType::TypeInt },
        Symbol { "buffer", ObelixType::TypePointer },
        Symbol { "number_of_bytes", ObelixType::TypeInt } }));
auto s_write = register_intrinsic(std::make_shared<FunctionDecl>(
    Symbol { "write", ObelixType::TypeInt },
    Symbols {
        Symbol { "fd", ObelixType::TypeInt },
        Symbol { "buffer", ObelixType::TypePointer },
        Symbol { "number_of_bytes", ObelixType::TypeInt } }));

}
