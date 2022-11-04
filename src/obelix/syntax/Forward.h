/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <vector>

#include <obelix/SyntaxNodeType.h>
#include <obelix/syntax/Syntax.h>

namespace Obelix {

#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type)          \
    class type;                            \
    using p##type = std::shared_ptr<type>; \
    using type##s = std::vector<p##type>;
ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE

}
