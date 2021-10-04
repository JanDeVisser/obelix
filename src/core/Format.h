//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <string>
#include <vector>
#include <core/Object.h>

namespace Obelix {

std::string fformat(std::string, ...);
std::string vformat(std::string const&, va_list);

}
