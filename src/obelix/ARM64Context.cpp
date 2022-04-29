/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/ARM64Context.h>

namespace Obelix {

std::vector<std::shared_ptr<MaterializedFunctionDef>> ARM64Context::s_function_stack {};
std::unordered_map<std::string, Assembly> ARM64Context::s_assemblies {};

ARM64Context::ARM64Context(ARM64Context& parent)
    : Context<int>(parent)
    , m_assembly(parent.m_assembly)
{
    auto offset_maybe = get("#offset");
    assert(offset_maybe.has_value());
    declare("#offset", get("#offset").value());
}

ARM64Context::ARM64Context(ARM64Context* parent)
    : Context<int>(parent)
    , m_assembly(parent->m_assembly)
{
    auto offset_maybe = get("#offset");
    assert(offset_maybe.has_value());
    declare("#offset", get("#offset").value());
}

ARM64Context::ARM64Context()
    : Context()
    , m_assembly()
{
    add_module(ROOT_MODULE_NAME);
    declare("#offset", make_obj<Integer>(0));
}

void ARM64Context::enter_function(std::shared_ptr<MaterializedFunctionDef> const& func) const
{
    s_function_stack.push_back(func);
    assembly().add_comment(func->declaration()->to_string());
    assembly().add_directive(".global", func->name());
    assembly().add_label(func->name());

    // Save fp and lr:
    int depth = func->stack_depth();
    if (depth % 16) {
        depth += (16 - (depth % 16));
    }
    assembly().add_instruction("stp", "fp,lr,[sp,#-{}]!", depth);

    // Set fp to the current sp. Now a return is setting sp back to fp,
    // and popping lr followed by ret.
    assembly().add_instruction("mov", "fp,sp");

    // Copy parameters from registers to their spot in the stack.
    // @improve Do this lazily, i.e. when we need the registers
    auto reg = 0;
    for (auto& param : func->declaration()->parameters()) {
        assembly().add_instruction("str", "x{},[fp,#{}]", reg++, param->offset());
#if 0
        if (param->type()->type() == PrimitiveType::String)
            assembly().add_instruction("str", "x{},[fp,#{}]", reg++, param->offset() + 8);
#endif
    }
}

void ARM64Context::function_return() const
{
    assert(!s_function_stack.empty());
    auto func_def = s_function_stack.back();
    assembly().add_instruction("b", format("__{}_return", func_def->name()));
}

void ARM64Context::leave_function() const
{
    assert(!s_function_stack.empty());
    auto func_def = s_function_stack.back();
    assembly().add_label(format("__{}_return", func_def->name()));
    int depth = func_def->stack_depth();
    if (depth % 16) {
        depth += (16 - (depth % 16));
    }
    assembly().add_instruction("ldp", "fp,lr,[sp],#{}", depth);
    assembly().add_instruction("ret");
    s_function_stack.pop_back();
}

void ARM64Context::reserve_on_stack(size_t bytes)
{
    if (bytes % 16)
        bytes = bytes + (16 - (bytes % 16));
    assembly().add_instruction("sub", "sp,sp,#{}", bytes);
    m_stack_allocated += bytes;
}

void ARM64Context::release_stack()
{
    assembly().add_instruction("add", "sp,sp,#{}", m_stack_allocated);
    m_stack_allocated = 0;
}


}
