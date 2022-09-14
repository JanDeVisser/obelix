/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/BoundSyntaxNode.h>

namespace Obelix {

ErrorOr<std::shared_ptr<BoundIntLiteral>, SyntaxError> BoundIntLiteral::cast(std::shared_ptr<BoundIntLiteral> const& from, std::shared_ptr<ObjectType> const& type)
{
    switch (type->size()) {
    case 1: {
        auto char_maybe = token_value<char>(from->token());
        if (char_maybe.has_value())
            return std::make_shared<BoundIntLiteral>(from->token(), char_maybe.value());
        return char_maybe.error();
    }
    case 2: {
        auto short_maybe = token_value<short>(from->token());
        if (short_maybe.has_value())
            return std::make_shared<BoundIntLiteral>(from->token(), short_maybe.value());
        return short_maybe.error();
    }
    case 4: {
        auto int_maybe = token_value<int>(from->token());
        if (int_maybe.has_value())
            return std::make_shared<BoundIntLiteral>(from->token(), int_maybe.value());
        return int_maybe.error();
    }
    case 8: {
        auto long_probably = token_value<long>(from->token());
        assert(long_probably.has_value());
        return std::make_shared<BoundIntLiteral>(from->token(), long_probably.value());
    }
    default:
        fatal("Unexpected int size {}", type->size());
    }
    return nullptr;
}

}
