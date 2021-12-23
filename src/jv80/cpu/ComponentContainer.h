/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/systembus.h>
#include <jv80/cpu/ConnectedComponent.h>

namespace Obelix::JV80::CPU {

class ComponentContainer : public Component {
public:
    ~ComponentContainer() override = default;

    void insert(std::shared_ptr<ConnectedComponent> const&);

    template<class ComponentType>
    [[nodiscard]] std::shared_ptr<ComponentType> component(int address) const
    {
        for (auto& component : m_slots) {
            auto& c = *component;
            if ((typeid(c) == typeid(ComponentType&)) && ((component->address() == address) || (component->alias() == address)))
                return std::dynamic_pointer_cast<ComponentType>(component);
        }
        return nullptr;
    }

    template<>
    [[nodiscard]] std::shared_ptr<ConnectedComponent> component(int address) const
    {
        for (auto& component : m_slots) {
            if ((component->address() == address) || (component->alias() == address))
                return component;
        }
        return nullptr;
    }

    template<class ComponentType>
    [[nodiscard]] std::shared_ptr<ComponentType> component() const
    {
        for (auto& component : m_slots) {
            auto& c = *component;
            if (typeid(c) == typeid(ComponentType&))
                return std::dynamic_pointer_cast<ComponentType>(component);
        }
        return nullptr;
    }

    void insertIO(std::shared_ptr<ConnectedComponent> const& component);
    SystemBus& bus();
    [[nodiscard]] SystemBus const& bus() const;
    [[nodiscard]] std::string name(int) const;
    [[nodiscard]] SystemError forAllComponents(const ComponentHandler&) const;
    [[nodiscard]] SystemError forAllChannels(const ComponentHandler&);

protected:
    SystemBus m_bus;

    ComponentContainer();
    explicit ComponentContainer(std::shared_ptr<ConnectedComponent> const&);

private:
    std::vector<std::shared_ptr<ConnectedComponent>> m_slots;
    std::vector<int> m_aliases;
    std::vector<std::shared_ptr<ConnectedComponent>> m_io;
};

}
