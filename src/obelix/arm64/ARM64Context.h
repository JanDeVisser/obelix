/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>

#include <config.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <obelix/Context.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Syntax.h>
#include <string>

namespace Obelix {

extern_logging_category(arm64);

class ARM64Context;
using ARM64Implementation = std::function<ErrorOr<void, SyntaxError>(ARM64Context&)>;

class Assembly {
public:
    template<typename... Args>
    void add_instruction(std::string const& mnemonic, std::string const& param, Args&&... args)
    {
        m_code = format("{}\t{}\t" + param + '\n', m_code, mnemonic, std::forward<Args>(args)...);
    }

    void add_instruction(std::string const& mnemonic, std::string const& param = "")
    {
        m_code = m_code + '\t' + mnemonic + '\t' + param + '\n';
    }

    void add_text(std::string const& text)
    {
        if (text.empty())
            return;
        auto t = strip(text);
        for (auto const& line : split(strip(text), '\n')) {
            if (line.empty()) {
                m_code += '\n';
                continue;
            }
            if (line[0] == ';') {
                m_code += '\t' + line + '\n';
                continue;
            }
            if ((line[0] == '.') || line.ends_with(":")) {
                m_code += line + '\n';
                continue;
            }
            auto parts = split(strip(line), ' ');
            m_code += '\t' + parts[0];
            if (parts.size() > 1) {
                for (auto ix = 1u; ix < parts.size(); ++ix) {
                    if (parts[ix].empty())
                        continue;
                    m_code += '\t' + parts[ix];
                }
            }
            m_code += '\n';
        }
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

    template <typename Arg>
    void add_data(std::string const& label, bool global, std::string type, bool is_static, Arg const& arg)
    {
        if (m_data.empty())
            m_data = "\n\n.section __DATA,__data\n";
        if (global)
            m_data += format("\n.global {}", label);
        m_data += format("\n.align 2\n{}:\n\t{}\t{}", label, type, arg);
        if (is_static)
            m_data += format("\n\t.short 0");
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

    ErrorOr<void, SyntaxError> save_and_assemble(std::string const& bare_file_name) const
    {
        {
            std::fstream s(bare_file_name + ".s", std::fstream::out);
            if (!s.is_open())
                return SyntaxError { ErrorCode::IOError, format("Could not open assembly file {}", bare_file_name + ".s") };
            s << to_string();
            if (s.fail() || s.bad())
                return SyntaxError { ErrorCode::IOError, format("Could not write assembly file {}", bare_file_name + ".s") };
        }
        if (auto code = execute("as", bare_file_name + ".s", "-o", bare_file_name + ".o"); code.is_error())
            return SyntaxError { code.error().code(), code.error().message() };
        return {};
    }

    [[nodiscard]] bool has_exports() const { return m_has_exports; }

private:
    std::string m_code { ".section	__TEXT,__text,regular,pure_instructions\n\n.align 2\n\n" };
    std::string m_text;
    std::string m_data;
    bool m_has_exports { false };
    std::unordered_map<std::string, int> m_strings {};
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

    void enter_function(std::shared_ptr<MaterializedFunctionDef> const& func);
    void function_return() const;
    void leave_function();

    void add_module(std::string const& module)
    {
        m_assembly = &s_assemblies[module];
    }

    [[nodiscard]] static std::unordered_map<std::string, Assembly> const& assemblies()
    {
        return s_assemblies;
    }

    void reserve_on_stack(size_t);
    void release_stack();

    [[nodiscard]] static unsigned long counter()
    { 
        return s_counter++; 
    }

    [[nodiscard]] size_t stack_depth() const
    {
        if (m_stack_depth.size() > 0) {
            return m_stack_depth.back();
        }
        assembly().add_comment(format("Stack depth empty!"));
        return 0;
    }

protected:
    void stack_depth(size_t depth)
    {
        m_stack_depth.push_back(depth);
        assembly().add_comment(format("Set stack depth to {}", stack_depth()));
    }

    void pop_stack_depth()
    {
        auto current = m_stack_depth.back();
        m_stack_depth.pop_back();
        if (m_stack_depth.empty()) {
            assembly().add_comment(format("Stack depth popped. Was {}, now empty", current));
        } else {
            assembly().add_comment(format("Stack depth popped. Was {}, now {}", current, stack_depth()));
        }
    }

private:
    Assembly* m_assembly { nullptr };
    size_t m_stack_allocated { 0 };
    std::vector<size_t> m_stack_depth {};
    static std::vector<std::shared_ptr<MaterializedFunctionDef>> s_function_stack;
    static std::unordered_map<std::string, Assembly> s_assemblies;
    static unsigned long s_counter;
};

template<typename T = long>
static inline void push(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("str", "{},[sp,-16]!", reg);
}

template<>
[[maybe_unused]] void push<uint8_t>(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("strb", "{},[sp,-16]!", reg);
}

template<typename T = long>
static inline void pop(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("ldr", "{},[sp],16", reg);
}

template<>
[[maybe_unused]] void pop<uint8_t>(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly().add_instruction("ldrb", "{},[sp],16", reg);
}

#if 0
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
#endif

}