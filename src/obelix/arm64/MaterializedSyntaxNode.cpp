/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/arm64/MaterializedSyntaxNode.h>
#include <obelix/arm64/VariableAddress.h>

namespace Obelix {

std::shared_ptr<VariableAddress> MaterializedVariableDecl::address() const
{
    return std::make_shared<StackVariableAddress>(offset());
}

std::shared_ptr<VariableAddress> MaterializedStaticVariableDecl::address() const
{
    return std::make_shared<StaticVariableAddress>(label());
}

}
