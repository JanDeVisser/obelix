/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/interp/Context.h>

namespace Obelix {

ErrorOrNode interpret(std::shared_ptr<SyntaxNode>, InterpContext);
ErrorOrNode interpret(std::shared_ptr<SyntaxNode>);

}
