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

std::string ARM64Context::contexts() const
{
    auto ret = format("Depth: {} Available: {}", m_register_contexts.size(), m_available_registers);
    for (auto ix = m_register_contexts.size() - 1; (int)ix >= 0; ix--) {
        ret += format("\n{02d} {}", ix, m_register_contexts[ix].to_string());
    }
    return ret;
}

void ARM64Context::new_targeted_context()
{
    m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Targeted);
    debug(parser, "New targeted context:\n{}", contexts());
}

void ARM64Context::new_inherited_context()
{
    assert(!m_register_contexts.empty());
    auto& prev_ctx = m_register_contexts.back();
    auto t = prev_ctx.targeted;
    m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Inherited);
    auto& reg_ctx = m_register_contexts.back();
    reg_ctx.targeted |= t;
    debug(parser, "New inherited context:\n{}", contexts());
}

void ARM64Context::new_temporary_context()
{
    m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Temporary);
    debug(parser, "New temporary context:\n{}", contexts());
}

void ARM64Context::new_enclosing_context()
{
    m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Enclosing);
    auto& reg_ctx = m_register_contexts.back();
    reg_ctx.m_saved_available_registers = m_available_registers;
    m_available_registers = 0xFFFFFF;
    debug(parser, "New enclosing context:\n{}", contexts());
}

void ARM64Context::release_register_context()
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    auto* prev_ctx_pointer = get_previous_register_context();
    debug(parser, "Releasing register context: {}", reg_ctx.to_string());

    m_available_registers |= reg_ctx.temporary_registers;
    switch (reg_ctx.type) {
    case RegisterContext::RegisterContextType::Enclosing:
        m_available_registers = reg_ctx.m_saved_available_registers;
        if (prev_ctx_pointer) {
            for (auto ix = 0; ix < reg_ctx.targeted.count(); ++ix) {
                int reg = (ix < prev_ctx_pointer->targeted.count()) ? get_target_register(1) : add_target_register(1);
                assembly().add_instruction("mov", "x{},x{}", reg, get_target_register(ix));
            }
        }
        debug(parser, "Released enclosing context:\n{}", contexts());
        return;
    case RegisterContext::RegisterContextType::Targeted:
        if (prev_ctx_pointer) {
            prev_ctx_pointer->rhs_targeted |= reg_ctx.targeted;
            m_available_registers |= reg_ctx.rhs_targeted | reg_ctx.reserved_registers;
        } else {
            m_available_registers |= reg_ctx.targeted | reg_ctx.rhs_targeted | reg_ctx.reserved_registers;
        }
        m_available_registers |= reg_ctx.temporary_registers;
        break;
    case RegisterContext::RegisterContextType::Inherited:
        assert(prev_ctx_pointer);
        prev_ctx_pointer->targeted |= reg_ctx.targeted;
        prev_ctx_pointer->rhs_targeted |= reg_ctx.rhs_targeted;
        prev_ctx_pointer->reserved_registers |= reg_ctx.reserved_registers;
        m_available_registers |= reg_ctx.temporary_registers;
        break;
    case RegisterContext::RegisterContextType::Temporary:
        m_available_registers |= reg_ctx.targeted | reg_ctx.rhs_targeted | reg_ctx.reserved_registers | reg_ctx.temporary_registers;
        break;
    }
    m_register_contexts.pop_back();
    debug(parser, "Released register context:\n{}", contexts());
}

void ARM64Context::release_all()
{
    m_available_registers = 0xFFFFFF;
    m_register_contexts.clear();
    debug(parser, "Released all contexts:\n{}", contexts());
}

size_t ARM64Context::get_target_count() const
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    return reg_ctx.targeted.count();
}

int ARM64Context::get_target_register(size_t ix, int level)
{
    assert(m_register_contexts.size() > level);
    auto& reg_ctx = m_register_contexts[m_register_contexts.size() - level - 1];
    if ((ix > 0) && (ix >= reg_ctx.targeted.count())) {
        fatal("{} >= reg_ctx.targeted.count():\n{}", ix, contexts());
    }
    if (reg_ctx.targeted.count() == 0 && ix == 0) {
        auto reg = add_target_register(level);
        return reg;
    }
    int count = 0;
    for (auto i = 0; i < 19; i++) {
        if (reg_ctx.targeted.test(i) && (count++ == ix))
            return i;
    }
    fatal("Unreachable");
}

