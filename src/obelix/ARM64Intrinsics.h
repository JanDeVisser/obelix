/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "obelix/Context.h"
#include <functional>
#include <string>
#include <unordered_map>

#include <obelix/Intrinsics.h>
#include <obelix/ARM64Context.h>

namespace Obelix {

using ARM64FunctionType = std::function<ErrorOr<void, SyntaxError>(ARM64Context&)>;

bool register_arm64_intrinsic(IntrinsicType, ARM64FunctionType);
ARM64FunctionType const& get_arm64_intrinsic(IntrinsicType);

}
