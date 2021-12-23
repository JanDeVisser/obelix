/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/systembus.h>

namespace Obelix::JV80::CPU {

class ComponentContainer : public Component {
public:
    ~ComponentContainer() override = default;

    void insert(std::shared_ptr<ConnectedComponent> const&);

    [[nodiscard]] std::shared_ptr<ConnectedComponent> const& component(int ix) const;
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
    std::vector<std::shared_ptr<ConnectedComponent>> m_components;
    std::vector<int> m_aliases;
    std::vector<std::shared_ptr<ConnectedComponent>> m_io;
};

}
