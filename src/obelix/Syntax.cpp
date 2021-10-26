//
// Created by Jan de Visser on 2021-09-18.
//

#include <obelix/Syntax.h>

std::shared_ptr<SyntaxNode> Block::evaluate(std::shared_ptr<Object> scope, bool must_resolve) const
{
    std::shared_ptr<SyntaxNode> ret = std::make_shared<Literal>(Object::null());
    for (auto& child : m_children) {
        ret = child->evaluate(scope, must_resolve);
    }
    return ret;
}

