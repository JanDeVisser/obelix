/*
 * FileBuffer.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
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
