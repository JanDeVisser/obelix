/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/Config.h>
#include <obelix/Processor.h>

namespace Obelix {

ProcessResult& materialize_arm64(ProcessResult&);
ProcessResult& output_arm64(ProcessResult&, Config const& config);

}
