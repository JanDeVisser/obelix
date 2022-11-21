/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/transpile/c/CTranspilerContext.h>

namespace Obelix {

ErrorOr<void, SyntaxError> open_header(CTranspilerContext& ctx, std::string main_module)
{
    return ctx.call_on_root<ErrorOr<void, SyntaxError>>([&main_module](CTranspilerContext& ctx) -> ErrorOr<void, SyntaxError> {
        return ctx().open_header(main_module);
    });
}

ErrorOr<void, SyntaxError> open_output_file(CTranspilerContext& ctx, std::string name)
{
    return ctx.call_on_root<ErrorOr<void, SyntaxError>>([&name](CTranspilerContext& ctx) -> ErrorOr<void, SyntaxError> {
        return ctx().open_output_file(name);
    });
}

std::vector<COutputFile const*> files(CTranspilerContext& ctx)
{
    return ctx.call_on_root<std::vector<COutputFile const*>>([](CTranspilerContext& ctx) -> std::vector<COutputFile const*> {
        return ctx().files();
    });
}

ErrorOr<void, SyntaxError> flush(CTranspilerContext& ctx)
{
    return ctx.call_on_root<ErrorOr<void, SyntaxError>>([](CTranspilerContext& ctx) -> ErrorOr<void, SyntaxError> {
        return ctx().flush();
    });
}

void writeln(CTranspilerContext& ctx, std::string const& text)
{
    ctx.call_on_root([&text](CTranspilerContext& ctx) -> void {
        ctx().writeln(text);
    });
}

void write(CTranspilerContext& ctx, std::string const& text)
{
    ctx.call_on_root([&text](CTranspilerContext& ctx) -> void {
        ctx().write(text);
    });
}

void indent(CTranspilerContext& ctx)
{
    ctx.call_on_root([](CTranspilerContext& ctx) -> void {
        ctx().indent();
    });
}

void dedent(CTranspilerContext& ctx)
{
    ctx.call_on_root([](CTranspilerContext& ctx) -> void {
        ctx().dedent();
    });
}

}
