/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <config.h>
#include <core/Logging.h>
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

    void add_data(std::string const& label, std::string d)
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

struct RegisterContext {
    enum class RegisterContextType {
        Enclosing,
        Targeted,
        Subordinate,
        Temporary
    };

    explicit RegisterContext(RegisterContextType context_type = RegisterContextType::Temporary)
        : type(context_type)
    {
    }

    RegisterContextType type { RegisterContextType::Temporary };
    std::bitset<19> targeted { 0x0 };
    std::bitset<19> rhs_targeted { 0x0 };
    std::bitset<19> temporary_registers { 0x0 };
};

class MacOSXContext : public Context<Obj> {
public:
    MacOSXContext(MacOSXContext& parent)
        : Context<Obj>(parent)
        , m_assembly(parent.assembly())
    {
        auto offset_maybe = get("#offset");
        assert(offset_maybe.has_value());
        declare("#offset", get("#offset").value());
    }

    explicit MacOSXContext(MacOSXContext* parent)
        : Context<Obj>(parent)
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

    void new_targeted_context()
    {
        m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Targeted);
        auto& reg_ctx = m_register_contexts.back();
        auto reg = claim_next_target();
        reg_ctx.targeted.set(reg);
    }

    void new_enclosing_context();

    void new_temporary_context()
    {
        m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Temporary);
        auto& reg_ctx = m_register_contexts.back();
        auto reg = claim_temporary_register();
        reg_ctx.targeted.set(reg);
    }

    void release_register_context();

    void release_all()
    {
        m_available_registers = 0xFFFFFF;
        m_register_contexts.clear();
    }

    [[nodiscard]] size_t get_target_count() const
    {
        auto& reg_ctx = m_register_contexts.back();
        return reg_ctx.targeted.count();
    }

    int get_target_register(size_t ix = 0)
    {
        auto& reg_ctx = m_register_contexts.back();
        assert(ix < reg_ctx.targeted.count());
        int count = 0;
        for (auto i = 0; i < 19; i++) {
            if (reg_ctx.targeted[i] && (count++ == ix))
                return i;
        }
        fatal("Unreachable");
    }

    [[nodiscard]] size_t get_rhs_count() const
    {
        auto& reg_ctx = m_register_contexts.back();
        return reg_ctx.targeted.count();
    }

    int get_rhs_register(size_t ix = 0)
    {
        auto& reg_ctx = m_register_contexts.back();
        assert(ix < reg_ctx.rhs_targeted.count());
        int count = 0;
        for (auto i = 0; i < 19; i++) {
            if (reg_ctx.targeted[i] && (count++ == ix))
                return i;
        }
        fatal("Unreachable");
    }

    int add_target_register()
    {
        auto& reg_ctx = m_register_contexts.back();
        auto reg = (reg_ctx.type == RegisterContext::RegisterContextType::Temporary) ? claim_temporary_register() : claim_next_target();
        reg_ctx.targeted.set(reg);
        return reg;
    }

    int temporary_register()
    {
        auto& reg_ctx = m_register_contexts.back();
        auto ret = claim_temporary_register();
        reg_ctx.temporary_registers.set(ret);
        return ret;
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
            if (m_available_registers[ix]) {
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
            if (m_available_registers[ix]) {
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
};

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
    ctx.assembly().add_instruction("ldr", "x{},[fp,{}]", ctx.get_target_register(), static_cast<T>(idx->to_long().value()));
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
    ctx.assembly().add_instruction("ldrb", "w{},[fp,{}]", ctx.get_target_register(), static_cast<uint8_t>(idx->to_long().value()));
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
    ctx.assembly().add_instruction("str", "x{},[fp,{}]", ctx.get_target_register(), idx);
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
    ctx.assembly().add_instruction("strb", "w{},[fp,{}]", ctx.get_target_register(), idx);
    ctx.release_register_context();
    return {};
}

void MacOSXContext::new_enclosing_context()
{
    if (!m_register_contexts.empty()) {
        auto& reg_ctx = m_register_contexts.back();
        for (int ix = 0; ix < reg_ctx.targeted.count(); ix++) {
            push(*this, format("x{}", get_target_register(ix)));
        }
        for (int ix = 0; ix < reg_ctx.rhs_targeted.count(); ix++) {
            push(*this, format("x{}", get_rhs_register(ix)));
        }
    }
    m_available_registers = 0xFFFFFF;
    m_register_contexts.emplace_back(RegisterContext::RegisterContextType::Enclosing);
}

void MacOSXContext::release_register_context()
{
    assert(!m_register_contexts.empty());
    auto& reg_ctx = m_register_contexts.back();
    auto* prev_ctx = get_previous_register_context();

    m_available_registers |= reg_ctx.temporary_registers;
    switch (reg_ctx.type) {
    case RegisterContext::RegisterContextType::Enclosing:
        m_available_registers = 0xFFFFFF;
        if (prev_ctx) {
            m_register_contexts.pop_back();
            m_available_registers ^= (prev_ctx->targeted | prev_ctx->rhs_targeted);
            if (prev_ctx->rhs_targeted.count() > 0) {
                for (auto ix = prev_ctx->rhs_targeted.count() - 1; ix >= 0; ix++) {
                    pop(*this, format("x{}", get_rhs_register(ix)));
                }
            }
            if (prev_ctx->targeted.count() > 0) {
                for (auto ix = prev_ctx->targeted.count() - 1; ix >= 0; ix++) {
                    push(*this, format("x{}", get_target_register(ix)));
                }
            }
            prev_ctx->temporary_registers.reset();
        }
        return;
    case RegisterContext::RegisterContextType::Targeted:
    case RegisterContext::RegisterContextType::Subordinate:
        if (prev_ctx)
            prev_ctx->rhs_targeted |= reg_ctx.targeted | reg_ctx.rhs_targeted;
        break;
    case RegisterContext::RegisterContextType::Temporary:
        m_available_registers |= reg_ctx.targeted | reg_ctx.rhs_targeted;
        break;
    }
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
    case SyntaxNodeType::FunctionDecl: {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
        ctx.assembly().add_comment(func_decl->to_string(0));

        // Set fp offsets of function parameters:
        auto ix = 48;
        for (auto& parameter : func_decl->parameters()) {
            ctx.declare(parameter.name(), make_obj<Integer>(ix));
            ix += 16;
        }

        ctx.assembly().add_directive(".global", func_decl->name());
        ctx.assembly().add_label(func_decl->name());

        // Push the return value:
        push(ctx, "lr");

        // Set fp to the current sp. Now a return is setting sp back to fp,
        // and popping lr followed by ret.
        ctx.assembly().add_instruction("mov", "fp,sp");
        return tree;
    }

        /*
         *  +------------------- +
         *  | Caller function fp |
         *  +--------------------+
         *  |     argument n     |
         *  +--------------------+  <---- Temp fp
         *  |    argument n-1    |
         *  +------------------- +
         *  |       ....         |
         *  +--------------------+
         *  |     argument 1     |
         *  +--------------------+   <- fp[48]
         *  |       Temp fp      |
         *  +--------------------+   <- fp[32]
         *  |  return addr (lr)  |
         *  +--------------------+
         *  |    local var #1    |
         *  +--------------------+   <---- Called function fp
         *  |       ....         |
         *  +--------------------+
         */

    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);

        // Push fp, and set temp fp to sp:
        push(ctx, "fp");
        ctx.assembly().add_instruction("mov", "fp,sp");

        ctx.new_enclosing_context();
        for (auto& argument : call->arguments()) {
            ctx.new_targeted_context();
            TRY_RETURN(output_macosx_processor(argument, ctx));
            ctx.release_register_context();
        }
        ctx.release_register_context();

        // Push temp fp.
        push(ctx, "fp");

        // Call function:
        ctx.assembly().add_instruction("bl", call->name());

        // Pop temp fp:
        pop(ctx, "fp");

        // Load sp with temp fp. This will discard all function arguments
        ctx.assembly().add_instruction("mov", "sp,fp");

        // Pop caller fp:
        pop(ctx, "fp");
        return tree;
    }

    case SyntaxNodeType::NativeFunctionCall: {
        auto native_func_call = std::dynamic_pointer_cast<NativeFunctionCall>(tree);

        auto func_decl = native_func_call->declaration();

        ctx.new_enclosing_context();
        for (auto& arg : native_func_call->arguments()) {
            ctx.new_targeted_context();
            TRY_RETURN(output_macosx_processor(arg, ctx));
            ctx.release_register_context();
        }
        ctx.release_register_context();

        // Call the native function
        ctx.assembly().add_instruction("bl", func_decl->native_function_name());
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
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
        }

        if (call->name() == "close") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x06);
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
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
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
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
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
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
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
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
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
        }

        if (call->name() == "putchar") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("strb", "w0,[sp,-16]!");
            ctx.assembly().add_instruction("mov", "x0,#1");    // x0: stdin
            ctx.assembly().add_instruction("add", "x1,sp,16"); // x1: 16 bytes up from SP
            ctx.assembly().add_instruction("mov", "x2,#1");    // x2: Number of characters
            ctx.assembly().syscall(0x04);
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
            ctx.assembly().add_instruction("add", "sp,sp,16");
        }

        if (call->name() == "puts") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "w2,w1");
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,#1");
            ctx.assembly().syscall(0x04);
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
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
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
        }

        if (call->name() == "write") {
            TRY_RETURN(output_macosx_processor(call->arguments().at(2), ctx));
            ctx.assembly().add_instruction("mov", "x2,x0");
            TRY_RETURN(output_macosx_processor(call->arguments().at(1), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            TRY_RETURN(output_macosx_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x04);
            if (ctx.get_target_register() != 0)
                ctx.assembly().add_instruction("mov", "x{},x0", ctx.get_target_register());
        }
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
        return tree;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        ctx.new_targeted_context();
        auto operand = TRY_AND_CAST(Expression, output_macosx_processor(expr->operand(), ctx));
        ctx.release_register_context();

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
            ctx.assembly().add_instruction("ldr", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", "x0,[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("ldrbs", "w{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", "w{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("ldr", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            auto reg = ctx.add_target_register();
            ctx.assembly().add_instruction("ldrw", "w{},[fp,-{}]", reg, idx->to_long().value() + 8);
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
            ctx.assembly().add_instruction("str", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strbs", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("strb", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("str", "x{},[fp,-{}]", ctx.get_target_register(), idx->to_long().value());
            auto reg = ctx.add_target_register();
            ctx.assembly().add_instruction("strw", "w{},[fp,-{}]", reg, idx->to_long().value() + 8);
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        ctx.assembly().add_comment(var_decl->to_string(0));
        auto offset = (signed char)ctx.get("#offset").value()->to_long().value();
        ctx.set("#offset", make_obj<Integer>(offset + 16)); // FIXME Use type size
        ctx.declare(var_decl->variable().identifier(), make_obj<Integer>(offset));
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
        ctx.assembly().add_instruction("str", "x{},[sp,-16]", ctx.get_target_register());
        if (ctx.get_target_count() > 1) {
            ctx.assembly().add_instruction("strw", "x{},[sp,8]", ctx.get_target_register(1));
        }
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        ctx.assembly().add_comment(expr_stmt->to_string(0));
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_macosx_processor(expr_stmt->expression(), ctx));
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        ctx.assembly().add_comment(ret->to_string(0));
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_macosx_processor(ret->expression(), ctx));
        ctx.release_register_context();

        // Load sp with the current value of bp. This will discard all local variables
        ctx.assembly().add_instruction("mov", "sp,fp");

        // Pop return address into lr:
        pop(ctx, "lr");

        // Return:
        ctx.assembly().add_instruction("ret");
        return tree;
    }

    case SyntaxNodeType::Label: {
        auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
        ctx.assembly().add_comment(label->to_string(0));
        ctx.assembly().add_label(format("lbl_{}", label->label_id()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
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
                ctx.assembly().add_comment(format("if ({})", branch->condition()->to_string(0)));
                ctx.new_targeted_context();
                auto cond = TRY_AND_CAST(Expression, output_macosx_processor(branch->condition(), ctx));
                ctx.assembly().add_instruction("cmp", "w{},0x00", ctx.get_target_register());
                ctx.assembly().add_instruction("b.eq", "lbl_{}", else_label);
            } else {
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

ErrorOrNode extract_intrinsics_processor(std::shared_ptr<SyntaxNode> const& tree, Context<int>& ctx)
{
    switch (tree->node_type()) {
    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);

        if (is_intrinsic(call->name())) {
            return std::make_shared<CompilerIntrinsic>(call);
        }
        return tree;
    }
    default:
        return process_tree(tree, ctx, extract_intrinsics_processor);
    }
}

ErrorOrNode extract_intrinsics(std::shared_ptr<SyntaxNode> const& tree)
{
    Context<int> root;
    return extract_intrinsics_processor(tree, root);
}

ErrorOrNode output_macosx(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    auto processed = TRY(extract_intrinsics(tree));

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
