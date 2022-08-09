/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <obelix/Intrinsics.h>
#include <obelix/interp/Context.h>

namespace Obelix {

bool register_interp_intrinsic(IntrinsicType, InterpImplementation);
InterpImplementation const& get_interp_intrinsic(IntrinsicType);

}
