/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <bitset>
#include <cstdio>
#include <fstream>
#include <iostream>

#include <config.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <obelix/Context.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(arm64);

class Assembly {
public:
    template<typename... Args>
    void add_instruction(std::string const& mnemonic, Args&&... args)
    {
        m_code = format("{}\t{}\t{}\n", m_code, mnemonic, std::forward<Args>(args)...);
    }

    void add_instruction(std::string const& mnemonic)
    {
        m_code = format("{}\t{}\n", m_code, mnemonic);
    }

    void add_label(std::string const& label)
    {
        m_code = format("{}{}:\n", m_code, label);
    }

    void add_directive(std::string const& directive, std::string const& args)
    {
        if (directive == ".global")
            m_has_exports = true;
        m_code = format("{}{}\t{}\n", m_code, directive, args);
    }

    int add_string(std::string const& str)
    {
        if (m_strings.contains(str)) {
            return m_strings.at(str);
        }
        auto id = Label::reserve_id();
        m_text = format("{}.align 2\nstr_{}:\n\t.string\t\"{}\"\n", m_text, id, str);
        m_strings[str] = id;
        return id;
    }

    void add_comment(std::string const& comment)
    {
        auto c = comment;
        for (auto pos = c.find('\n'); pos != std::string::npos; pos = c.find('\n'))
            c[pos] = ' ';
        m_code = format("{}\n\t; {}\n", m_code, c);
    }

    void add_data(std::string const& label, std::string const& d)
    {
        if (m_data.empty())
            m_data = ".data\n\n";
        m_data = format("{}\n.align 2\n{}:\t{}", m_data, label, d);
    }

    void syscall(int id)
    {
        add_instruction("mov", format("x16,#{}", id));
        add_instruction("svc", "#0x00");
    }

    [[nodiscard]] std::string to_string() const
    {
        return format("{}\n{}\n{}\n", m_code, m_text, m_data);
    }

    ErrorOr<void> save_and_assemble(std::string const& bare_file_name) const
    {
        {
            std::fstream s(bare_file_name + ".s", std::fstream::out);
            if (!s.is_open())
                return Error { ErrorCode::IOError, format("Could not open assembly file {}", bare_file_name + ".s") };
            s << to_string();
            if (s.fail() || s.bad())
                return Error { ErrorCode::IOError, format("Could not write assembly file {}", bare_file_name + ".s") };
        }
        if (auto code = execute("as", bare_file_name + ".s", "-o", bare_file_name + ".o"); code.is_error())
            return code.error();
        return {};
    }

    [[nodiscard]] bool has_exports() const { return m_has_exports; }

private:
    std::string m_code { ".align 2\n\n" };
    std::string m_text;
    std::string m_data;
    bool m_has_exports { false };
    std::unordered_map<std::string, int> m_strings {};
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
    std::bitset<19> assigned { 0x0 };
    std::bitset<19> temporary_registers { 0x0 };
    std::bitset<19> reserved_registers { 0x0 };
    std::bitset<19> m_saved_available_registers { 0x0 };

    [[nodiscard]] std::string to_string() const
    {
        return format("{9s} assigned: {} res: {} temp: {}", RegisterContextType_name(type),
            assigned, reserved_registers, temporary_registers);
    }
};

class ARM64Context : public Context<int> {
public:
    constexpr static char const* ROOT_MODULE_NAME = "#root";

    ARM64Context(ARM64Context& parent);
    explicit ARM64Context(ARM64Context* parent);
    ARM64Context();

    [[nodiscard]] Assembly& assembly() const
    {
        assert(m_assembly);
        return *m_assembly;
    }

    [[nodiscard]] std::string contexts() const;
    void new_targeted_context();
    void new_inherited_context();
    void new_enclosing_context();
    void new_temporary_context();
    void release_register_context();

    void release_all();
    int get_register(size_t ix = 0, int level = 0);
    int add_register(int level = 0);
    int temporary_register();

    template<typename... Ints>
    void reserve_register(int reg, Ints... args)
    {
        assert(!m_register_contexts.empty());
        assert(m_available_registers.test(reg));
        auto& reg_ctx = m_register_contexts.back();
        m_available_registers.reset(reg);
        reg_ctx.reserved_registers.set(reg);
        debug(arm64, "Reserved register {}:\n{}", reg, contexts());
        reserve_register(std::forward<Ints>(args)...);
    }

    void reserve_register()
    {
    }

    void clear_context();
    void enter_function(std::shared_ptr<MaterializedFunctionDef> const& func) const;
    void function_return() const;
    void leave_function() const;

    void add_module(std::string const& module)
    {
        m_assembly = &s_assemblies[module];
    }

    [[nodiscard]] static std::unordered_map<std::string, Assembly> const& assemblies()
    {
        return s_assemblies;
    }

private:
    RegisterContext& get_current_register_context();
    RegisterContext* get_previous_register_context();
    int claim_temporary_register();
    int claim_next_target();
    int claim_register(int reg);
    void release_register(int reg);

    Assembly* m_assembly { nullptr };
    std::vector<RegisterContext> m_register_contexts {};
    std::bitset<19> m_available_registers { 0xFFFFFF };
    static std::vector<std::shared_ptr<MaterializedFunctionDef>> s_function_stack;
    static std::unordered_map<std::string, Assembly> s_assemblies;
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
    ctx.assembly().add_instruction("mov", "x{},{}", ctx.get_register(), value);
    push(ctx, format("x{}", ctx.get_register()));
    ctx.release_register_context();
}

template<>
[[maybe_unused]] void push_imm(ARM64Context& ctx, uint8_t value)
{
    ctx.new_temporary_context();
    ctx.assembly().add_instruction("movb", "w{},{}", ctx.get_register(), value);
    push<uint8_t>(ctx, format("w{}", ctx.get_register()));
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
    ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.get_register(), idx);
    push<T>(ctx, format("x{}", ctx.get_register()));
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
    ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_register(), idx);
    push<uint8_t>(ctx, format("w{}", ctx.get_register()));
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
    pop<T>(ctx, format("x{}", ctx.get_register()));
    ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.get_register(), idx);
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
    pop<uint8_t>(ctx, format("w{}", ctx.get_register()));
    ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_register(), idx);
    ctx.release_register_context();
    return {};
}

}
