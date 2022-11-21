/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

// -- SyntaxNode ------------------------------------------------------------

SyntaxNode::SyntaxNode(Token token)
    : m_token(std::move(token))
{
}

std::string SyntaxNode::text_contents() const
{
    return "";
}

std::string SyntaxNode::attributes() const
{
    return "";
}

Nodes SyntaxNode::children() const
{
    return Nodes {};
}

std::string SyntaxNode::to_xml(int indent) const
{
    std::string ret = "";
    while (ret.length() < indent)
        ret += ' ';
    ret += format("<{}", node_type());
    auto attrs = attributes();
    auto child_nodes = children();
    auto text = text_contents();
    if (!attrs.empty()) {
        ret += " " + attrs;
    }
    if (text.empty() && child_nodes.empty())
        return ret + "/>";
    ret += ">\n";
    for (auto& child : child_nodes) {
        ret += child->to_xml(indent + 2);
        ret += "\n";
    }
    ret += text;
    for (auto ix = 0; ix < indent; ix++)
        ret += ' ';
    return ret + format("</{}>", node_type());
}

std::string SyntaxNode::to_string() const
{
    return SyntaxNodeType_name(node_type());
}

std::string SyntaxNode::to_xml() const
{
    return to_xml(0);
}

Token const& SyntaxNode::token() const
{
    return m_token;
}

}
