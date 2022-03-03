/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <string>

#include <core/Error.h>

namespace Obelix {
ErrorOr<int> execute(std::string const&, std::vector<std::string> const& = {});
}
