/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <sys/stat.h>

#include <core/Error.h>
#include <core/StringBuffer.h>

namespace Obelix {

class FileBuffer {
public:
    explicit FileBuffer(std::string file_name);
    [[nodiscard]] std::unique_ptr<StringBuffer>& buffer() { return m_buffer; }
    [[nodiscard]] std::string const& file_name() const { return m_file_name; }
    [[nodiscard]] bool file_is_read() const { return m_error.code() == ErrorCode::NoError; };
    [[nodiscard]] Error const& error() const { return m_error; }
    [[nodiscard]] size_t size() const { return m_size; }

private:
    std::string m_file_name;
    std::unique_ptr<StringBuffer> m_buffer;
    size_t m_size { 0 };
    Error m_error { ErrorCode::NoError };
};

}