size_t ARM64Context::get_rhs_count() const
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    return reg_ctx.targeted.count();
}

int ARM64Context::get_rhs_register(size_t ix)
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    if (ix >= reg_ctx.rhs_targeted.count()) {
        fatal("{} >= reg_ctx.rhs_targeted.count():\n{}", ix, contexts());
    }
    int count = 0;
    for (auto i = 0; i < 19; i++) {
        if (reg_ctx.rhs_targeted.test(i) && (count++ == ix))
            return i;
    }
    fatal("Unreachable");
}

int ARM64Context::add_target_register(int level)
{
    assert(m_register_contexts.size() > level);
    auto& reg_ctx = m_register_contexts[m_register_contexts.size() - level - 1];
    if (reg_ctx.reserved_registers.count()) {
        for (auto ix = 0; ix < reg_ctx.reserved_registers.size(); ix++) {
            if (reg_ctx.reserved_registers.test(ix)) {
                reg_ctx.targeted.set(ix);
                reg_ctx.reserved_registers.reset(ix);
                debug(parser, "Claimed reserved register:\n{}", contexts());
                return ix;
            }
        }
    }
    auto reg = (reg_ctx.type == RegisterContext::RegisterContextType::Temporary) ? claim_temporary_register() : claim_next_target();
    reg_ctx.targeted.set(reg);
    debug(parser, "Claimed target register:\n{}", contexts());
    return reg;
}

int ARM64Context::temporary_register()
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    auto ret = claim_temporary_register();
    reg_ctx.temporary_registers.set(ret);
    debug(parser, "Claimed temp register:\n{}", contexts());
    return ret;
}

void ARM64Context::clear_targeted()
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    m_available_registers |= reg_ctx.targeted;
    reg_ctx.targeted.reset();
    debug(parser, "Cleared targets:\n{}", contexts());
}

void ARM64Context::clear_rhs()
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    m_available_registers |= reg_ctx.rhs_targeted;
    reg_ctx.rhs_targeted.reset();
    debug(parser, "Cleared rhs targets:\n{}", contexts());
}

void ARM64Context::clear_context()
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    m_available_registers |= reg_ctx.rhs_targeted | reg_ctx.targeted | reg_ctx.temporary_registers | reg_ctx.reserved_registers;
    reg_ctx.targeted.reset();
    reg_ctx.rhs_targeted.reset();
    reg_ctx.temporary_registers.reset();
    reg_ctx.reserved_registers.reset();
    debug(parser, "Cleared entire context:\n{}", contexts());
}

void ARM64Context::enter_function(std::shared_ptr<MaterializedFunctionDef> const& func) const
{
    s_function_stack.push_back(func);
    assembly().add_comment(func->declaration()->to_string(0));
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
        assembly().add_instruction("str", "x{},[fp,{}]", reg++, param->offset());
        if (param->type() == ObelixType::TypeString)
            assembly().add_instruction("str", "x{},[fp,{}]", reg++, param->offset() + 8);
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

RegisterContext& ARM64Context::get_current_register_context()
{
    assert(!m_register_contexts.empty());
    return m_register_contexts.back();
}

RegisterContext* ARM64Context::get_previous_register_context()
{
    if (m_register_contexts.size() < 2)
        return nullptr;
    return &m_register_contexts[m_register_contexts.size() - 2];
}

int ARM64Context::claim_temporary_register()
{
    if (m_available_registers.count() == 0) {
        fatal("Registers exhausted");
    }
    for (auto ix = 18; ix >= 0; ix--) {
        if (m_available_registers.test(ix)) {
            m_available_registers.reset(ix);
            return ix;
        }
    }
    fatal("Unreachable");
}

int ARM64Context::claim_next_target()
{
    if (m_available_registers.count() == 0) {
        fatal("Registers exhausted");
    }
    for (auto ix = 0; ix < 19; ix++) {
        if (m_available_registers.test(ix)) {
            m_available_registers.reset(ix);
            return ix;
        }
    }
    fatal("Unreachable");
}

int ARM64Context::claim_register(int reg)
{
    if (!m_available_registers[reg])
        fatal(format("Register {} already claimed", reg).c_str());
    m_available_registers.reset(reg);
    return reg;
}

void ARM64Context::release_register(int reg)
{
    m_available_registers.set(reg);
}

}
