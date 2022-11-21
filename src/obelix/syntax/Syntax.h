/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdarg>
#include <memory>
#include <string>
#include <vector>

#include <core/Logging.h>
#include <lexer/Token.h>
#include <obelix/SyntaxNodeType.h>
#include <obelix/Type.h>

namespace Obelix {

#define ABSTRACT_NODE_CLASS(name, derives_from) \
    class name;                                 \
    using p##name = std::shared_ptr<name>;      \
    using name##s = std::vector<p##name>;       \
    class name                                  \
        : public derives_from {

#define NODE_CLASS(name, derives_from, ...)                     \
    class name;                                                 \
    using p##name = std::shared_ptr<name>;                      \
    using name##s = std::vector<p##name>;                       \
    class name                                                  \
        : public derives_from                                   \
          __VA_OPT__(, ) __VA_ARGS__ {                          \
    public:                                                     \
        [[nodiscard]] SyntaxNodeType node_type() const override \
        {                                                       \
            return SyntaxNodeType::name;                        \
        }                                                       \
                                                                \
    private:

#define NODE_CLASS_TEMPLATE(name, derives_from, ...)            \
    class name                                                  \
        : public derives_from                                   \
          __VA_OPT__(, ) __VA_ARGS__ {                          \
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

using pSyntaxNode = std::shared_ptr<SyntaxNode>;
using SyntaxNodes = Nodes;

template<class NodeClass>
NODE_CLASS_TEMPLATE(NodeList, SyntaxNode, public std::vector<std::shared_ptr<NodeClass>>)
public :

    explicit NodeList(std::string tag)
    : SyntaxNode()
    , m_tag(std::move(tag))
{
}

NodeList(std::string tag, std::vector<std::shared_ptr<NodeClass>> nodes)
    : SyntaxNode()
    , std::vector<std::shared_ptr<NodeClass>>(std::move(nodes))
    , m_tag(std::move(tag))
{
}

[[nodiscard]] std::string const& tag() const { return m_tag; }

[[nodiscard]] std::string attributes() const override
{
    return format(R"(tag="{}")", tag());
}

[[nodiscard]] Nodes children() const override
{
    Nodes ret;
    for (auto const& element : *this) {
        ret.push_back(element);
    }
    return ret;
}

private:
std::string m_tag;
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
