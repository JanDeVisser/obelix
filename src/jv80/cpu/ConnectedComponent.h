/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/systembus.h>

namespace Obelix::JV80::CPU {

class ConnectedComponent : public Component {
public:
    [[nodiscard]] virtual int address() const { return m_address; }
    [[nodiscard]] virtual std::string name() const { return m_componentName; }
    void bus(SystemBus* bus) { m_systemBus = bus; }
    [[nodiscard]] SystemBus* bus() const { return m_systemBus; }
    [[nodiscard]] virtual int getValue() const { return 0; }

protected:
    ConnectedComponent() = default;
    explicit ConnectedComponent(int, std::string);

private:
    SystemBus* m_systemBus = nullptr;
    int m_address = -1;
    std::string m_componentName = "?";
};

}
