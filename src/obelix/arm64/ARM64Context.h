/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>

#include <config.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <obelix/Context.h>
#include <obelix/Syntax.h>
#include <obelix/arm64/MaterializedSyntaxNode.h>

namespace Obelix {

extern_logging_category(arm64);

class ARM64Context;
using ARM64Implementation = std::function<ErrorOr<void, SyntaxError>(ARM64Context&)>;

class Code {
public:
    explicit Code(std::string prolog = "", std::string epilog = "")
        : m_prolog(std::move(prolog))
        , m_epilog(std::move(epilog))
    {
    }

    template<typename... Args>
    void add_instruction(std::string const& mnemonic, std::string const& param, Args&&... args)
    {
        *m_active = format("{}\t{}\t" + param + '\n', *m_active, mnemonic, std::forward<Args>(args)...);
    }

    void add_instruction(std::string const& mnemonic, std::string const& param = "")
    {
        *m_active = *m_active + '\t' + mnemonic + '\t' + param + '\n';
    }

    void add_text(std::string const& text)
    {
        if (text.empty())
            return;
        auto t = strip(text);
        for (auto const& line : split(strip(text), '\n')) {
            if (line.empty()) {
                *m_active += '\n';
                continue;
            }
            if (line[0] == ';') {
                *m_active += '\t' + line + '\n';
                continue;
            }
            if ((line[0] == '.') || line.ends_with(":")) {
                *m_active += line + '\n';
                continue;
            }
            auto parts = split(strip(line), ' ');
            *m_active += '\t' + parts[0];
            if (parts.size() > 1) {
                for (auto ix = 1u; ix < parts.size(); ++ix) {
                    if (parts[ix].empty())
                        continue;
                    *m_active += '\t' + parts[ix];
                }
            }
            *m_active += '\n';
        }
    }

    void add_label(std::string const& label)
    {
        *m_active = format("{}{}:\n", *m_active, label);
    }

    void add_directive(std::string const& directive, std::string const& args)
    {
        *m_active = format("{}{}\t{}\n", m_active, directive, args);
    }

    void add_comment(std::string const& comment)
    {
        auto c = comment;
        for (auto pos = c.find('\n'); pos != std::string::npos; pos = c.find('\n'))
            c[pos] = ' ';
        *m_active = format("{}\n\t; {}\n", *m_active, c);
    }

    [[nodiscard]] std::string to_string() const
    {
        std::string ret;
        if (!m_prolog.empty())
            ret = m_prolog + "\n";
        ret += m_code;
        if (!m_epilog.empty())
            ret += "\n" + m_epilog;
        return ret;
    }

    [[nodiscard]] bool empty() const
    {
        return m_code.empty();
    }

    [[nodiscard]] bool has_text() const
    {
        return !empty();
    }

    void enter_function(std::string const& name, size_t stack_depth = 0)
    {
        add_directive(".global", name);
        add_label(name);
        add_instruction("stp", "fp,lr,[sp,#-16]!");
        if (stack_depth > 0)
            add_instruction("sub", "sp,sp,#{}", stack_depth);
        add_instruction("mov", "fp,sp");
    }

    void leave_function(size_t stack_depth = 0)
    {
        add_instruction("mov", "sp,fp");
        if (stack_depth > 0)
            add_instruction("add", "sp,sp,#{}", stack_depth);
        add_instruction("ldp", "fp,lr,[sp],16");
        add_instruction("ret");
    }

    void prolog() { m_active = &m_prolog; }
    void epilog() { m_active = &m_epilog; }
    void code() { m_active = &m_code; }

private:
    std::string m_prolog;
    std::string m_code;
    std::string m_epilog;
    std::string* m_active { &m_code };
};

class Assembly {
public:
    explicit Assembly(std::string const& name)
    {
        m_static.prolog();
        m_static.enter_function(format("static_{}", name));
        m_static.epilog();
        m_static.leave_function();
        m_static.code();
        m_current_target = &m_code;
    }

    template<typename... Args>
    void add_instruction(std::string const& mnemonic, std::string const& param, Args&&... args)
    {
        m_current_target->add_instruction(mnemonic, param, std::forward<Args>(args)...);
    }

    void add_instruction(std::string const& mnemonic, std::string const& param = "")
    {
        m_current_target->add_instruction(mnemonic, param);
    }

    void add_text(std::string const& text) { m_current_target->add_text(text); }
    void add_label(std::string const& label) { m_current_target->add_label(label); }
    void add_comment(std::string const& comment) { m_current_target->add_comment(comment); }
    void enter_function(std::string const& name, size_t stack_depth = 0) { m_current_target->enter_function(name, stack_depth); }
    void leave_function(size_t stack_depth = 0) { m_current_target->leave_function(stack_depth); }

