/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <config.h>
#include <core/Logging.h>
#include <core/ScopeGuard.h>
#include <obelix/Intrinsics.h>
#include <obelix/MacOSX.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

struct Assembly {
    std::string code;
    std::string text;
    std::string data;

    template<typename... Args>
    void add_instruction(std::string const& mnemonic, Args&&... args)
    {
        code = format("{}\t{}\t{}\n", code, mnemonic, std::forward<Args>(args)...);
    }

    void add_instruction(std::string const& mnemonic)
    {
        code = format("{}\t{}\n", code, mnemonic);
    }

    void add_label(std::string const& label)
    {
        code = format("{}{}:\n", code, label);
    }

    void add_directive(std::string const& directive, std::string const& args)
    {
        code = format("{}{}\t{}\n", code, directive, args);
    }

    void add_string(int id, std::string const& str)
    {
        text = format("{}.align 2\nstr_{}:\n\t.string\t\"{}\"\n", text, id, str);
    }

    void add_comment(std::string const& comment)
    {
        auto c = comment;
        for (auto pos = c.find('\n'); pos != std::string::npos; pos = c.find('\n'))
            c[pos] = ' ';
        code = format("{}\n\t; {}\n", code, c);
    }

    void add_data(std::string const& label, std::string const& d)
    {
        if (data.empty())
            data = ".data\n\n";
        data = format("{}\n.align 2\n{}:\t{}", data, label, d);
    }

    void syscall(int id)
    {
        add_instruction("mov", format("x16, #{}", id));
        add_instruction("svc", "#0x00");
    }
};

#define ENUM_REGISTER_CONTEXT_TYPES(S) \
    S(Enclosing)                       \
    S(Targeted)                        \
    S(Inherited)                       \
    S(Temporary)

struct RegisterContext {
    enum class RegisterContextType {
#undef REGISTER_CONTEXT_TYPE
#define REGISTER_CONTEXT_TYPE(type) type,
        ENUM_REGISTER_CONTEXT_TYPES(REGISTER_CONTEXT_TYPE)
#undef REGISTER_CONTEXT_TYPE
    };

    [[nodiscard]] static char const* RegisterContextType_name(RegisterContextType t)
    {
        switch (t) {
#undef REGISTER_CONTEXT_TYPE
#define REGISTER_CONTEXT_TYPE(type) \
    case RegisterContextType::type: \
        return #type;
            ENUM_REGISTER_CONTEXT_TYPES(REGISTER_CONTEXT_TYPE)
#undef REGISTER_CONTEXT_TYPE
        }
    }

    explicit RegisterContext(RegisterContextType context_type = RegisterContextType::Temporary)
        : type(context_type)
    {
    }

    RegisterContextType type { RegisterContextType::Temporary };
    std::bitset<19> targeted { 0x0 };
    std::bitset<19> rhs_targeted { 0x0 };
    std::bitset<19> temporary_registers { 0x0 };
    std::bitset<19> m_saved_available_registers { 0x0 };

    [[nodiscard]] std::string to_string() const
    {
        return format("{} lhs: {} rhs: {}", RegisterContextType_name(type), targeted.to_string(), rhs_targeted.to_string());
    }
};

class MacOSXContext : public Context<int> {
public:
    MacOSXContext(MacOSXContext& parent)
        : Context<int>(parent)
        , m_assembly(parent.assembly())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit MacOSXContext(MacOSXContext* parent)
        : Context<int>(parent)
        , m_assembly(parent->assembly())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit MacOSXContext(Assembly& assembly)
        : Context()
        , m_assembly(assembly)
    {
        declare("#offset", make_obj<Integer>(0));
    }

    [[nodiscard]] Assembly& assembly() const { return m_assembly; }

    [[nodiscard]] std::string contexts() const
    {
        auto ret = format("Depth: {} Available: {}", m_register_contexts.size(), m_available_registers);
        for (auto ix = m_register_contexts.size() - 1; (int)ix >= 0; ix--) {
            ret += format("\n{02d} {}", ix, m_register_contexts[ix].to_string());
        }
        return ret;
    }

