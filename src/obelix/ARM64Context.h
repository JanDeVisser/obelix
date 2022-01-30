/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <bitset>

#include <core/Logging.h>
#include <obelix/Context.h>
#include <obelix/Syntax.h>

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
    std::bitset<19> reserved_registers { 0x0 };
    std::bitset<19> m_saved_available_registers { 0x0 };

    [[nodiscard]] std::string to_string() const
    {
        return format("{9s} lhs: {} rhs: {} res: {} temp: {}", RegisterContextType_name(type),
            targeted, rhs_targeted, reserved_registers, temporary_registers);
    }
};

class ARM64Context : public Context<int> {
public:
    ARM64Context(ARM64Context& parent);
    explicit ARM64Context(ARM64Context* parent);
    explicit ARM64Context(Assembly& assembly);

    [[nodiscard]] Assembly& assembly() const { return m_assembly; }

    [[nodiscard]] std::string contexts() const;
    void new_targeted_context();
    void new_inherited_context();
    void new_enclosing_context();
    void new_temporary_context();
    void release_register_context();

    void release_all();
    [[nodiscard]] size_t get_target_count() const;
    int get_target_register(size_t ix = 0);
    [[nodiscard]] size_t get_rhs_count() const;
    int get_rhs_register(size_t ix = 0);
    int add_target_register();
    int temporary_register();

    template<typename... Ints>
    void reserve_register(int reg, Ints... args)
    {
        assert(!m_register_contexts.empty());
        assert(m_available_registers.test(reg));
        auto& reg_ctx = m_register_contexts.back();
        m_available_registers.reset(reg);
        reg_ctx.reserved_registers.set(reg);
        debug(parser, "Reserved register {}:\n{}", reg, contexts());
        reserve_register(std::forward<Ints>(args)...);
    }

    void reserve_register()
    {
    }

    void clear_targeted();
    void clear_rhs();
    void clear_context();
    void enter_function(std::shared_ptr<MaterializedFunctionDef> const& func) const;
    void function_return() const;
    void leave_function() const;

private:
    RegisterContext& get_current_register_context();
    RegisterContext* get_previous_register_context();
    int claim_temporary_register();
    int claim_next_target();
    int claim_register(int reg);
    void release_register(int reg);

    Assembly& m_assembly;
    std::vector<RegisterContext> m_register_contexts {};
    std::bitset<19> m_available_registers { 0xFFFFFF };
    static std::vector<std::shared_ptr<MaterializedFunctionDef>> m_function_stack;
};

template<typename T = long>
static inline void push(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("str", "{},[sp,-16]!", reg);
}

template<>
void push<uint8_t>(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("strb", "{},[sp,-16]!", reg);
}

template<typename T = long>
static inline void pop(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("ldr", "{},[sp],16", reg);
}

template<>
void pop<uint8_t>(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("ldrb", "{},[sp],16", reg);
}

template<typename T = long>
static inline void push_imm(ARM64Context& ctx, T value)
{
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("mov", "x{},{}", ctx.get_target_register(), value);
    push(ctx, format("x{}", ctx.get_target_register()));
    ctx.release_register_context();
}

template<>
[[maybe_unused]] void push_imm(ARM64Context& ctx, uint8_t value)
{
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("movb", "w{},{}", ctx.get_target_register(), value);
    push<uint8_t>(ctx, format("w{}", ctx.get_target_register()));
    ctx.release_register_context();
}

template<typename T = long>
static inline ErrorOr<void> push_var(ARM64Context& ctx, std::string const& name)
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
[[maybe_unused]] ErrorOr<void> push_var<uint8_t>(ARM64Context& ctx, std::string const& name)
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
static inline ErrorOr<void> pop_var(ARM64Context& ctx, std::string const& name)
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
[[maybe_unused]] ErrorOr<void> pop_var<uint8_t>(ARM64Context& ctx, std::string const& name)
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

}
