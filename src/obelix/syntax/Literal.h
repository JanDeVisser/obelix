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

ABSTRACT_NODE_CLASS(Literal, Expression)
protected:
    Literal(Token const&, std::shared_ptr<ExpressionType>);
    Literal(Token const&, std::shared_ptr<ObjectType> const&);

public:
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] virtual std::shared_ptr<Expression> apply(Token const&);
};

NODE_CLASS(IntLiteral, Literal)
public:
    explicit IntLiteral(Token const&);
    IntLiteral(Token const&, std::shared_ptr<ExpressionType>);
    IntLiteral(Token const&, std::shared_ptr<ObjectType> const&);
    [[nodiscard]] std::shared_ptr<Expression> apply(Token const&) override;
};

NODE_CLASS(CharLiteral, Literal)
public:
    explicit CharLiteral(Token const&);
    CharLiteral(Token const&, std::shared_ptr<ExpressionType>);
    CharLiteral(Token const&, std::shared_ptr<ObjectType> const&);
};

NODE_CLASS(FloatLiteral, Literal)
public:
    explicit FloatLiteral(Token const&);
    FloatLiteral(Token const&, std::shared_ptr<ExpressionType>);
    FloatLiteral(Token const&, std::shared_ptr<ObjectType> const&);
    [[nodiscard]] std::shared_ptr<Expression> apply(Token const&) override;
};

NODE_CLASS(StringLiteral, Literal)
public:
    explicit StringLiteral(Token const& t);
    StringLiteral(Token const&, std::shared_ptr<ExpressionType>);
    StringLiteral(Token const&, std::shared_ptr<ObjectType> const&);
};

NODE_CLASS(BooleanLiteral, Literal)
public:
    explicit BooleanLiteral(Token const&);
    BooleanLiteral(Token const&, std::shared_ptr<ExpressionType>);
    BooleanLiteral(Token const&, std::shared_ptr<ObjectType> const&);
    [[nodiscard]] std::shared_ptr<Expression> apply(Token const&) override;
};

}