    void new_targeted_context()
    {
        m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Targeted);
        debug(parser, "New targeted context:\n{}", contexts());
    }

    void new_inherited_context()
    {
        assert(!m_register_contexts.empty());
        auto& prev_ctx = m_register_contexts.back();
        auto t = prev_ctx.targeted;
        m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Inherited);
        auto& reg_ctx = m_register_contexts.back();
        reg_ctx.targeted |= t;
        debug(parser, "New inherited context:\n{}", contexts());
    }

    void new_enclosing_context();

    void new_temporary_context()
    {
        m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Temporary);
        debug(parser, "New temporary context:\n{}", contexts());
    }

    void release_register_context();

    void release_all()
    {
        m_available_registers = 0xFFFFFF;
        m_register_contexts.clear();
        debug(parser, "Released all contexts:\n{}", contexts());
    }

    [[nodiscard]] size_t get_target_count() const
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        return reg_ctx.targeted.count();
    }

    int get_target_register(size_t ix = 0)
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        if ((ix > 0) && (ix >= reg_ctx.targeted.count())) {
            fatal("{} >= reg_ctx.targeted.count():\n{}", ix, contexts());
        }
        if (reg_ctx.targeted.count() == 0 && ix == 0) {
            auto reg = claim_next_target();
            reg_ctx.targeted.set(reg);
            return reg;
        }
        int count = 0;
        for (auto i = 0; i < 19; i++) {
            if (reg_ctx.targeted.test(i) && (count++ == ix))
                return i;
        }
        fatal("Unreachable");
    }

    [[nodiscard]] size_t get_rhs_count() const
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        return reg_ctx.targeted.count();
    }

    int get_rhs_register(size_t ix = 0)
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

    int add_target_register()
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        auto reg = (reg_ctx.type == RegisterContext::RegisterContextType::Temporary) ? claim_temporary_register() : claim_next_target();
        reg_ctx.targeted.set(reg);
        debug(parser, "Claimed target register:\n{}", contexts());
        return reg;
    }

    int temporary_register()
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        auto ret = claim_temporary_register();
        reg_ctx.temporary_registers.set(ret);
        debug(parser, "Claimed temp register:\n{}", contexts());
        return ret;
    }

    void clear_targeted()
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        m_available_registers |= reg_ctx.targeted;
        reg_ctx.targeted.reset();
        debug(parser, "Cleared targets:\n{}", contexts());
    }

    void clear_rhs()
    {
        ScopeGuard sg([this]() {
            debug(parser, "Clearing RHS:\n{}", contexts());
        });
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        m_available_registers |= reg_ctx.rhs_targeted;
        reg_ctx.rhs_targeted.reset();
    }

    void clear_context()
    {
        assert(!m_register_contexts.empty());
        auto& reg_ctx = m_register_contexts.back();
        m_available_registers |= reg_ctx.rhs_targeted | reg_ctx.targeted | reg_ctx.temporary_registers;
        reg_ctx.targeted.reset();
        reg_ctx.rhs_targeted.reset();
        reg_ctx.temporary_registers.reset();
        debug(parser, "Cleared entire context:\n{}", contexts());
    }

    void enter_function(std::shared_ptr<MaterializedFunctionDef> const& func) const
    {
        m_function_stack.push_back(func);
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

    void function_return() const
    {
        assert(!m_function_stack.empty());
        auto func_def = m_function_stack.back();
        assembly().add_instruction("b", format("__{}_return", func_def->name()));
    }

    void leave_function() const
    {
        assert(!m_function_stack.empty());
        auto func_def = m_function_stack.back();
        assembly().add_label(format("__{}_return", func_def->name()));
        int depth = func_def->stack_depth();
        if (depth % 16) {
            depth += (16 - (depth % 16));
        }
        assembly().add_instruction("ldp", "fp,lr,[sp],#{}", depth);
        assembly().add_instruction("ret");
        m_function_stack.pop_back();
    }

private:
    RegisterContext& get_current_register_context()
    {
        assert(!m_register_contexts.empty());
        return m_register_contexts.back();
    }

    RegisterContext* get_previous_register_context()
    {
        if (m_register_contexts.size() < 2)
            return nullptr;
        return &m_register_contexts[m_register_contexts.size() - 2];
    }

    int claim_temporary_register()
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

    int claim_next_target()
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

    int claim_register(int reg)
    {
        if (!m_available_registers[reg])
            fatal(format("Register {} already claimed", reg).c_str());
        m_available_registers.reset(reg);
        return reg;
    }

    void release_register(int reg)
    {
        m_available_registers.set(reg);
    }

    Assembly& m_assembly;
    std::vector<RegisterContext> m_register_contexts {};
    std::bitset<19> m_available_registers { 0xFFFFFF };
    static std::vector<std::shared_ptr<MaterializedFunctionDef>> m_function_stack;
};

