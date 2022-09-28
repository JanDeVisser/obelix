/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>

namespace Obelix {

ABSTRACT_NODE_CLASS(Statement, SyntaxNode)
public:
    Statement() = default;
    explicit Statement(Token);
};

using Statements = std::vector<std::shared_ptr<Statement>>;

NODE_CLASS(Import, Statement)
public:
    Import(Token, std::string);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string const& name() const;

private:
    std::string m_name;
};

NODE_CLASS(Pass, Statement)
public:
    explicit Pass(Token);
    explicit Pass(std::shared_ptr<Statement> elided_statement);
    [[nodiscard]] std::string text_contents() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<Statement> m_elided_statement;
};

class Goto;

NODE_CLASS(Label, Statement)
public:
    explicit Label(Token = {});
    explicit Label(std::shared_ptr<Goto> const&);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] int label_id() const;
    static int reserve_id();

private:
    static int m_current_id;
    int m_label_id;
};

NODE_CLASS(Goto, Statement)
public:
    explicit Goto(Token = {}, std::shared_ptr<Label> const& = nullptr);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] int label_id() const;

private:
    int m_label_id;
};

NODE_CLASS(Block, Statement)
public:
    explicit Block(Token, Statements);
    [[nodiscard]] Statements const& statements() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

protected:
    Statements m_statements {};
};

NODE_CLASS(Module, Block)
public:
    Module(Statements const&, std::string);
    Module(Token, Statements const&, std::string);
    Module(std::shared_ptr<Module> const&, Statements const&);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] const std::string& name() const;

private:
    std::string m_name;
};

using Modules = std::vector<std::shared_ptr<Module>>;

NODE_CLASS(Compilation, Module)
public:
    Compilation(std::shared_ptr<Module> const& root, Modules);
    Compilation(Statements const&, Modules);
    [[nodiscard]] Modules const& modules() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string root_to_xml() const;

private:
    Modules m_modules;
};

NODE_CLASS(ExpressionStatement, Statement)
public:
    explicit ExpressionStatement(std::shared_ptr<Expression> expression);
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<Expression> m_expression;
};

NODE_CLASS(Return, Statement)
public:
    Return(Token, std::shared_ptr<Expression>);
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const;

private:
    std::shared_ptr<Expression> m_expression;
};

}
