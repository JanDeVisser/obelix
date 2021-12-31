/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ConnectedComponent.h"

namespace Obelix::CPU {

ConnectedComponent::ConnectedComponent(int address, std::string n)
    : m_address(address)
    , m_componentName(std::move(n))
{
}

}
