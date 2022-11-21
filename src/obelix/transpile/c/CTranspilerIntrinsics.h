/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <obelix/Intrinsics.h>
#include <obelix/transpile/c/CTranspilerContext.h>

namespace Obelix {

using CTranspilerFunctionType = std::function<ErrorOr<void, SyntaxError>(CTranspilerContext&)>;

bool register_c_transpiler_intrinsic(IntrinsicType, CTranspilerFunctionType);
CTranspilerFunctionType const& get_c_transpiler_intrinsic(IntrinsicType);

}
