/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/Error.h>
#include <core/Logging.h>

namespace Obelix {

std::string ErrorCode_name(ErrorCode code)
{
    switch (code) {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) \
    case ErrorCode::code:                 \
        return #code;
        ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

std::string ErrorCode_message(ErrorCode code)
{
    switch (code) {
#undef __ENUMERATE_ERROR_CODE
#define __ENUMERATE_ERROR_CODE(code, msg) \
    case ErrorCode::code:                 \
        return msg;
        ENUMERATE_ERROR_CODES(__ENUMERATE_ERROR_CODE)
#undef __ENUMERATE_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

}
