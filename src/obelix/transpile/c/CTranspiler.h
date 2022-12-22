/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/Config.h>
#include <obelix/Processor.h>

namespace Obelix {

ProcessResult& transpile_to_c(ProcessResult& result, Config const& config);

}
