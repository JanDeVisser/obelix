/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
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

using InterpContext = Context<std::shared_ptr<SyntaxNode>>;
using InterpImplementation = std::function<ErrorOrNode(InterpContext&, BoundLiterals)>;

}
