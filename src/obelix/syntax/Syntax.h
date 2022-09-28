/*
* Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
*
* SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <cstdarg>
#include <memory>
#include <string>
#include <vector>

#include <core/Logging.h>
#include <lexer/Token.h>
#include <obelix/Type.h>
#include <obelix/SyntaxNodeType.h>

namespace Obelix {

#define ABSTRACT_NODE_CLASS(name, derives_from) \
    class name                                  \
        : public derives_from {                 \

#define NODE_CLASS(name, derives_from, ...)                     \
    class name                                                  \
        : public derives_from __VA_OPT__(,) __VA_ARGS__ {       \
    public:                                                     \
        [[nodiscard]] SyntaxNodeType node_type() const override \
        {                                                       \
            return SyntaxNodeType::name;                        \
        }                                                       \
                                                                \
    private:

class SyntaxNode;
using Nodes = std::vector<std::shared_ptr<SyntaxNode>>;
using Strings = std::vector<std::string>;

class SyntaxNode : public std::enable_shared_from_this<SyntaxNode> {
public:
    explicit SyntaxNode(Token token = {});
    virtual ~SyntaxNode() = default;

    [[nodiscard]] virtual SyntaxNodeType node_type() const = 0;
    [[nodiscard]] virtual std::string text_contents() const;
    [[nodiscard]] virtual std::string attributes() const;
    [[nodiscard]] virtual Nodes children() const;
    [[nodiscard]] std::string to_xml(int) const;
    [[nodiscard]] virtual std::string to_string() const;
    [[nodiscard]] std::string to_xml() const;
    [[nodiscard]] Token const& token() const;

private:
    Token m_token;
};

template<class T, class... Args>
std::shared_ptr<T> make_node(Args&&... args)
{
    auto ret = std::make_shared<T>(std::forward<Args>(args)...);
    debug(parser, "{}: {}", SyntaxNodeType_name(ret->node_type()), ret->to_string());
    return ret;
}

template<>
struct Converter<SyntaxNode*> {
    static std::string to_string(SyntaxNode* const val)
    {
        return val->to_string();
    }

    static double to_double(SyntaxNode* const)
    {
        return NAN;
    }

    static unsigned long to_long(SyntaxNode* const)
    {
        return 0;
    }
};

}
