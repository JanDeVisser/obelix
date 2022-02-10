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

#undef INTRINSIC_SIGNATURE
#define INTRINSIC_SIGNATURE(sig) ErrorOr<void> arm64_##sig(ARM64Context&);
INTRINSIC_SIGNATURE_ENUM(INTRINSIC_SIGNATURE)
#undef INTRINSIC_SIGNATURE

}
