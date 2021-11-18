//
// Created by Jan de Visser on 2021-10-11.
//

#pragma once

#include <core/Arguments.h>
#include <core/Dictionary.h>
#include <core/Object.h>
#include <lexer/Token.h>
#include <string>

namespace Obelix {

enum class ExecutionResultCode {
    None,
    Break,
    Continue,
    Return,
    Skipped,
    Error
};

struct ExecutionResult {
    ExecutionResultCode code { ExecutionResultCode::None };
    Obj return_value;
};

struct Config {
    bool show_tree { false };
};

class Scope : public Object {
public:
    Scope()
        : Scope(make_null<Scope>())
    {
    }

    explicit Scope(Ptr<Scope> const& parent);

    void declare(std::string const& name, Obj const& value);
    [[nodiscard]] bool contains(std::string const& name) const { return m_variables.contains(name); }
    void set(std::string const& name, Obj const& value);
    [[nodiscard]] std::optional<Obj> resolve(std::string const&) const override;
    [[nodiscard]] std::optional<Obj> assign(std::string const&, Obj const&) override;
    [[nodiscard]] Ptr<Object> copy() const override;
    virtual Ptr<Scope> import_module(std::string const& module_name) { assert(m_parent); return m_parent->import_module(module_name); }
    [[nodiscard]] virtual Config const& config() const { assert(m_parent); return m_parent->config(); }
    Ptr<Scope> eval(std::string const& src);

    [[nodiscard]] std::string to_string() const override { return "scope"; }
    [[nodiscard]] ExecutionResult const& result() const { return m_result; }
    void set_result(ExecutionResult const& result) { m_result = result; }

private:
    Ptr<Scope> m_parent;
    Dictionary m_variables {};
    ExecutionResult m_result {};
};

}