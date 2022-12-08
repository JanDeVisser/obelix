/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Function.h>
#include <obelix/boundsyntax/ControlFlow.h>
#include <obelix/boundsyntax/Expression.h>
#include <obelix/boundsyntax/Variable.h>

namespace Obelix {

NODE_CLASS(BoundFunctionDecl, BoundStatement)
public:
    BoundFunctionDecl(std::shared_ptr<SyntaxNode> const&, std::string, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    BoundFunctionDecl(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] std::string const& module() const;
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
    explicit BoundFunctionDecl(std::string, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    [[nodiscard]] std::string parameters_to_string() const;

private:
    std::string m_module;
    std::shared_ptr<BoundIdentifier> m_identifier;
    BoundIdentifiers m_parameters;
};

using BoundFunctionDecls = std::vector<std::shared_ptr<BoundFunctionDecl>>;

NODE_CLASS(BoundNativeFunctionDecl, BoundFunctionDecl)
public:
    BoundNativeFunctionDecl(std::shared_ptr<NativeFunctionDecl> const&, std::string, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    BoundNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const&, std::string, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    [[nodiscard]] std::string const& native_function_name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_native_function_name;
};

NODE_CLASS(BoundIntrinsicDecl, BoundFunctionDecl)
public:
    BoundIntrinsicDecl(std::shared_ptr<SyntaxNode> const&, std::string, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    BoundIntrinsicDecl(std::string, std::shared_ptr<BoundIdentifier>, BoundIdentifiers);
    explicit BoundIntrinsicDecl(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(FunctionBlock, Block)
public:
    FunctionBlock(Token, Statements, pBoundFunctionDecl);
    FunctionBlock(Token, pStatement, pBoundFunctionDecl);
    [[nodiscard]] pBoundFunctionDecl const& declaration() const;

private:
    pBoundFunctionDecl m_declaration;
};

NODE_CLASS(BoundFunctionDef, BoundStatement)
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
    [[nodiscard]] bool is_fully_bound() const override;

protected:
    std::shared_ptr<BoundFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
};

NODE_CLASS(BoundFunctionCall, BoundExpression)
public:
    BoundFunctionCall(Token, std::shared_ptr<BoundFunctionDecl> const&, BoundExpressions);
    BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const&, BoundExpressions, std::shared_ptr<BoundFunctionDecl> const& = nullptr);
    BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const&, pObjectType = nullptr);
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
    BoundNativeFunctionCall(Token, std::shared_ptr<BoundNativeFunctionDecl> const&, BoundExpressions);
    BoundNativeFunctionCall(std::shared_ptr<BoundNativeFunctionCall> const&, BoundExpressions, std::shared_ptr<BoundFunctionDecl> const& = nullptr);
    BoundNativeFunctionCall(std::shared_ptr<BoundNativeFunctionCall> const&, pObjectType = nullptr);
};

NODE_CLASS(BoundIntrinsicCall, BoundFunctionCall)
public:
    BoundIntrinsicCall(Token, std::shared_ptr<BoundIntrinsicDecl> const&, BoundExpressions, IntrinsicType);
    BoundIntrinsicCall(std::shared_ptr<BoundIntrinsicCall> const&, BoundExpressions, std::shared_ptr<BoundIntrinsicDecl> const& decl = nullptr);
    BoundIntrinsicCall(std::shared_ptr<BoundIntrinsicCall> const&, pObjectType = nullptr);
    [[nodiscard]] IntrinsicType intrinsic() const;

private:
    IntrinsicType m_intrinsic;
};

ABSTRACT_NODE_CLASS(BoundFunction, BoundExpression)
public:
    BoundFunction(Token, std::shared_ptr<BoundFunctionDecl>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const;

private:
    std::shared_ptr<BoundFunctionDecl> m_declaration;
};

NODE_CLASS(BoundLocalFunction, BoundFunction)
public:
    BoundLocalFunction(Token, std::shared_ptr<BoundFunctionDecl>);
};

NODE_CLASS(BoundImportedFunction, BoundFunction)
public:
    BoundImportedFunction(Token, std::shared_ptr<BoundModule>, std::shared_ptr<BoundFunctionDecl>);
    [[nodiscard]] std::shared_ptr<BoundModule> const& module() const;

private:
    std::shared_ptr<BoundModule> m_module;
};

}
