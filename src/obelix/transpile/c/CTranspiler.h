/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/Config.h>
#include <obelix/Processor.h>

namespace Obelix {

ProcessResult transpile_to_c(std::shared_ptr<SyntaxNode> const&, Config const& config);

}
