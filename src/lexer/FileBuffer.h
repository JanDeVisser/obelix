/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/StringBuffer.h>
#include <memory>
#include <sys/stat.h>

namespace Obelix {

class FileBuffer {
public:
    explicit FileBuffer(std::string file_name);
    StringBuffer& buffer();

private:
    std::pair<int, struct stat> try_open(std::string const&);

    std::string m_file_name;
    std::unique_ptr<StringBuffer> m_buffer;
};

}
