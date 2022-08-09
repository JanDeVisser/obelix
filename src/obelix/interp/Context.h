/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <config.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Processor.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(interp);

using InterpContext = Context<int>;
using InterpImplementation = std::function<ErrorOrNode(InterpContext&, std::vector<std::shared_ptr<BoundLiteral>>)>;

}
