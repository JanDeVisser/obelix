/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_map>

#include <obelix/Syntax.h>

namespace Obelix {

bool is_intrinsic(std::string);
std::shared_ptr<FunctionDecl> get_intrinsic(std::string);

}
