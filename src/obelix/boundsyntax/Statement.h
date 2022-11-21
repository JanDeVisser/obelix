/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Forward.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(BoundStatement, Statement)
public:
    BoundStatement() = default;
    explicit BoundStatement(Token);
    [[nodiscard]] bool is_fully_bound() const override { return true; }
};

NODE_CLASS(BoundExpression, SyntaxNode)
public:
    BoundExpression() = default;
    BoundExpression(Token, std::shared_ptr<ObjectType>);
    BoundExpression(std::shared_ptr<Expression> const&, std::shared_ptr<ObjectType>);
    BoundExpression(std::shared_ptr<BoundExpression> const&);
    BoundExpression(Token, PrimitiveType);
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const;
    [[nodiscard]] std::string const& type_name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] virtual std::string qualified_name() const;

private:
    std::shared_ptr<ObjectType> m_type { nullptr };
};

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
    [[nodiscard]] bool is_fully_bound() const;
    [[nodiscard]] int unbound_statements() const;
    [[nodiscard]] std::string qualified_name() const override;

private:
    std::string m_name;
    std::shared_ptr<Block> m_block;
    BoundFunctionDecls m_exports;
    BoundFunctionDecls m_imports;
};

NODE_CLASS(BoundCompilation, BoundExpression)
public:
    BoundCompilation(BoundModules, ObjectTypes const&, std::string);
    BoundCompilation(BoundModules, BoundTypes, std::string);
    [[nodiscard]] BoundModules const& modules() const;
    [[nodiscard]] BoundTypes const& custom_types() const;
    [[nodiscard]] std::shared_ptr<BoundModule> const& root() const;
    [[nodiscard]] std::shared_ptr<BoundModule> const& main() const;
    [[nodiscard]] std::string const& main_module() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string root_to_xml() const;
    [[nodiscard]] bool is_fully_bound() const;
    [[nodiscard]] int unbound_statements() const;

private:
    BoundModules m_modules;
    BoundTypes m_custom_types;
    std::string m_main_module;
    std::shared_ptr<BoundModule> m_root;
    std::shared_ptr<BoundModule> m_main;
};

}
