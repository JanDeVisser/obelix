/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/component.h>
#include <vector>

namespace Obelix::JV80::CPU {

using Reset = std::function<void(void)>;
using ClockEvent = std::function<SystemError()>;

class ComponentContainer;

class SystemBus : public Component {
public:
    enum class RunMode {
        Continuous = 0,
        BreakAtInstruction = 1,
        BreakAtClock = 2,
    };

    enum ProcessorFlags {
        Clear = 0x00,
        Z = 0x01,
        C = 0x02,
        V = 0x04,
    };

    enum OperatorFlags {
        None = 0x00,
        IOIn = 0x01,
        INC = 0x01,
        DEC = 0x02,
        Flags = 0x04,
        MSB = 0x08,
        Halt = 0x08,
        IOOut = 0x08,
        Mask = 0x0F,
        Done = 0x10,
    };

    explicit SystemBus(ComponentContainer*);
    ~SystemBus() override = default;
    [[nodiscard]] byte readDataBus() const;
    void putOnDataBus(byte);
    [[nodiscard]] byte readAddrBus() const;
    void putOnAddrBus(byte);
    [[nodiscard]] bool xdata() const { return _xdata; }
    [[nodiscard]] bool xaddr() const { return _xaddr; }
    [[nodiscard]] bool io() const { return _io; }
    [[nodiscard]] bool halt() const { return _halt; }
    [[nodiscard]] bool sus() const { return _sus; }
    void clearSus() { _sus = true; }
    [[nodiscard]] bool nmi() const { return _nmi; }
    void setNmi() { _nmi = false; }
    void clearNmi() { _nmi = true; }
    [[nodiscard]] byte putID() const { return put; }
    [[nodiscard]] byte getID() const { return get; }
    [[nodiscard]] byte opflags() const { return op; }

    void initialize(bool, bool, bool, byte, byte, byte, byte = 0x00, byte = 0x00);
    void xdata(int, int, int);
    void xaddr(int, int, int);
    void io(int, int, int);
    void stop();
    void suspend();
    SystemError reset() override;
    [[nodiscard]] std::string to_string() const override;

    void setFlag(ProcessorFlags, bool = true);
    void clearFlag(ProcessorFlags);
    void clearFlags();
    void setFlags(byte);
    [[nodiscard]] byte flags() const { return m_flags; }
    [[nodiscard]] bool isSet(ProcessorFlags) const;
    [[nodiscard]] std::string flagsString() const;

    [[nodiscard]] RunMode runMode() const { return m_runMode; }
    void setRunMode(RunMode runMode) { m_runMode = runMode; }

    ComponentContainer* backplane()
    {
        return m_backplane;
    }

private:
    ComponentContainer* m_backplane;
    byte data_bus = 0;
    byte addr_bus = 0;
    byte put = 0;
    byte get = 0;
    byte op = 0;
    bool _halt = true;
    bool _sus = true;
    bool _nmi = true;
    bool _xdata = true;
    bool _xaddr = true;
    bool rst = false;
    bool _io = true;

    byte m_flags = 0x0;

    void _reset();
    RunMode m_runMode = RunMode::Continuous;
};

class ConnectedComponent : public Component {
public:
    [[nodiscard]] virtual int id() const { return ident; }
    [[nodiscard]] virtual int alias() const { return id(); }
    [[nodiscard]] virtual std::string name() const { return componentName; }
    void bus(SystemBus* bus) { systemBus = bus; }
    [[nodiscard]] SystemBus* bus() const { return systemBus; }
    [[nodiscard]] virtual int getValue() const { return 0; }

protected:
    ConnectedComponent() = default;
    explicit ConnectedComponent(int id, std::string n)
        : ident(id)
    {
        componentName = std::move(n);
    }

private:
    SystemBus* systemBus = nullptr;
    int ident = -1;
    std::string componentName = "?";
};

class ComponentContainer : public Component {
public:
    ~ComponentContainer() override = default;

    void insert(std::shared_ptr<ConnectedComponent> const& component)
    {
        component->bus(&m_bus);
        m_components[component->id()] = component;
        m_aliases[component->id()] = component->id();
        if (component->id() != component->alias()) {
            m_aliases[component->alias()] = component->id();
        }
    }

    [[nodiscard]] std::shared_ptr<ConnectedComponent> const& component(int ix) const
    {
        return m_components[m_aliases[ix]];
    }

    void insertIO(std::shared_ptr<ConnectedComponent> const& component)
    {
        component->bus(&m_bus);
        m_io[component->id()] = component;
    }

    SystemBus& bus()
    {
        return m_bus;
    }

    [[nodiscard]] SystemBus const& bus() const
    {
        return m_bus;
    }

    [[nodiscard]] std::string name(int ix) const
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

    SystemError forAllComponents(const ComponentHandler& handler) const
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

    SystemError forAllChannels(const ComponentHandler& handler)
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

protected:
    SystemBus m_bus;

    explicit ComponentContainer()
        : m_bus(this)
        , m_components()
        , m_aliases()
    {
        m_components.resize(16);
        m_aliases.resize(16);
        m_io.resize(16);
    };

    explicit ComponentContainer(std::shared_ptr<ConnectedComponent> const& c)
        : ComponentContainer()
    {
        insert(c);
    };

private:
    std::vector<std::shared_ptr<ConnectedComponent>> m_components;
    std::vector<int> m_aliases;
    std::vector<std::shared_ptr<ConnectedComponent>> m_io;
};

}
