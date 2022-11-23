/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <rt/obelix.h>

$enum_value $get_enum_value($enum_value values[], int32_t value)
{
    for (int ix = 0; values[ix].text != NULL; ix++) {
        if (values[ix].value == value)
            return values[ix];
    }
    $enum_value ret = { 0, NULL };
    return ret;
}
