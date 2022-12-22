/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Variable.h>
#include <obelix/boundsyntax/ControlFlow.h>
#include <obelix/boundsyntax/Expression.h>
#include <obelix/boundsyntax/Function.h>
#include <obelix/boundsyntax/Typedef.h>

namespace Obelix {

ABSTRACT_NODE_CLASS(BoundVariableAccess, BoundExpression)
public:
    BoundVariableAccess(std::shared_ptr<Expression>, std::shared_ptr<ObjectType>);
    BoundVariableAccess(Span, std::shared_ptr<ObjectType>);
};

NODE_CLASS(BoundIdentifier, BoundVariableAccess)
public:
    BoundIdentifier(std::shared_ptr<Identifier> const&, std::shared_ptr<ObjectType>);
    BoundIdentifier(Span location, std::string, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string qualified_name() const override;

private:
    std::string m_identifier;
};

NODE_CLASS(BoundVariable, BoundIdentifier)
public:
    BoundVariable(std::shared_ptr<Variable>, std::shared_ptr<ObjectType>);
    BoundVariable(Span, std::string, std::shared_ptr<ObjectType>);
};

NODE_CLASS(BoundMemberAccess, BoundVariableAccess)
public:
    BoundMemberAccess(std::shared_ptr<BoundExpression>, std::shared_ptr<BoundIdentifier>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& structure() const;
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& member() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string qualified_name() const override;

private:
    std::shared_ptr<BoundExpression> m_struct;
    std::shared_ptr<BoundIdentifier> m_member;
};

NODE_CLASS(UnboundMemberAccess, Expression)
public:
    UnboundMemberAccess(std::shared_ptr<BoundExpression>, std::shared_ptr<Variable>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& structure() const;
    [[nodiscard]] std::shared_ptr<Variable> const& member() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundExpression> m_struct;
    std::shared_ptr<Variable> m_member;
};

NODE_CLASS(BoundMemberAssignment, BoundMemberAccess)
public:
    BoundMemberAssignment(std::shared_ptr<BoundExpression>, std::shared_ptr<BoundIdentifier>);
};

NODE_CLASS(BoundArrayAccess, BoundVariableAccess)
public:
    BoundArrayAccess(std::shared_ptr<BoundExpression>, std::shared_ptr<BoundExpression>, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& array() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& subscript() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string qualified_name() const override;

private:
    std::shared_ptr<BoundExpression> m_array;
    std::shared_ptr<BoundExpression> m_index;
};

NODE_CLASS(BoundVariableDeclaration, BoundStatement)
public:
    explicit BoundVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    explicit BoundVariableDeclaration(Span, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] virtual bool is_static() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& variable() const;

private:
    std::shared_ptr<BoundIdentifier> m_variable;
    bool m_const { false };
    std::shared_ptr<BoundExpression> m_expression;
};

NODE_CLASS(BoundStaticVariableDeclaration, BoundVariableDeclaration)
public:
    BoundStaticVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    BoundStaticVariableDeclaration(Span, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] bool is_static() const override;
};

NODE_CLASS(BoundLocalVariableDeclaration, BoundVariableDeclaration)
public:
    BoundLocalVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    BoundLocalVariableDeclaration(Span, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(BoundGlobalVariableDeclaration, BoundVariableDeclaration)
public:
    BoundGlobalVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    BoundGlobalVariableDeclaration(Span, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] bool is_static() const override;
};


NODE_CLASS(BoundAssignment, BoundExpression)
public:
    BoundAssignment(Span location, std::shared_ptr<BoundVariableAccess> assignee, std::shared_ptr<BoundExpression> expression);
    [[nodiscard]] std::shared_ptr<BoundVariableAccess> const& assignee() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundVariableAccess> m_assignee;
    std::shared_ptr<BoundExpression> m_expression;
};

}
