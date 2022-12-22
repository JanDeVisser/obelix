/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(FunctionDecl, Statement)
public:
    FunctionDecl(Span, std::string, std::shared_ptr<Identifier>, Identifiers);
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const;
    [[nodiscard]] std::string const& module() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ExpressionType> type() const;
    [[nodiscard]] std::string type_name() const;
    [[nodiscard]] Identifiers const& parameters() const;
    [[nodiscard]] ExpressionTypes parameter_types() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

protected:
    [[nodiscard]] std::string parameters_to_string() const;

private:
    std::string m_module;
    std::shared_ptr<Identifier> m_identifier;
    Identifiers m_parameters;
};

NODE_CLASS(NativeFunctionDecl, FunctionDecl)
public:
    NativeFunctionDecl(Span, std::string, std::shared_ptr<Identifier>, Identifiers, std::string);
    [[nodiscard]] std::string const& native_function_name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_native_function_name;
};

NODE_CLASS(IntrinsicDecl, FunctionDecl)
public:
    IntrinsicDecl(Span, std::string, std::shared_ptr<Identifier>, Identifiers);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(FunctionDef, Statement)
public:
    FunctionDef(Span, std::shared_ptr<FunctionDecl>, std::shared_ptr<Statement> = nullptr);
    [[nodiscard]] std::shared_ptr<FunctionDecl> const& declaration() const;
    [[nodiscard]] std::shared_ptr<Identifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ExpressionType> type() const;
    [[nodiscard]] Identifiers const& parameters() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

protected:
    std::shared_ptr<FunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
};

}
