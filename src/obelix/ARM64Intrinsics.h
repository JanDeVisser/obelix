/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <obelix/Intrinsics.h>
#include <obelix/ARM64Context.h>

namespace Obelix {

#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic) ErrorOr<void> arm64_##intrinsic(ARM64Context&);
INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE

}
