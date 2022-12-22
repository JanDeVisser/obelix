/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Literal.h>
#include <obelix/boundsyntax/Expression.h>

namespace Obelix {

ABSTRACT_NODE_CLASS(BoundLiteral, BoundExpression)
public:
    BoundLiteral(Token, pObjectType);
    [[nodiscard]] virtual long int_value() const;
    [[nodiscard]] virtual std::string string_value() const;
    [[nodiscard]] virtual bool bool_value() const;
    [[nodiscard]] Token const& token() const;

private:
    Token m_token;
};

using BoundLiterals = std::vector<std::shared_ptr<BoundLiteral>>;

NODE_CLASS(BoundIntLiteral, BoundLiteral)
public:
    explicit BoundIntLiteral(std::shared_ptr<IntLiteral> const& literal, std::shared_ptr<ObjectType> type = nullptr);
    BoundIntLiteral(std::shared_ptr<BoundIntLiteral> const& literal, std::shared_ptr<ObjectType> type = nullptr);
    BoundIntLiteral(Span, long, std::shared_ptr<ObjectType>);
    BoundIntLiteral(Span, unsigned long, std::shared_ptr<ObjectType>);
    BoundIntLiteral(Span, long);
    BoundIntLiteral(Span, unsigned long);
    BoundIntLiteral(Span, int);
    BoundIntLiteral(Span, unsigned);
    BoundIntLiteral(Span, short);
    BoundIntLiteral(Span, unsigned char);
    BoundIntLiteral(Span, signed char);
    BoundIntLiteral(Span, unsigned short);
    [[nodiscard]] ErrorOr<std::shared_ptr<BoundIntLiteral>, SyntaxError> cast(std::shared_ptr<ObjectType> const&) const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] long int_value() const override;

    template<typename IntType = int>
    [[nodiscard]] IntType value() const { return static_cast<IntType>(m_int); }

private:
    long m_int;
};

NODE_CLASS(BoundStringLiteral, BoundLiteral)
public:
    explicit BoundStringLiteral(std::shared_ptr<StringLiteral> const&);
    BoundStringLiteral(Span, std::string);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string const& value() const;
    [[nodiscard]] std::string string_value() const override;

private:
    std::string m_string;
};

NODE_CLASS(BoundBooleanLiteral, BoundLiteral)
public:
    explicit BoundBooleanLiteral(std::shared_ptr<BooleanLiteral> const&);
    BoundBooleanLiteral(Span, bool);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] bool value() const;
    [[nodiscard]] bool bool_value() const override;

private:
    bool m_value;
};

NODE_CLASS(BoundTypeLiteral, BoundLiteral)
public:
    BoundTypeLiteral(Span, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<ObjectType> const& value() const;

private:
    std::shared_ptr<ObjectType> m_type_value;
};

NODE_CLASS(BoundEnumValue, BoundExpression)
public:
    BoundEnumValue(Span, std::shared_ptr<ObjectType>, std::string, long);
    [[nodiscard]] long const& value() const;
    [[nodiscard]] std::string const& label() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    long m_value;
    std::string m_label;
};

NODE_CLASS(BoundModuleLiteral, BoundExpression)
public:
    BoundModuleLiteral(std::shared_ptr<Variable> const&);
    BoundModuleLiteral(Span, std::string);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_name;
};

}
