/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/arm64/Mnemonic.h>

namespace Obelix {

static TypeMnemonicMap mnemonic_map[] = {
    { PrimitiveType::SignedIntegerNumber, true, 8, "ldr", "str", "x" },
    { PrimitiveType::IntegerNumber, false, 8, "ldr", "str", "x" },
    { PrimitiveType::Enum, false, 8, "ldr", "str", "x" },
    { PrimitiveType::Pointer, false, 8, "ldr", "str", "x" },
    { PrimitiveType::SignedIntegerNumber, true, 4, "ldr", "str", "w" },
    { PrimitiveType::IntegerNumber, false, 4, "ldr", "str", "w" },
    { PrimitiveType::SignedIntegerNumber, true, 1, "ldrsb", "strsb", "w" },
    { PrimitiveType::IntegerNumber, false, 1, "ldrb", "strb", "w" },
};

TypeMnemonicMap const* get_type_mnemonic_map(std::shared_ptr<ObjectType> const& type)
{
    bool is_signed = (type->has_template_argument("signed")) && type->template_argument<bool>("signed");
    auto size = (type->has_template_argument("size")) ? type->template_argument<long>("size") : type->size();
    for (auto const& mm : mnemonic_map) {
        if (mm.type != type->type())
            continue;
        if (mm.is_signed == is_signed && mm.size == size) {
            return &mm;
        }
    }
    return nullptr;
}

}