std::vector<std::shared_ptr<MaterializedFunctionDef>> MacOSXContext::m_function_stack {};

template<typename T = long>
void push(MacOSXContext& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("str", "{},[sp,-16]!", reg);
}

template<>
void push<uint8_t>(MacOSXContext& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("strb", "{},[sp,-16]!", reg);
}

template<typename T = long>
void pop(MacOSXContext& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("ldr", "{},[sp],16", reg);
}

template<>
void pop<uint8_t>(MacOSXContext& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("ldrb", "{},[sp],16", reg);
}

void push_imm(MacOSXContext& ctx, long value)
{
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("mov", "x{},{}", ctx.get_target_register(), value);
    push(ctx, format("x{}", ctx.get_target_register()));
    ctx.release_register_context();
}

void push_imm(MacOSXContext& ctx, uint8_t value)
{
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("movb", "w{},{}", ctx.get_target_register(), value);
    push<uint8_t>(ctx, format("w{}", ctx.get_target_register()));
    ctx.release_register_context();
}

template<typename T = long>
ErrorOr<void> push_var(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.get_target_register(), idx);
    push<T>(ctx, format("x{}", ctx.get_target_register()));
    ctx.release_register_context();
    return {};
}

template<>
ErrorOr<void> push_var<uint8_t>(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_target_register(), idx);
    push<uint8_t>(ctx, format("w{}", ctx.get_target_register()));
    ctx.release_register_context();
    return {};
}

template<typename T = long>
ErrorOr<void> pop_var(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.new_temporary_context();
    pop<T>(ctx, format("x{}", ctx.get_target_register()));
    ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.get_target_register(), idx);
    ctx.release_register_context();
    return {};
}

template<>
ErrorOr<void> pop_var<uint8_t>(MacOSXContext& ctx, std::string const& name)
{
    auto idx_maybe = ctx.get(name);
    if (!idx_maybe.has_value())
        return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", name) };
    auto idx = idx_maybe.value();
    ctx.new_temporary_context();
    pop<uint8_t>(ctx, format("w{}", ctx.get_target_register()));
    ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_target_register(), idx);
    ctx.release_register_context();
    return {};
}

void MacOSXContext::new_enclosing_context()
{
    if (!m_register_contexts.empty()) {
        auto& reg_ctx = m_register_contexts.back();
        for (int ix = 0; ix < reg_ctx.targeted.size(); ix++) {
            if (reg_ctx.targeted.test(ix))
                push(*this, format("x{}", ix));
        }
        for (int ix = 0; ix < reg_ctx.rhs_targeted.size(); ix++) {
            if (reg_ctx.rhs_targeted.test(ix))
                push(*this, format("x{}", ix));
        }
    }
    m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Enclosing);
    auto& reg_ctx = m_register_contexts.back();
    reg_ctx.m_saved_available_registers = m_available_registers;
    m_available_registers = 0xFFFFFF;
    debug(parser, "New enclosing context:\n{}", contexts());
}

