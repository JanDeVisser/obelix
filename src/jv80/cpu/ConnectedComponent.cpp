/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/cpu/ConnectedComponent.h>

namespace Obelix::JV80::CPU {

ConnectedComponent::ConnectedComponent(int address, std::string n)
    : m_address(address)
    , m_componentName(std::move(n))
{
}

}
