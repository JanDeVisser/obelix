/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <sys/stat.h>

#include <core/FileBuffer.h>
#include <core/StringBuffer.h>

namespace Obelix {

class OblBuffer {
public:
    explicit OblBuffer(std::string file_name);
    StringBuffer& buffer();
    [[nodiscard]] bool file_is_read() const { return m_error.code() == ErrorCode::NoError; }
    [[nodiscard]] Error const& error() const { return m_error; }
    [[nodiscard]] std::string const& file_name() const;
    [[nodiscard]] std::string const& dir_name() const;
    [[nodiscard]] std::string const& effective_file_name() const;

private:
    ErrorOr<void> try_open(std::string const&, std::optional<std::string> const& = {});

    std::string m_file_name;
    std::string m_dir_name;
    std::string m_effective_file_name;
    std::unique_ptr<StringBuffer> m_buffer { nullptr };
    Error m_error { ErrorCode::NoError };
};

}