    void add_directive(std::string const& directive, std::string const& args)
    {
        if (directive == ".global") {
            m_has_exports = true;
            if (args == "main")
                m_has_main = true;
        }
        m_current_target->add_directive(directive, args);
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

    template <typename Arg>
    void add_data(std::string const& label, bool global, std::string type, bool is_static, Arg const& arg)
    {
        if (m_data.empty())
            m_data = "\n\n.section __DATA,__data\n";
        if (global)
            m_data += format("\n.global {}", label);
        m_data += format("\n.align 8\n{}:\n\t{}\t{}", label, type, arg);
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
        std::string ret = m_code.to_string() + "\n";
        if (m_static.has_text())
            ret += m_static.to_string();
        ret += m_text + "\n" + m_data + "\n";
        return ret;
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
    [[nodiscard]] bool has_main() const { return m_has_main; }

    Code& static_initializer() { return m_static; }
    void target_code() { m_current_target = &m_code; }
    void target_static() { m_current_target = &m_static; };

private:
    Code m_code { ".section	__TEXT,__text,regular,pure_instructions\n\n.align 2\n\n" };
    Code m_static;
    Code* m_current_target;
    std::string m_text;
    std::string m_data;
    bool m_has_exports { false };
    bool m_has_main { false };
    std::unordered_map<std::string, int> m_strings {};
};

struct ARM64ContextPayload {
    ARM64ContextPayload()
        : m_assembly()
    {
    }

    std::shared_ptr<Assembly> m_assembly { nullptr };
    size_t m_stack_allocated { 0 };
    std::vector<size_t> m_stack_depth {};
    static std::vector<std::shared_ptr<MaterializedFunctionDef>> s_function_stack;
    static std::unordered_map<std::string, std::shared_ptr<Assembly>> s_assemblies;
    static unsigned long s_counter;
};

class ARM64Context : public Context<int, ARM64ContextPayload> {
public:
    constexpr static char const* ROOT_MODULE_NAME = "__obelix__root";

    ARM64Context(Config const&);
    ARM64Context(Context<int, ARM64ContextPayload>* parent)
        : Context(parent)
    {
    }

    [[nodiscard]] std::shared_ptr<Assembly> assembly() const
    {
        assert(data().m_assembly);
        return data().m_assembly;
    }

    ErrorOr<void, SyntaxError> zero_initialize(std::shared_ptr<ObjectType> const&, int);
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType> const&, size_t, int);
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType> const&, size_t, int);
    ErrorOr<void, SyntaxError> define_static_storage(std::string const&, std::shared_ptr<ObjectType> const&, bool global, std::shared_ptr<BoundExpression> const& = nullptr);
    ErrorOr<void, SyntaxError> load_immediate(std::shared_ptr<ObjectType> const&, uint64_t, int);

    ErrorOr<void, SyntaxError> enter_function(std::shared_ptr<MaterializedFunctionDef> const& func);
    void function_return() const;
    void leave_function();

    void add_module(std::string const& module)
    {
        if (!data().s_assemblies.contains(module))
            data().s_assemblies[module] = std::make_shared<Assembly>(module);
        data().m_assembly = data().s_assemblies[module];
    }

    [[nodiscard]] static std::unordered_map<std::string, std::shared_ptr<Assembly>> const& assemblies()
    {
        return ARM64ContextPayload::s_assemblies;
    }

    void reserve_on_stack(size_t);
    void release_stack();

    [[nodiscard]] static unsigned long counter()
    {
        return ARM64ContextPayload::s_counter++;
    }

    [[nodiscard]] size_t stack_depth() const
    {
        if (!data().m_stack_depth.empty()) {
            return data().m_stack_depth.back();
        }
        assembly()->add_comment(format("Stack depth empty!"));
        return 0;
    }

protected:
    void stack_depth(size_t depth)
    {
        data().m_stack_depth.push_back(depth);
        assembly()->add_comment(format("Set stack depth to {}", stack_depth()));
    }

    void pop_stack_depth()
    {
        auto current = data().m_stack_depth.back();
        data().m_stack_depth.pop_back();
        if (data().m_stack_depth.empty()) {
            assembly()->add_comment(format("Stack depth popped. Was {}, now empty", current));
        } else {
            assembly()->add_comment(format("Stack depth popped. Was {}, now {}", current, stack_depth()));
        }
    }
};

template<>
inline ARM64Context& make_subcontext(ARM64Context& ctx)
{
    return dynamic_cast<ARM64Context&>(ctx.make_subcontext());
}

template<typename T = long>
static inline void push(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly()->add_instruction("str", "{},[sp,-16]!", reg);
}

template<>
[[maybe_unused]] void push<uint8_t>(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly()->add_instruction("strb", "{},[sp,-16]!", reg);
}

template<typename T = long>
static inline void pop(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly()->add_instruction("ldr", "{},[sp],16", reg);
}

template<>
[[maybe_unused]] void pop<uint8_t>(ARM64Context& ctx, std::string const& reg)
{
    ctx.assembly()->add_instruction("ldrb", "{},[sp],16", reg);
}

#if 0
template<typename T = long>
static inline void push_imm(ARM64Context& ctx, T value)
{
    ctx.new_temporary_context();
    ctx.assembly()->add_instruction("mov", "x{},{}", ctx.get_register(), value);
    push(ctx, format("x{}", ctx.get_register()));
    ctx.release_register_context();
}

template<>
[[maybe_unused]] void push_imm(ARM64Context& ctx, uint8_t value)
{
    ctx.new_temporary_context();
    ctx.assembly()->add_instruction("movb", "w{},{}", ctx.get_register(), value);
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
    ctx.assembly()->add_instruction("ldr", "x{},[fp,#{}]", ctx.get_register(), idx);
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
    ctx.assembly()->add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_register(), idx);
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
    ctx.assembly()->add_instruction("ldr", "x{},[fp,#{}]", ctx.get_register(), idx);
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
    ctx.assembly()->add_instruction("ldrb", "w{},[fp,#{}]", ctx.get_register(), idx);
    ctx.release_register_context();
    return {};
}
#endif

}
