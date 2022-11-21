/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>
#include <obelix/syntax/Expression.h>

namespace Obelix {

ABSTRACT_NODE_CLASS(Literal, Expression)
protected:
    Literal(Token, std::shared_ptr<ExpressionType> = nullptr);

public:
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] virtual std::shared_ptr<Expression> apply(Token const&);
};

NODE_CLASS(IntLiteral, Literal)
public:
    IntLiteral(Token, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::shared_ptr<Expression> apply(Token const&) override;
};

NODE_CLASS(CharLiteral, Literal)
public:
    CharLiteral(Token, std::shared_ptr<ExpressionType> = nullptr);
};

NODE_CLASS(FloatLiteral, Literal)
public:
    FloatLiteral(Token, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::shared_ptr<Expression> apply(Token const&) override;
};

NODE_CLASS(StringLiteral, Literal)
public:
    StringLiteral(Token, std::shared_ptr<ExpressionType> = nullptr);
};

NODE_CLASS(BooleanLiteral, Literal)
public:
    BooleanLiteral(Token, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::shared_ptr<Expression> apply(Token const&) override;
};

}
