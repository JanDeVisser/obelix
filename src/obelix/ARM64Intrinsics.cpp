/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <sys/stat.h>

#include <obelix/ARM64Intrinsics.h>

namespace Obelix {

using Intrinsic = std::function<ErrorOr<void>(ARM64Context&)>;

static std::unordered_map<std::string, Intrinsic> s_intrinsics = {};

auto register_instrinsic_impl(std::string const& name, Intrinsic function)
{
    s_intrinsics[name] = function;
    return function;
}

Intrinsic get_intrinsic_impl(std::string const& name)
{
    if (s_intrinsics.contains(name))
        return s_intrinsics.at(name);
    fatal("Unknown intrinsic '{}'", name);
}

#define INTRINSIC(intrinsic)                                                     \
    static ErrorOr<void> intrinsic(ARM64Context&);                               \
    static auto s_##intrinsic = register_instrinsic_impl(#intrinsic, intrinsic); \
    ErrorOr<void> intrinsic(ARM64Context& ctx)

#define INTRINSIC_ALIAS(intrinsic, alias) \
    static auto s_##intrinsic = register_instrinsic_impl(#intrinsic, alias);

// This is a mmap syscall
INTRINSIC(allocate)
{
    if (ctx.get_register() != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register());
    ctx.assembly().add_instruction("mov", "x0,xzr");
    ctx.assembly().add_instruction("mov", "w2,#3");
    ctx.assembly().add_instruction("mov", "w3,#0x1002");
    ctx.assembly().add_instruction("mov", "w4,#-1");
    ctx.assembly().add_instruction("mov", "x5,xzr");
    ctx.assembly().syscall(0xC5);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(close)
{
    if (ctx.get_register(0) != 0)
        ctx.assembly().add_instruction("mov", "x0,x{}", ctx.get_register(0));
    ctx.assembly().syscall(0x06);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(eputs)
{
    if (ctx.get_register() != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register());
    if (ctx.get_register(1) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(1));
    ctx.assembly().add_instruction("mov", "x0,#2");
    ctx.assembly().syscall(0x04);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(exit)
{
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x0,x{}", ctx.get_register());
    ctx.assembly().syscall(0x01);
    return {};
}

INTRINSIC(fputs)
{
    if (ctx.get_register(0) != 0)
        ctx.assembly().add_instruction("mov", "x0,x{}", ctx.get_register(0));
    if (ctx.get_register(1) != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register(1));
    if (ctx.get_register(2) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(2));
    ctx.assembly().syscall(0x04);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(fsize)
{
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x0,x{}", ctx.get_register());
    auto sz = sizeof(struct stat);
    if (sz % 16)
        sz += 16 - (sz % 16);
    ctx.assembly().add_instruction("sub", "sp,sp,#{}", sz);
    ctx.assembly().add_instruction("mov", "x1,sp");
    ctx.assembly().syscall(189);
    ctx.assembly().add_instruction("cmp", "x0,#0x00");
    auto lbl = format("lbl_{}", Label::reserve_id());
    ctx.assembly().add_instruction("bne", lbl);
    ctx.assembly().add_instruction("ldr", "x{},[sp,#{}]", ctx.get_register(), offsetof(struct stat, st_size));
    ctx.assembly().add_label(lbl);
    ctx.assembly().add_instruction("add", "sp,sp,#{}", sz);
    return {};
}

INTRINSIC(itoa)
{
    if (ctx.get_register(0) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(0));
    ctx.assembly().add_instruction("sub", "sp,sp,32");
    ctx.assembly().add_instruction("mov", "x0,[sp,-32]!");
    ctx.assembly().add_instruction("mov", "x1,#32");
    ctx.assembly().add_instruction("mov", "w3,#10");
    ctx.assembly().add_instruction("bl", "to_string");
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    if (ctx.get_register(1) != 1)
        ctx.assembly().add_instruction("mov", "x{},x1", ctx.get_register(1));
    return {};
}

INTRINSIC(open)
{
    if (ctx.get_register(0) != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register());
    if (ctx.get_register(2) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(2));
    ctx.assembly().syscall(0x05);
    if (ctx.get_register(0) != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(putchar)
{
    ctx.assembly().add_instruction("strb", "w{},[sp,-16]!", ctx.get_register());
    ctx.assembly().add_instruction("mov", "x0,#1"); // x0: stdin
    ctx.assembly().add_instruction("mov", "x1,sp"); // x1: SP
    ctx.assembly().add_instruction("mov", "x2,#1"); // x2: Number of characters
    ctx.assembly().syscall(0x04);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    ctx.assembly().add_instruction("add", "sp,sp,16");
    return {};
}

INTRINSIC(puts)
{
    if (ctx.get_register(1) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(1));
    if (ctx.get_register() != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register());
    ctx.assembly().add_instruction("mov", "x0,#1");
    ctx.assembly().syscall(0x04);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(read)
{
    if (ctx.get_register(0) != 0)
        ctx.assembly().add_instruction("mov", "x0,x{}", ctx.get_register(0));
    if (ctx.get_register(1) != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register(1));
    if (ctx.get_register(2) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(2));
    ctx.assembly().syscall(0x03);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(write)
{
    if (ctx.get_register(0) != 0)
        ctx.assembly().add_instruction("mov", "x0,x{}", ctx.get_register(0));
    if (ctx.get_register(1) != 1)
        ctx.assembly().add_instruction("mov", "x1,x{}", ctx.get_register(1));
    if (ctx.get_register(2) != 2)
        ctx.assembly().add_instruction("mov", "x2,x{}", ctx.get_register(2));
    ctx.assembly().syscall(0x04);
    if (ctx.get_register() != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_register());
    return {};
}

INTRINSIC(add_Int_Int)
{
    ctx.assembly().add_instruction("add", "x{},x{},x{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(subtract_Int_Int)
{
    ctx.assembly().add_instruction("sub", "x{},x{},x{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(multiply_Int_Int)
{
    ctx.assembly().add_instruction("mul", "x{},x{},x{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(divide_Int_Int)
{
    ctx.assembly().add_instruction("sdiv", "x{},x{},x{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(equals_Int_Int)
{
    ctx.assembly().add_instruction("cmp", "x{},x{}", ctx.get_register(0), ctx.get_register(1));
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("bne", set_false);
    ctx.assembly().add_instruction("mov", "w{},#0x01", ctx.get_register(0));
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w{},wzr", ctx.get_register(0));
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(greater_Int_Int)
{
    ctx.assembly().add_instruction("cmp", "x{},x{}", ctx.get_register(0), ctx.get_register(1));
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.le", set_false);
    ctx.assembly().add_instruction("mov", "w{},#0x01", ctx.get_register(0));
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w{},wzr", ctx.get_register(0));
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(less_Int_Int)
{
    ctx.assembly().add_instruction("cmp", "x{},x{}", ctx.get_register(0), ctx.get_register(1));
    auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.lt", set_true);
    ctx.assembly().add_instruction("mov", "w{},wzr", ctx.get_register(0));
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_true);
    ctx.assembly().add_instruction("mov", "w{},#0x01", ctx.get_register(0));
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(negate_Int)
{
    ctx.assembly().add_instruction("neg", "x{},x{}", ctx.get_register(0), ctx.get_register(0));
    return {};
}

INTRINSIC(invert_Int)
{
    ctx.assembly().add_instruction("mvn", "x{},x{}", ctx.get_register(0), ctx.get_register(0));
    return {};
}
INTRINSIC_ALIAS(invert_Unsigned, invert_Int);

INTRINSIC(linvert_Boolean)
{
    ctx.assembly().add_instruction("eor", "w{},w{},#0x01", ctx.get_register(), ctx.get_register()); // a is 0b00000001 (a was true) or 0b00000000 (a was false
    return {};
}

INTRINSIC(land_Boolean_Boolean)
{
    ctx.assembly().add_instruction("and", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}
INTRINSIC_ALIAS(and_Byte_Byte, land_Boolean_Boolean);
INTRINSIC_ALIAS(and_Char_Char, land_Boolean_Boolean);

INTRINSIC(lor_Boolean_Boolean)
{
    ctx.assembly().add_instruction("orr", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}
INTRINSIC_ALIAS(or_Byte_Byte, lor_Boolean_Boolean);
INTRINSIC_ALIAS(or_Char_Char, lor_Boolean_Boolean);

INTRINSIC(xor_Boolean_Boolean)
{
    ctx.assembly().add_instruction("eor", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}
INTRINSIC_ALIAS(xor_Byte_Byte, xor_Boolean_Boolean);
INTRINSIC_ALIAS(xor_Char_Char, xor_Boolean_Boolean);

INTRINSIC(equals_Boolean_Boolean)
{
    ctx.assembly().add_instruction("eor", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1)); // a is 0b00000000 (a == b) or 0b00000001 (a != b
    ctx.assembly().add_instruction("eor", "w{},w{},#0x01", ctx.get_register(0), ctx.get_register(0));                    // a is 0b00000001 (a == b) or 0b00000000 (a != b
    return {};
}

INTRINSIC(negate_Byte)
{
    ctx.assembly().add_instruction("neg", "w{},w{}", ctx.get_register(0), ctx.get_register(0));
    return {};
}

INTRINSIC(invert_Byte)
{
    ctx.assembly().add_instruction("mvn", "w{},w{}", ctx.get_register(0), ctx.get_register(0));
    return {};
}
INTRINSIC_ALIAS(invert_Char, invert_Byte);

INTRINSIC(add_Byte_Byte)
{
    ctx.assembly().add_instruction("add", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(subtract_Byte_Byte)
{
    ctx.assembly().add_instruction("sub", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(multiply_Byte_Byte)
{
    ctx.assembly().add_instruction("smull", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(divide_Byte_Byte)
{
    ctx.assembly().add_instruction("sdiv", "w{},w{},w{}", ctx.get_register(0), ctx.get_register(0), ctx.get_register(1));
    return {};
}

INTRINSIC(equals_Byte_Byte)
{
    ctx.assembly().add_instruction("cmp", "w{},w{}", ctx.get_register(0), ctx.get_register(1));
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("bne", set_false);
    ctx.assembly().add_instruction("mov", "w{},#0x01", ctx.get_register(0));
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w{},wzr", ctx.get_register(0));
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(greater_Byte_Byte)
{
    ctx.assembly().add_instruction("cmp", "w{},w{}", ctx.get_register(0), ctx.get_register(1));
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.le", set_false);
    ctx.assembly().add_instruction("mov", "w{},#0x01", ctx.get_register(0));
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w{},wzr", ctx.get_register(0));
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(less_Byte_Byte)
{
    ctx.assembly().add_instruction("cmp", "w{},w{}", ctx.get_register(0), ctx.get_register(1));
    auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.lt", set_true);
    ctx.assembly().add_instruction("mov", "w{},wzr", ctx.get_register(0));
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_true);
    ctx.assembly().add_instruction("mov", "w{},#0x01", ctx.get_register(0));
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(add_String_String)
{
    auto lhs_orig = ctx.get_register();
    auto lhs = lhs_orig;
    auto lhs_len_orig = ctx.get_register(1);
    auto lhs_len = lhs_len_orig;
    auto rhs_orig = ctx.get_register(2);
    auto rhs = rhs_orig;
    auto rhs_len_orig = ctx.get_register(3);
    auto rhs_len = rhs_len_orig;
    if (lhs <= 5) {
        lhs = ctx.temporary_register();
        ctx.assembly().add_instruction("mov", "x{},x{}", lhs, lhs_orig);
    }
    if (lhs_len <= 5) {
        lhs_len = ctx.temporary_register();
        ctx.assembly().add_instruction("mov", "w{},w{}", lhs_len, lhs_len_orig);
    }
    if (rhs <= 5) {
        rhs = ctx.temporary_register();
        ctx.assembly().add_instruction("mov", "x{},x{}", rhs, rhs_orig);
    }
    if (rhs_len <= 5) {
        rhs_len = ctx.temporary_register();
        ctx.assembly().add_instruction("mov", "w{},w{}", rhs_len, rhs_len_orig);
    }
    ctx.assembly().add_instruction("mov", "x0,x{}", lhs);
    ctx.assembly().add_instruction("add", "w1,w{},w{}", lhs_len, rhs_len);
    ctx.assembly().add_instruction("bl", "string_alloc");
    ctx.assembly().add_instruction("mov", "w1,w{}", lhs_len);
    ctx.assembly().add_instruction("mov", "x2,x{}", rhs);
    ctx.assembly().add_instruction("mov", "w3,w{}", rhs_len);
    ctx.assembly().add_instruction("bl", "string_concat");
    if (lhs_orig != 0)
        ctx.assembly().add_instruction("mov", "x{},x0", lhs_orig);
    ctx.assembly().add_instruction("add", "w{},w{},w{}", lhs_len_orig, lhs_len, rhs_len);
    return {};
}

}
