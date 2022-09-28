/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/Function.h>
#include <obelix/boundsyntax/Expression.h>
#include <obelix/boundsyntax/Variable.h>

namespace Obelix {

NODE_CLASS(BoundFunctionDecl, Statement)
public:
    BoundFunctionDecl(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    BoundFunctionDecl(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] std::string type_name() const;
    [[nodiscard]] BoundIdentifiers const& parameters() const;
    [[nodiscard]] ObjectTypes parameter_types() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

protected:
    explicit BoundFunctionDecl(std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    [[nodiscard]] std::string parameters_to_string() const;

private:
    std::shared_ptr<BoundIdentifier> m_identifier;
    BoundIdentifiers m_parameters;
};

NODE_CLASS(BoundNativeFunctionDecl, BoundFunctionDecl)
public:
    BoundNativeFunctionDecl(std::shared_ptr<NativeFunctionDecl> const&, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    BoundNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const&, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    [[nodiscard]] std::string const& native_function_name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_native_function_name;
};

NODE_CLASS(BoundIntrinsicDecl, BoundFunctionDecl)
public:
    BoundIntrinsicDecl(std::shared_ptr<SyntaxNode> const&, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    BoundIntrinsicDecl(std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    explicit BoundIntrinsicDecl(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(BoundFunctionDef, Statement)
public:
    BoundFunctionDef(std::shared_ptr<FunctionDef> const&, std::shared_ptr<BoundFunctionDecl>, std::shared_ptr<Statement> = nullptr);
    BoundFunctionDef(Token token, std::shared_ptr<BoundFunctionDecl>, std::shared_ptr<Statement> = nullptr);
    explicit BoundFunctionDef(std::shared_ptr<BoundFunctionDef> const&, std::shared_ptr<Statement> = nullptr);
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const;
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] BoundIdentifiers const& parameters() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

protected:
    std::shared_ptr<BoundFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
};

NODE_CLASS(BoundFunctionCall, BoundExpression)
public:
    BoundFunctionCall(std::shared_ptr<FunctionCall> const&, BoundExpressions, std::shared_ptr<BoundFunctionDecl>);
    BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const&, BoundExpressions, std::shared_ptr<BoundFunctionDecl> const& = nullptr);
    BoundFunctionCall(Token token, std::shared_ptr<BoundFunctionDecl> const&, BoundExpressions);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] BoundExpressions const& arguments() const;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const;
    [[nodiscard]] ObjectTypes argument_types() const;

private:
    std::string m_name;
    BoundExpressions m_arguments;
    std::shared_ptr<BoundFunctionDecl> m_declaration;
};

NODE_CLASS(BoundNativeFunctionCall, BoundFunctionCall)
public:
    BoundNativeFunctionCall(std::shared_ptr<FunctionCall> const&, BoundExpressions, std::shared_ptr<BoundNativeFunctionDecl>);
    BoundNativeFunctionCall(std::shared_ptr<BoundNativeFunctionCall> const&, BoundExpressions, std::shared_ptr<BoundFunctionDecl> const& = nullptr);
};

NODE_CLASS(BoundIntrinsicCall, BoundFunctionCall)
public:
    BoundIntrinsicCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundIntrinsicDecl> decl, IntrinsicType intrinsic);
    BoundIntrinsicCall(std::shared_ptr<BoundIntrinsicCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundIntrinsicDecl> const& decl = nullptr);
    BoundIntrinsicCall(Token token, std::shared_ptr<BoundIntrinsicDecl> const& decl, BoundExpressions arguments, IntrinsicType intrinsic_type);
    [[nodiscard]] IntrinsicType intrinsic() const;

private:
    IntrinsicType m_intrinsic;
};

}
