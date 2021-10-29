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

class Scope : public Object {
public:
    explicit Scope(Ptr<Scope> parent);

    void declare(std::string const& name, Obj value);
    void set(std::string const& name, Obj value);
    [[nodiscard]] std::optional<Obj> resolve(std::string const&) const override;
    [[nodiscard]] std::optional<Obj> assign(std::string const&, Obj const&) override;
    [[nodiscard]] Ptr<Scope> clone();

    [[nodiscard]] std::string to_string() const override { return "scope"; }
    [[nodiscard]] ExecutionResult const& result() const { return m_result; }

private:
    Ptr<Scope> m_parent;
    Dictionary m_variables {};
    ExecutionResult m_result {};
};

}