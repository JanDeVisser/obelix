/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/Processor.h>

namespace Obelix {

ErrorOrNode output_jv80(std::shared_ptr<SyntaxNode> const&, std::string const& = "");

}
