/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
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

#define INTRINSIC(intrinsic)                                                                                                 \
    ErrorOr<void, SyntaxError> c_transpiler_intrinsic_##intrinsic(CTranspilerContext&, ObjectTypes const&);                  \
    auto s_c_transpiler_##intrinsic##_decl = register_c_transpiler_intrinsic(intrinsic, c_transpiler_intrinsic_##intrinsic); \
    ErrorOr<void, SyntaxError> c_transpiler_intrinsic_##intrinsic(CTranspilerContext& ctx, ObjectTypes const& types)

#define INTRINSIC_ALIAS(intrinsic, alias) \
    auto s_c_transpiler_##intrinsic##_decl = register_c_transpiler_intrinsic(intrinsic, c_transpiler_intrinsic_##alias);

INTRINSIC(allocate)
{
    writeln(ctx, "malloc($arg0);");
    return {};
}

INTRINSIC(free)
{
    writeln(ctx, "free($arg0);");
    return {};
}

INTRINSIC(exit)
{
    writeln(ctx, "exit($arg0);");
    return {};
}

INTRINSIC(ok)
{
    writeln(ctx, "$arg0.success");
    return {};
}

INTRINSIC(error)
{
    writeln(ctx, "!$arg0.success");
    return {};
}

INTRINSIC(eputs)
{
    writeln(ctx, "write(2,$arg1,$arg0);");
    return {};
}

INTRINSIC(fputs)
{
    writeln(ctx, "write($arg0,$arg2,$arg1);");
    return {};
}

INTRINSIC(int_to_string)
{
    fatal("Not implemented");
}

INTRINSIC(putchar)
{
    writeln(ctx,
R"(
uint8_t ch = (uint8_t) $arg0;
write(1,&ch,1);)");
    return {};
}

INTRINSIC(ptr_math)
{
    writeln(ctx, "((void*) $arg0) + $arg1;");
    return {};
}

INTRINSIC(dereference)
{
    writeln(ctx, "*$arg0;");
    return {};
}

INTRINSIC(add_int_int)
{
    write(ctx, "$arg0 + $arg1");
    return {};
}

INTRINSIC(subtract_int_int)
{
    write(ctx, "$arg0 - $arg1");
    return {};
}

INTRINSIC(multiply_int_int)
{
    write(ctx, "$arg0 * $arg1");
    return {};
}

INTRINSIC(divide_int_int)
{
    write(ctx, "$arg0 / $arg1");
    return {};
}

INTRINSIC(equals_int_int)
{
    write(ctx, "$arg0 == $arg1");
    return {};
}

INTRINSIC(greater_int_int)
{
    write(ctx, "$arg0 > $arg1");
    return {};
}

INTRINSIC(less_int_int)
{
    write(ctx, "$arg0 < $arg1");
    return {};
}

INTRINSIC(negate_s64)
{
    write(ctx, "-$arg0");
    return {};
}

INTRINSIC(negate_s32)
{
    write(ctx, "-$arg0");
    return {};
}

INTRINSIC(negate_s16)
{
    write(ctx, "-$arg0");
    return {};
}

INTRINSIC(negate_s8)
{
    write(ctx, "-$arg0");
    return {};
}

INTRINSIC(invert_int)
{
    write(ctx, "~$arg0");
    return {};
}

INTRINSIC(invert_bool)
{
    write(ctx, "!$arg0");
    return {};
}

INTRINSIC(and_bool_bool)
{
    write(ctx, "$arg0 && $arg1");
    return {};
}

INTRINSIC(or_bool_bool)
{
    write(ctx, "$arg0 || $arg1");
    return {};
}

INTRINSIC(xor_bool_bool)
{
    write(ctx, "$arg0 ^ $arg1");
    return {};
}

INTRINSIC(equals_bool_bool)
{
    write(ctx, "$arg0 == $arg1");
    return {};
}

INTRINSIC(add_str_str)
{
    write(ctx, "str_concat($arg0, $arg1)");
    return {};
}

INTRINSIC(greater_str_str)
{
    write(ctx, "str_compare($arg0, $arg1) > 0");
    return {};
}

INTRINSIC(less_str_str)
{
    write(ctx, "str_compare($arg0, $arg1) < 0");
    return {};
}

INTRINSIC(equals_str_str)
{
    write(ctx, "str_compare($arg0, $arg1) == 0");
    return {};
}

INTRINSIC(multiply_str_int)
{
    write(ctx, "str_multiply($arg0, $arg1)");
    return {};
}

INTRINSIC(enum_text_value)
{
    auto enum_type = types[0];
    writeln(ctx, format(
R"($enum_value v = $get_enum_value($_{}_values, $arg0);
str_view_for(v.text);)", enum_type->name()));
    return {};
}

INTRINSIC(free_str)
{
    writeln(ctx, "str_free($self);");
    return {};
}

}