void MacOSXContext::release_register_context()
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
            auto registers_to_copy = reg_ctx.targeted;
            m_register_contexts.pop_back();
            auto& prev_ctx = m_register_contexts.back();
            for (auto ix = prev_ctx.rhs_targeted.size() - 1; (int)ix >= 0; ix--) {
                if (prev_ctx.rhs_targeted.test(ix))
                    pop(*this, format("x{}", ix));
            }
            for (auto ix = prev_ctx.targeted.size() - 1; (int)ix >= 0; ix--) {
                if (prev_ctx.targeted.test(ix))
                    pop(*this, format("x{}", ix));
            }
            for (auto ix = 0; ix < registers_to_copy.size(); ix++) {
                if (registers_to_copy.test(ix)) {
                    auto reg = claim_next_target();
                    prev_ctx.targeted.set(reg);
                    if (reg != ix)
                        assembly().add_instruction("mov", "x{},x{}", reg, ix);
                }
            }
        }
        debug(parser, "Released enclosing context:\n{}", contexts());
        return;
    case RegisterContext::RegisterContextType::Targeted:
        if (prev_ctx_pointer) {
            prev_ctx_pointer->rhs_targeted |= reg_ctx.targeted;
            m_available_registers |= reg_ctx.rhs_targeted;
        } else {
            m_available_registers |= reg_ctx.targeted | reg_ctx.rhs_targeted;
        }
        m_available_registers |= reg_ctx.temporary_registers;
        break;
    case RegisterContext::RegisterContextType::Inherited:
        assert(prev_ctx_pointer);
        prev_ctx_pointer->targeted |= reg_ctx.targeted;
        prev_ctx_pointer->rhs_targeted |= reg_ctx.rhs_targeted;
        m_available_registers |= reg_ctx.temporary_registers;
        break;
    case RegisterContext::RegisterContextType::Temporary:
        m_available_registers |= reg_ctx.targeted | reg_ctx.rhs_targeted | reg_ctx.temporary_registers;
        break;
    }
    m_register_contexts.pop_back();
    debug(parser, "Released register context:\n{}", contexts());
}

