/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Intrinsics.h>
#include <obelix/transpile/c/CTranspilerIntrinsics.h>

namespace Obelix {

extern_logging_category(c_transpiler);

static std::array<CTranspilerFunctionType, IntrinsicType::count> s_intrinsics = {};

bool register_c_transpiler_intrinsic(IntrinsicType type, CTranspilerFunctionType intrinsic)
{
    s_intrinsics[type] = std::move(intrinsic);
    return true;
}

CTranspilerFunctionType const& get_c_transpiler_intrinsic(IntrinsicType type)
{
    assert(type > IntrinsicType::NotIntrinsic && type < IntrinsicType::count);
    return s_intrinsics[type];
}

#define INTRINSIC(intrinsic)                                                                            \
    ErrorOr<void, SyntaxError> c_transpiler_intrinsic_##intrinsic(CTranspilerContext& ctx);                          \
    auto s_c_transpiler_##intrinsic##_decl = register_c_transpiler_intrinsic(intrinsic, c_transpiler_intrinsic_##intrinsic); \
    ErrorOr<void, SyntaxError> c_transpiler_intrinsic_##intrinsic(CTranspilerContext& ctx)

#define INTRINSIC_ALIAS(intrinsic, alias) \
    auto s_c_transpiler_##intrinsic##_decl = register_c_transpiler_intrinsic(intrinsic, c_transpiler_intrinsic_##alias); \

INTRINSIC(allocate)
{
    ctx.writeln("malloc(arg0);");
    return {};
}

INTRINSIC(eputs)
{
    ctx.writeln("write(2,arg1,arg0);");
    return {};
}

INTRINSIC(fputs)
{
    ctx.writeln("write(arg0,arg2,arg1);");
    return {};
}

INTRINSIC(int_to_string)
{
    fatal("Not implemented");
}

INTRINSIC(putchar)
{
    ctx.writeln(
R"(
uint8_t ch = (uint8_t) arg0;
write(1,&ch,1);)");
    return {};
}

INTRINSIC(ptr_math)
{
    ctx.writeln("((void*) arg0) + arg1;");
    return {};
}

INTRINSIC(dereference)
{
    ctx.writeln("*arg0;");
    return {};
}

INTRINSIC(add_int_int)
{
    ctx.write("arg0 + arg1");
    return {};
}

INTRINSIC(subtract_int_int)
{
    ctx.write("arg0 - arg1");
    return {};
}

INTRINSIC(multiply_int_int)
{
    ctx.write("arg0 * arg1");
    return {};
}

INTRINSIC(divide_int_int)
{
    ctx.write("arg0 / arg1");
    return {};
}

INTRINSIC(equals_int_int)
{
    ctx.write("arg0 == arg1");
    return {};
}

INTRINSIC(greater_int_int)
{
    ctx.write("arg0 > arg1");
    return {};
}

INTRINSIC(less_int_int)
{
    ctx.write("arg0 < arg1");
    return {};
}

INTRINSIC(negate_s64)
{
    ctx.write("-arg0");
    return {};
}

INTRINSIC(negate_s32)
{
    ctx.write("-arg0");
    return {};
}

INTRINSIC(negate_s16)
{
    ctx.write("-arg0");
    return {};
}

INTRINSIC(negate_s8)
{
    ctx.write("-arg0");
    return {};
}

INTRINSIC(invert_int)
{
    ctx.write("~arg0");
    return {};
}

INTRINSIC(invert_bool)
{
    ctx.write("!arg0");
    return {};
}

INTRINSIC(and_bool_bool)
{
    ctx.write("arg0 && arg1");
    return {};
}

INTRINSIC(or_bool_bool)
{
    ctx.write("arg0 || arg1");
    return {};
}

INTRINSIC(xor_bool_bool)
{
    ctx.write("arg0 ^ arg1");
    return {};
}

INTRINSIC(equals_bool_bool)
{
    ctx.write("arg0 == arg1");
    return {};
}

INTRINSIC(add_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(greater_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(less_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(equals_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(multiply_str_int)
{
    fatal("Not implemented");
}

}
