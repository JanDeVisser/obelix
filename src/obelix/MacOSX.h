/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/Processor.h>

namespace Obelix {

ErrorOrNode output_macosx(std::shared_ptr<SyntaxNode> const&, std::string const& = "");

}
