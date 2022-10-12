/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/Variable.h>
#include <obelix/boundsyntax/Expression.h>

namespace Obelix {

ABSTRACT_NODE_CLASS(BoundVariableAccess, BoundExpression)
public:
    BoundVariableAccess(std::shared_ptr<Expression>, std::shared_ptr<ObjectType>);
    BoundVariableAccess(Token, std::shared_ptr<ObjectType>);
};

NODE_CLASS(BoundIdentifier, BoundVariableAccess)
public:
    BoundIdentifier(std::shared_ptr<Identifier> const&, std::shared_ptr<ObjectType>);
    BoundIdentifier(Token token, std::string, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_identifier;
};

using BoundIdentifiers = std::vector<std::shared_ptr<BoundIdentifier>>;

NODE_CLASS(BoundVariable, BoundIdentifier)
public:
    BoundVariable(std::shared_ptr<Variable>, std::shared_ptr<ObjectType>);
    BoundVariable(Token, std::string, std::shared_ptr<ObjectType>);
};

NODE_CLASS(BoundMemberAccess, BoundVariableAccess)
public:
    BoundMemberAccess(std::shared_ptr<BoundExpression>, std::shared_ptr<BoundIdentifier>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& structure() const;
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& member() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundExpression> m_struct;
    std::shared_ptr<BoundIdentifier> m_member;
};

NODE_CLASS(BoundArrayAccess, BoundVariableAccess)
public:
    BoundArrayAccess(std::shared_ptr<BoundExpression>, std::shared_ptr<BoundExpression>, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& array() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& subscript() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundExpression> m_array;
    std::shared_ptr<BoundExpression> m_index;
};

NODE_CLASS(BoundVariableDeclaration, Statement)
public:
    explicit BoundVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    explicit BoundVariableDeclaration(Token, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] bool is_const() const;
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
    BoundStaticVariableDeclaration(Token, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(BoundLocalVariableDeclaration, BoundVariableDeclaration)
public:
    BoundLocalVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    BoundLocalVariableDeclaration(Token, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(BoundGlobalVariableDeclaration, BoundVariableDeclaration)
public:
    BoundGlobalVariableDeclaration(std::shared_ptr<VariableDeclaration> const&, std::shared_ptr<BoundIdentifier>, std::shared_ptr<BoundExpression>);
    BoundGlobalVariableDeclaration(Token, std::shared_ptr<BoundIdentifier>, bool, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string to_string() const override;
};


NODE_CLASS(BoundAssignment, BoundExpression)
public:
    BoundAssignment(Token token, std::shared_ptr<BoundVariableAccess> assignee, std::shared_ptr<BoundExpression> expression);
    [[nodiscard]] std::shared_ptr<BoundVariableAccess> const& assignee() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundVariableAccess> m_assignee;
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundFunctionDecl;
using BoundFunctionDecls = std::vector<std::shared_ptr<BoundFunctionDecl>>;

NODE_CLASS(BoundModule, BoundExpression)
public:
    BoundModule(Token, std::string, std::shared_ptr<Block>, BoundFunctionDecls, BoundFunctionDecls);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<Block> const& block() const;
    [[nodiscard]] BoundFunctionDecls const& exports() const;
    [[nodiscard]] BoundFunctionDecls const& imports() const;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> exported(std::string const&);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> resolve(std::string const&, ObjectTypes const&) const;

private:
    std::string m_name;
    std::shared_ptr<Block> m_block;
    BoundFunctionDecls m_exports;
    BoundFunctionDecls m_imports;
};

using BoundModules = std::vector<std::shared_ptr<BoundModule>>;

NODE_CLASS(BoundCompilation, BoundExpression)
public:
    BoundCompilation(BoundModules);
    [[nodiscard]] BoundModules const& modules() const;
    [[nodiscard]] std::shared_ptr<BoundModule> const& root() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string root_to_xml() const;

private:
    BoundModules m_modules;
    std::shared_ptr<BoundModule> m_root;
};



}
