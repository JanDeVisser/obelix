/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Logging.h>
#include <obelix/Architecture.h>

namespace Obelix {

extern_logging_category(type);

std::optional<Architecture> Architecture_by_name(std::string const& a)
{
    auto arch_upper = to_upper(a);
#undef __ENUM_ARCHITECTURE
#define __ENUM_ARCHITECTURE(arch) \
    if (arch_upper == #arch)      \
        return Architecture::arch;
    ENUMERATE_ARCHITECTURES(__ENUM_ARCHITECTURE)
#undef __ENUM_ARCHITECTURE
    return {};
}


}
