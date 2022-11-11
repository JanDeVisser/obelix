/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <optional>

#include <core/Format.h>

namespace Obelix {

#define ENUMERATE_ARCHITECTURES(S)    \
    S(MACOS_ARM64, "macos_aarch64")   \
    S(RASPI_ARM64, "raspi_aarch64")   \
    S(MACOS_X86_64, "macos_x68_64")   \
    S(LINUX_X86_64, "linux")          \
    S(WINDOWS_X86_64, "windows")      \
    S(C_TRANSPILER, "c_transpiler")   \
    S(JS_TRANSPILER, "js_transpiler") \
    S(JV_80, "JV-80 8-bit CPU")       \
    S(INTERPRETER, "interp")

enum class Architecture {
#undef __ENUM_ARCHITECTURE
#define __ENUM_ARCHITECTURE(arch, text) arch,
    ENUMERATE_ARCHITECTURES(__ENUM_ARCHITECTURE)
#undef __ENUM_ARCHITECTURE
};

constexpr const char* Architecture_name(Architecture a)
{
    switch (a) {
#undef __ENUM_ARCHITECTURE
#define __ENUM_ARCHITECTURE(arch, text) \
    case Architecture::arch:            \
        return text;
        ENUMERATE_ARCHITECTURES(__ENUM_ARCHITECTURE)
#undef __ENUM_ARCHITECTURE
    }
}

std::optional<Architecture> Architecture_by_name(std::string const&);

template<>
struct Converter<Architecture> {
    static std::string to_string(Architecture val)
    {
        return Architecture_name(val);
    }

    static double to_double(Architecture val)
    {
        return static_cast<double>(val);
    }

    static long to_long(Architecture val)
    {
        return static_cast<long>(val);
    }
};

}
