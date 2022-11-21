/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/Type.h>

namespace Obelix {

struct TypeMnemonicMap {
    PrimitiveType type;
    bool is_signed;
    int size;
    char const* load_mnemonic;
    char const* store_mnemonic;
    char const* reg_width;
};

TypeMnemonicMap const* get_type_mnemonic_map(std::shared_ptr<ObjectType> const&);

}
