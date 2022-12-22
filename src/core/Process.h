/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

#include <core/Error.h>

namespace Obelix {

ErrorOr<int,SystemError> execute(std::string const&, std::vector<std::string> const& = {});

template<typename... Args>
ErrorOr<int,SystemError> execute(std::string const& name, std::vector<std::string>& cmd_args, std::string const& arg, Args&&... args)
{
    cmd_args.push_back(arg);
    return execute(name, cmd_args, std::forward<Args>(args)...);
}

template<typename... Args>
ErrorOr<int,SystemError> execute(std::string const& name, std::string const& arg, Args&&... args)
{
    std::vector<std::string> cmd_args;
    cmd_args.push_back(arg);
    return execute(name, cmd_args, std::forward<Args>(args)...);
}

class Process {
public:
    explicit Process(std::string, std::vector<std::string>);

    template<typename... Args>
    explicit Process(std::string const& name, Args&&... args)
        : Process(name, std::vector<std::string> {})
    {
        add_arguments(std::forward<Args>(args)...);
    }

    [[nodiscard]] std::string const& standard_out() const
    {
        return m_stdout;
    }

    [[nodiscard]] std::string const& standard_error() const
    {
        return m_stderr;
    }

    ErrorOr<int,SystemError> execute();

private:
    template<typename... Args>
    void add_arguments(std::string const& argument, Args&&... args)
    {
        m_arguments.push_back(argument);
        add_arguments(std::forward<Args>(args)...);
    }

    void add_arguments()
    {
    }

    std::string m_command;
    std::vector<std::string> m_arguments = {};
    std::string m_stdout;
    std::string m_stderr;
};

}
