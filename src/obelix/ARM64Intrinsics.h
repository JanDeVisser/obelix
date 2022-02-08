/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <obelix/ARM64Context.h>

namespace Obelix {

using Intrinsic = std::function<ErrorOr<void>(ARM64Context&)>;

auto register_instrinsic_impl(std::string const& name, Intrinsic function);
Intrinsic get_intrinsic_impl(std::string const& name);

}