ErrorOr<void> bool_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::ExclamationPoint:
        ctx.assembly().add_instruction("eorb", "w{},w{},#0x01", ctx.get_target_register()); // a is 0b00000001 (a was true) or 0b00000000 (a was false
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> bool_bool_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    auto lhs = ctx.get_target_register();
    auto rhs = ctx.get_rhs_register();
    switch (expr.op().code()) {
    case TokenCode::LogicalAnd:
        ctx.assembly().add_instruction("and", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::LogicalOr:
        ctx.assembly().add_instruction("orr", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Hat:
        ctx.assembly().add_instruction("xor", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Equals: {
        ctx.assembly().add_instruction("eor", "x{},x{},x{}", lhs, lhs, rhs); // a is 0b00000000 (a == b) or 0b00000001 (a != b
        ctx.assembly().add_instruction("eor", "x{},x{},#0x01", lhs, lhs);    // a is 0b00000001 (a == b) or 0b00000000 (a != b
        break;
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> int_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    auto operand = ctx.get_target_register();
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };
        ctx.assembly().add_instruction("neg", "x{},x{}", operand, operand);
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvn", "x{},x{}", operand, operand);
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> int_int_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    auto lhs = ctx.get_target_register();
    auto rhs = ctx.get_rhs_register();
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("add", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("sub", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Asterisk:
        ctx.assembly().add_instruction("mul", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Slash:
        ctx.assembly().add_instruction("sdiv", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmp", "x{},x{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("mov", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "x{},x{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b.le", set_false);
        ctx.assembly().add_instruction("mov", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "x{},x{}", lhs, rhs);
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b.lt", set_true);
        ctx.assembly().add_instruction("mov", "w{},wzr", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("mov", "w{},#0x01", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> byte_unary_expression(MacOSXContext& ctx, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    auto operand = ctx.get_target_register();
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };

        ctx.assembly().add_instruction("neg", "w{},w{}", operand, operand);
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvnb", "w{},w{}", operand, operand);
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> byte_byte_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    auto lhs = ctx.get_target_register();
    auto rhs = ctx.get_rhs_register();
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("andb", "w{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("subb", "w{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::Asterisk:
        ctx.assembly().add_instruction("smull", "x{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::Slash:
        ctx.assembly().add_instruction("sdiv", "w{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmp", "w{},w{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("movb", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "w{},w{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_false);
        ctx.assembly().add_instruction("movb", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "w{},w{}", lhs, rhs);
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_true);
        ctx.assembly().add_instruction("movb", "w{},wzr", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("movb", "w{},#0x01", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> string_binary_expression(MacOSXContext& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::Plus:
        break;
    case TokenCode::Asterisk:
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOrNode output_macosx_processor(std::shared_ptr<SyntaxNode> const& tree, MacOSXContext& ctx)
{
    switch (tree->node_type()) {
    case SyntaxNodeType::MaterializedFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<MaterializedFunctionDef>(tree);
        debug(parser, "func {}", func_def->name());
        if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
            ctx.enter_function(func_def);
            TRY_RETURN(output_macosx_processor(func_def->statement(), ctx));
            ctx.leave_function();
        }
        return tree;
    }

    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);

        // Load arguments in registers:
        ctx.new_enclosing_context();
        for (auto& argument : call->arguments()) {
            ctx.new_inherited_context();
            TRY_RETURN(output_macosx_processor(argument, ctx));
            ctx.release_register_context();
        }

        ctx.clear_context();
        // Call function:
        ctx.assembly().add_instruction("bl", call->name());
        // Add x0 to the register context
        ctx.add_target_register();
        if (call->type() == ObelixType::TypeString)
            ctx.add_target_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::NativeFunctionCall: {
        auto native_func_call = std::dynamic_pointer_cast<NativeFunctionCall>(tree);

        auto func_decl = native_func_call->declaration();

        ctx.new_enclosing_context();
        for (auto& arg : native_func_call->arguments()) {
            ctx.new_inherited_context();
            TRY_RETURN(output_macosx_processor(arg, ctx));
            ctx.release_register_context();
        }

        ctx.clear_context();
        // Call the native function
        ctx.assembly().add_instruction("bl", func_decl->native_function_name());
        // Add x0 to the register context
        ctx.add_target_register();
        if (native_func_call->type() == ObelixType::TypeString)
            ctx.add_target_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::CompilerIntrinsic: {
        auto call = std::dynamic_pointer_cast<CompilerIntrinsic>(tree);

        ctx.new_enclosing_context();
        if (call->name() == "allocate") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,xzr");
            ctx.assembly().add_instruction("mov", "w2,#3");
            ctx.assembly().add_instruction("mov", "w3,#0x1002");
            ctx.assembly().add_instruction("mov", "w4,#-1");
            ctx.assembly().add_instruction("mov", "x5,xzr");
            ctx.assembly().syscall(0xC5);
        }

        if (call->name() == "close") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x06);
        }

        if (call->name() == "fputs") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            auto fd_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "w{},w0", fd_reg);
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "w0,w{}", fd_reg);
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "itoa") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x2,x0");
            ctx.assembly().add_instruction("sub", "sp,sp,32");
            ctx.assembly().add_instruction("add", "x0,sp,16");
            ctx.assembly().add_instruction("mov", "x1,#32");
            ctx.assembly().add_instruction("mov", "w3,#10");
            ctx.assembly().add_instruction("bl", "to_string");
            ctx.assembly().add_instruction("add", "sp,sp,32");
        }

        if (call->name() == "exit") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x01);
        }

        if (call->name() == "eputs") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#0x02");
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "fsize") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("sub", "x1,sp,#-{}", sizeof(struct stat));
            ctx.assembly().syscall(189);
            ctx.assembly().add_instruction("cmp", "x0,#0x00");
            auto lbl = format("lbl_{}", Label::reserve_id());
            ctx.assembly().add_instruction("bne", lbl);
            ctx.assembly().add_instruction("ldr", "x0,[sp,-{}]", sizeof(struct stat) - offsetof(struct stat, st_size));
            ctx.assembly().add_label(lbl);
        }

        if (call->name() == "memset") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            int len_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},x0", len_reg);
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            int char_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},x0", char_reg);
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));

            int count_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},xzr", count_reg);

            auto loop = format("lbl_{}", Label::reserve_id());
            auto skip = format("lbl_{}", Label::reserve_id());
            ctx.assembly().add_label(loop);
            ctx.assembly().add_instruction("cmp", "x{},x{}", count_reg, len_reg);
            ctx.assembly().add_instruction("b.ge", skip);

            int ptr_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("add", "x{},x0,x{}", ptr_reg, count_reg);
            ctx.assembly().add_instruction("strb", "w{},[x{}]", char_reg, ptr_reg);
            ctx.assembly().add_instruction("add", "x{},x{},#1", count_reg, count_reg);
            ctx.assembly().add_instruction("b", loop);
            ctx.assembly().add_label(skip);
        }

        if (call->name() == "open") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            auto flags_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},x0", flags_reg);
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x1,x{}", flags_reg);
            ctx.assembly().syscall(0x05);
        }

        if (call->name() == "putchar") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("strb", "w0,[sp,-16]!");
            ctx.assembly().add_instruction("mov", "x0,#1"); // x0: stdin
            ctx.assembly().add_instruction("mov", "x1,sp"); // x1: SP
            ctx.assembly().add_instruction("mov", "x2,#1"); // x2: Number of characters
            ctx.assembly().syscall(0x04);
            ctx.assembly().add_instruction("add", "sp,sp,16");
        }

        if (call->name() == "puts") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#1");
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "read") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            auto len_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},x0", len_reg);
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            auto buf_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},x0", buf_reg);
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x2,x{}", len_reg);
            ctx.assembly().add_instruction("mov", "x1,x{}", buf_reg);
            ctx.assembly().syscall(0x03);
        }

        if (call->name() == "write") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            ctx.assembly().add_instruction("mov", "x2,x0");
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x04);
        }

        // Add x0 to the register context
        ctx.add_target_register();
        if (call->type() == ObelixType::TypeString)
            ctx.add_target_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if (expr->lhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->lhs()->to_string(0) };
        }
        if (expr->rhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->rhs()->to_string(0) };
        }

        ctx.new_inherited_context();
        ctx.new_targeted_context();
        auto rhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->rhs(), ctx));
        ctx.release_register_context();

        auto lhs = TRY_AND_CAST(Expression, output_macosx_processor(expr->lhs(), ctx));
        if ((expr->lhs()->type() == ObelixType::TypeInt && expr->lhs()->type() == ObelixType::TypeInt) || (expr->lhs()->type() == ObelixType::TypeUnsigned && expr->lhs()->type() == ObelixType::TypeUnsigned)) {
            TRY_RETURN(int_int_binary_expression(ctx, *expr));
        }
        if ((expr->lhs()->type() == ObelixType::TypeByte && expr->lhs()->type() == ObelixType::TypeByte) || (expr->lhs()->type() == ObelixType::TypeChar && expr->lhs()->type() == ObelixType::TypeChar)) {
            TRY_RETURN(byte_byte_binary_expression(ctx, *expr));
        }
        if (expr->lhs()->type() == ObelixType::TypeBoolean && expr->lhs()->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_bool_binary_expression(ctx, *expr));
        }
        if (expr->lhs()->type() == ObelixType::TypeString) {
            TRY_RETURN(string_binary_expression(ctx, *expr));
        }
        ctx.clear_rhs();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, output_macosx_processor(expr->operand(), ctx));

        if (operand->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, operand->to_string(0) };
        }

        if (operand->type() == ObelixType::TypeInt || operand->type() == ObelixType::TypeUnsigned) {
            TRY_RETURN(int_unary_expression(ctx, *expr));
        }
        if (operand->type() == ObelixType::TypeByte || operand->type() == ObelixType::TypeChar) {
            TRY_RETURN(byte_unary_expression(ctx, *expr));
        }
        if (operand->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_unary_expression(ctx, *expr));
        }
        return tree;
    }

    case SyntaxNodeType::Literal: {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        auto val_maybe = TRY(literal->to_object());
        assert(val_maybe.has_value());
        auto val = val_maybe.value();
        switch (val.type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned: {
            ctx.assembly().add_instruction("mov", "x{},#{}", ctx.get_target_register(), val->to_long().value());
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.get_target_register(), static_cast<uint8_t>(val->to_long().value()));
            break;
        }
        case ObelixType::TypeString: {
            auto str_id = Label::reserve_id();
            ctx.assembly().add_instruction("adr", "x{},str_{}", ctx.get_target_register(), str_id);
            auto reg = ctx.add_target_register();
            ctx.assembly().add_instruction("mov", "w{},#{}", reg, val->to_string().length());
            ctx.assembly().add_string(str_id, val->to_string());
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(val.type())));
        }
        return tree;
    }

    case SyntaxNodeType::Identifier: {
        auto identifier = std::dynamic_pointer_cast<Identifier>(tree);
        auto idx_maybe = ctx.get(identifier->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", identifier->name()) };
        auto idx = idx_maybe.value();

        switch (identifier->type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", "x0,[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("ldrbs", "w{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            auto reg = ctx.add_target_register();
            ctx.assembly().add_instruction("ldrw", "w{},[fp,#{}]", reg, idx + 8);
            break;
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot push values of variables of type {} yet", identifier->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::Assignment: {
        auto assignment = std::dynamic_pointer_cast<Assignment>(tree);

        auto idx_maybe = ctx.get(assignment->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", assignment->name()) };
        auto idx = idx_maybe.value();

        TRY_RETURN(output_macosx_processor(assignment->expression(), ctx));

        switch (assignment->type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strbs", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("strb", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(), idx);
            auto reg = ctx.add_target_register();
            ctx.assembly().add_instruction("strw", "w{},[fp,#{}]", reg, idx + 8);
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::MaterializedVariableDecl: {
        auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
        debug(parser, "{}", var_decl->to_string(0));
        ctx.assembly().add_comment(var_decl->to_string(0));
        ctx.declare(var_decl->variable().identifier(), var_decl->offset());
        ctx.release_all();
        ctx.new_targeted_context();
        if (var_decl->expression() != nullptr) {
            TRY_RETURN(output_macosx_processor(var_decl->expression(), ctx));
        } else {
            switch (var_decl->expression()->type()) {
            case ObelixType::TypeString: {
                auto reg = ctx.add_target_register();
                ctx.assembly().add_instruction("mov", "w{},wzr", reg);
            } // fall through
            case ObelixType::TypePointer:
            case ObelixType::TypeInt:
            case ObelixType::TypeUnsigned:
            case ObelixType::TypeByte:
            case ObelixType::TypeChar:
            case ObelixType::TypeBoolean:
                ctx.assembly().add_instruction("mov", "x{},xzr", ctx.get_target_register());
                break;
            default:
                return Error { ErrorCode::NotYetImplemented, format("Cannot initialize variables of type {} yet", var_decl->expression()->type()) };
            }
        }
        ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(), var_decl->offset());
        if (ctx.get_target_count() > 1) {
            debug(parser, "count: {}\n{}", ctx.get_target_count(), ctx.contexts());
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(1), var_decl->offset() + 8);
            debug(parser, "2");
        }
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        debug(parser, "{}", expr_stmt->to_string(0));
        ctx.assembly().add_comment(expr_stmt->to_string(0));
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_macosx_processor(expr_stmt->expression(), ctx));
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        debug(parser, "{}", ret->to_string(0));
        ctx.assembly().add_comment(ret->to_string(0));
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_macosx_processor(ret->expression(), ctx));
        ctx.release_register_context();
        ctx.function_return();

        return tree;
    }

    case SyntaxNodeType::Label: {
        auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
        debug(parser, "{}", label->to_string(0));
        ctx.assembly().add_comment(label->to_string(0));
        ctx.assembly().add_label(format("lbl_{}", label->label_id()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
        debug(parser, "{}", goto_stmt->to_string(0));
        ctx.assembly().add_comment(goto_stmt->to_string(0));
        ctx.assembly().add_instruction("b", "lbl_{}", goto_stmt->label_id());
        return tree;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        ctx.release_all();

        auto end_label = Obelix::Label::reserve_id();
        auto count = if_stmt->branches().size() - 1;
        for (auto& branch : if_stmt->branches()) {
            auto else_label = (count) ? Obelix::Label::reserve_id() : end_label;
            if (branch->condition()) {
                debug(parser, "if ({})", branch->condition()->to_string(0));
                ctx.assembly().add_comment(format("if ({})", branch->condition()->to_string(0)));
                ctx.new_targeted_context();
                auto cond = TRY_AND_CAST(Expression, output_macosx_processor(branch->condition(), ctx));
                ctx.assembly().add_instruction("cmp", "w{},0x00", ctx.get_target_register());
                ctx.assembly().add_instruction("b.eq", "lbl_{}", else_label);
            } else {
                debug(parser, "else");
                ctx.assembly().add_comment("else");
            }
            auto stmt = TRY_AND_CAST(Statement, output_macosx_processor(branch->statement(), ctx));
            if (count) {
                ctx.assembly().add_instruction("b", "lbl_{}", end_label);
                ctx.assembly().add_label(format("lbl_{}", else_label));
            }
            count--;
        }
        ctx.assembly().add_label(format("lbl_{}", end_label));
        return tree;
    }

    default:
        return process_tree(tree, ctx, output_macosx_processor);
    }
}

ErrorOrNode prepare_arm64_processor(std::shared_ptr<SyntaxNode> const& tree, Context<int>& ctx)
{
    switch (tree->node_type()) {
    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto func_decl = func_def->declaration();
        Context<int> func_ctx(ctx);
        int offset = 16;
        FunctionParameters function_parameters;
        for (auto& parameter : func_decl->parameters()) {
            function_parameters.push_back(std::make_shared<FunctionParameter>(parameter, offset));
            switch (parameter.type()) {
            case ObelixType::TypeString:
                offset += 16;
                break;
            default:
                offset += 8;
            }
        }

        auto materialized_function_decl = std::make_shared<MaterializedFunctionDecl>(func_decl->identifier(), function_parameters);
        if (func_decl->node_type() == SyntaxNodeType::NativeFunctionDecl) {
            auto native_decl = std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl);
            materialized_function_decl = std::make_shared<MaterializedNativeFunctionDecl>(materialized_function_decl, native_decl->native_function_name());
        }

        func_ctx.declare("#offset", offset);
        std::shared_ptr<Block> block;
        if (func_def->statement()) {
            assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
            block = TRY_AND_CAST(FunctionBlock, prepare_arm64_processor(func_def->statement(), func_ctx));
        }
        return std::make_shared<MaterializedFunctionDef>(materialized_function_decl, block, func_ctx.get("#offset").value());
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto offset = ctx.get("#offset").value();
        auto ret = std::make_shared<MaterializedVariableDecl>(var_decl, offset);
        switch (var_decl->type()) {
        case ObelixType::TypeString:
            offset += 16;
            break;
        default:
            offset += 8;
        }
        ctx.set("#offset", offset);
        return ret;
    }

    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);
        if (is_intrinsic(call->name()))
            return std::make_shared<CompilerIntrinsic>(call);
        return tree;
    }

    default:
        return process_tree(tree, ctx, prepare_arm64_processor);
    }
}

