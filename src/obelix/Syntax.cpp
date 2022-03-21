/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Logging.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/BoundFunction.h>
#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

int Label::m_current_id = 0;

SyntaxNode::SyntaxNode(Token token)
    : m_token(std::move(token))
{
}

Label::Label(std::shared_ptr<Goto> const& goto_stmt)
    : Statement(goto_stmt->token())
    , m_label_id(goto_stmt->label_id())
{
}

}
