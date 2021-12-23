/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/cpu/ComponentContainer.h>

namespace Obelix::JV80::CPU {

ComponentContainer::ComponentContainer()
    : m_bus(this)
    , m_components()
    , m_aliases()
{
    m_components.resize(16);
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
    m_components[component->id()] = component;
    m_aliases[component->id()] = component->id();
    if (component->id() != component->alias()) {
        m_aliases[component->alias()] = component->id();
    }
}

[[nodiscard]] std::shared_ptr<ConnectedComponent> const& ComponentContainer::component(int ix) const
{
    return m_components[m_aliases[ix]];
}

void ComponentContainer::insertIO(std::shared_ptr<ConnectedComponent> const& component)
{
    component->bus(&m_bus);
    m_io[component->id()] = component;
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
        auto& c = component(ix);
        return (c) ? c->name() : "";
    }
    }
}

[[nodiscard]] SystemError ComponentContainer::forAllComponents(const ComponentHandler& handler) const
{
    for (auto& component : m_components) {
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