ErrorOrNode prepare_arm64(std::shared_ptr<SyntaxNode> const& tree)
{
    Context<int> root;
    return prepare_arm64_processor(tree, root);
}

ErrorOrNode output_macosx(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    auto processed = TRY(prepare_arm64(tree));

    Assembly assembly;
    MacOSXContext root(assembly);

#if 0
    assembly.code = ".global _start\n"
                    ".align 2\n\n"
                    "_start:\n"
                    "\tmov\tfp,sp\n"
                    "\tbl\t_init\n"
                    "\tbl\tfunc_main\n"
                    "\tmov\tx16,#1\n"
                    "\tsvc\t0\n";
#else
    assembly.code = ".align 2\n\n";
#endif

    auto ret = output_macosx_processor(processed, root);

    if (ret.is_error()) {
        return ret;
    }

    printf("%s\n%s\n", assembly.code.c_str(), assembly.text.c_str());

    std::string bare_file_name = file_name;
    if (auto dot = file_name.find_last_of('.'); dot != std::string::npos) {
        bare_file_name = file_name.substr(0, dot);
    }
    auto assembly_file = format("{}.s", bare_file_name);
    FILE* f = fopen(assembly_file.c_str(), "w");
    if (f) {
        fprintf(f, "%s\n%s\n", assembly.code.c_str(), assembly.text.c_str());
        fclose(f);

        std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;
        auto as_cmd = format("as -o {}.o {}.s", bare_file_name, bare_file_name);
        auto ld_cmd = format("ld -o {} {}.o -loblrt -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 -L{}/lib",
            bare_file_name, bare_file_name, obl_dir);

        std::cout << "[CMD] " << as_cmd << "\n";
        if (!system(as_cmd.c_str())) {
            std::cout << "[CMD] " << ld_cmd << "\n";
            system(ld_cmd.c_str());
        }
    }
    return ret;
}

}
