/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/cpu/component.h>

namespace Obelix::JV80::CPU {

ComponentListener* Component::setListener(ComponentListener* l)
{
    ComponentListener* old = listener;
    listener = l;
    return old;
}

void Component::sendEvent(int ev) const
{
    if (listener) {
        listener->componentEvent(this, ev);
    }
}

std::ostream& operator<<(std::ostream& os, Component& c)
{
    return c.status(os);
}

}
