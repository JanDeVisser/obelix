/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/interp/Context.h>

namespace Obelix {

ProcessResult& interpret(ProcessResult&, InterpContext);
ProcessResult& interpret(ProcessResult&);

}
