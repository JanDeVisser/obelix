/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "component.h"

namespace Obelix::CPU {

ComponentListener* Component::setListener(ComponentListener* listener)
{
    auto old = m_listener;
    m_listener = listener;
    return old;
}

void Component::sendEvent(int ev) const
{
    if (m_listener) {
        m_listener->componentEvent(this, ev);
    }
}

SystemErrorCode Component::error(SystemErrorCode err)
{
    m_error = err;
    return m_error;
}

}
