/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/cpu/ComponentContainer.h>

namespace Obelix::JV80::CPU {

ComponentContainer::ComponentContainer()
    : m_bus(this)
    , m_slots()
    , m_aliases()
{
    m_aliases.resize(16);
    m_io.resize(16);
};

ComponentContainer::ComponentContainer(std::shared_ptr<ConnectedComponent> const& c)
    : ComponentContainer()
{
    insert(c);
};

void ComponentContainer::insert(std::shared_ptr<ConnectedComponent> const& component)
{
    component->bus(&m_bus);
    m_slots.push_back(component);
    m_aliases[component->address()] = component->address();
    if (component->address() != component->alias()) {
        m_aliases[component->alias()] = component->address();
    }
}

void ComponentContainer::insertIO(std::shared_ptr<ConnectedComponent> const& component)
{
    component->bus(&m_bus);
    m_io[component->address()] = component;
}

SystemBus& ComponentContainer::bus()
{
    return m_bus;
}

[[nodiscard]] SystemBus const& ComponentContainer::bus() const
{
    return m_bus;
}

[[nodiscard]] std::string ComponentContainer::name(int ix) const
{
    switch (ix) {
    case 0x7:
        return "MEM";
    case 0xF:
        return "ADDR";
    default: {
        auto c = component<ConnectedComponent>(ix);
        return (c) ? c->name() : "";
    }
    }
}

[[nodiscard]] SystemError ComponentContainer::forAllComponents(const ComponentHandler& handler) const
{
    for (auto& component : m_slots) {
        if (!component)
            continue;
        if (auto err = handler(*component); err.is_error()) {
            return err;
        }
    }
    return {};
}

[[nodiscard]] SystemError ComponentContainer::forAllChannels(const ComponentHandler& handler)
{
    for (auto& channel : m_io) {
        if (!channel)
            continue;
        if (auto err = handler(*channel); err.is_error()) {
            return err;
        }
    }
    return {};
}

}
