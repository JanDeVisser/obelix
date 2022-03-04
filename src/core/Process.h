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

template<typename... Args>
ErrorOr<int> execute(std::string const& name, std::vector<std::string>& cmd_args, std::string const& arg, Args&&... args)
{
    cmd_args.push_back(arg);
    return execute(name, cmd_args, std::forward<Args>(args)...);
}

template<typename... Args>
ErrorOr<int> execute(std::string const& name, std::string const& arg, Args&&... args)
{
    std::vector<std::string> cmd_args;
    cmd_args.push_back(arg);
    return execute(name, cmd_args, std::forward<Args>(args)...);
}
}
