/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "../../../../../Qt/6.2.1/macos/lib/QtGui.framework/Headers/QPainter"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QLabel"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QLayout"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QStyleOption"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QWidget"

#include <jv80/gui/qled.h>

#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/memory.h>
#include <jv80/cpu/register.h>

#define LED_SIZE 16

namespace Obelix::JV80::GUI {

using namespace Obelix::JV80::CPU;

class StyledWidget : public QWidget {
    Q_OBJECT

public:
    explicit StyledWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setStyleSheet("StyledWidget { border: 1px solid grey; border-radius: 5px; }");
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QStyleOption o;
        o.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(
            QStyle::PE_Widget, &o, &p, this);
    }
};

class ImpactLabel : public QLabel {
public:
    explicit ImpactLabel(const QString& label)
        : QLabel(label)
    {
        setFont(QFont("Impact Label Reversed", 16));
        setStyleSheet("QLabel { color : white; }");
    }

    void setFontSize(int sz)
    {
        setFont(QFont("Impact Label Reversed", sz));
    }
};

class DSegLabel : public QLabel {
private:
    QLatin1Char m_fill;
    int m_width;

public:
    enum Style {
        SevenSegment,
        FourteenSegment,
        IBM3270,
        SkyFont,
    };

    explicit DSegLabel(const QString& label, int width = 4)
        : QLabel(label)
        , m_fill(' ')
        , m_width(width)
    {
        setDSegStyle(IBM3270);
    }

    void setDSegStyle(Style style)
    {
        switch (style) {
        case SevenSegment:
            setStyleSheet("color: green; font-family: \"DSEG7 Classic\"; font-size: 16pt;");
            m_fill = QLatin1Char('!');
            break;
        case FourteenSegment:
            setStyleSheet("color: green; font-family: \"DSEG14 Classic\"; font-size: 16pt;");
            m_fill = QLatin1Char('!');
            break;
        case IBM3270:
            setStyleSheet("font-family: \"IBM 3270\"; font-size: 20pt; color: green;");
            ;
            m_fill = QLatin1Char(' ');
            break;
        case SkyFont:
            setStyleSheet("font-family: SkyFont; font-size: 20pt; color: green;");
            ;
            m_fill = QLatin1Char(' ');
            break;
        }
    }

    void setValue(int value)
    {
        setText(QString("%1").arg(value, m_width, 16, QLatin1Char('0')));
    }

    void setValue(QString& value)
    {
        setText(QString("%1").arg(value, m_width, m_fill));
    }

    void setValue(QString&& value)
    {
        setText(QString("%1").arg(value, m_width, m_fill));
    }

    void setValue(std::string& value)
    {
        setText(QString("%1").arg(value.c_str(), m_width, m_fill));
    }

    void setValue(std::string&& value)
    {
        setText(QString("%1").arg(value.c_str(), m_width, m_fill));
    }

    void erase()
    {
        setText("");
    }
};

class RegisterNameLabel : public DSegLabel {
private:
    SystemBus& m_bus;

public:
    explicit RegisterNameLabel(SystemBus& bus)
        : DSegLabel("", 4)
        , m_bus(bus)
    {
        erase();
    }

    void setRegister(int id)
    {
        auto bp = m_bus.backplane();
        auto n = bp.name(id);
        while (n.size() < 4)
            n += " ";
        setText(n.c_str());
    }
};

class ByteWidget : public QWidget {
private:
    QLayout* m_layout;
    QLedArray* m_leds;
    DSegLabel* m_label;

public:
    explicit ByteWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_layout = new QHBoxLayout;
        setLayout(m_layout);
        m_leds = new QLedArray(8, 12);
        m_layout->addWidget(m_leds);
        m_label = new DSegLabel("", 2);
        m_label->setValue(0);
        m_layout->addWidget(m_label);
        setValue(0);
    }

    void setValue(byte value)
    {
        m_leds->setValue(value);
        m_label->setText(QString("%1").arg(value, 2, 16, QLatin1Char('0')));
    }
};

typedef std::function<void()> ValueUpdater;

class ComponentView : public StyledWidget
    , ComponentListener {
    Q_OBJECT

signals:
    void valueChanged();

public:
    void componentEvent(Component const* sender, int ev) override;

protected:
    ConnectedComponent* component;
    ImpactLabel* name;
    DSegLabel* value;
    QHBoxLayout* layout;
    ValueUpdater updater = nullptr;

    explicit ComponentView(ConnectedComponent*, int = 4, QWidget* = nullptr);

protected slots:
    void refresh();
};

class RegisterView : public ComponentView {
    Q_OBJECT

public:
    explicit RegisterView(Register* reg, QWidget* parent = nullptr)
        : ComponentView(reg, 2, parent)
    {
    }
};

class AddressRegisterView : public ComponentView {
    Q_OBJECT

public:
    explicit AddressRegisterView(AddressRegister* reg, QWidget* parent = nullptr)
        : ComponentView(reg, 4, parent)
    {
    }
};

class InstructionRegisterView : public ComponentView {
    Q_OBJECT

signals:
    void stepChanged();

public:
    explicit InstructionRegisterView(Controller* reg, QWidget* parent = nullptr);
    void componentEvent(Component const*, int) override;

private:
    DSegLabel* step;

protected:
    Controller* controller() const
    {
        return dynamic_cast<Controller*>(component);
    }
};

class MemoryView : public ComponentView {
    Q_OBJECT

signals:
    void configurationChanged();
    void contentsChanged();
    void imageLoaded();

public:
    explicit MemoryView(Memory* reg, QWidget* parent = nullptr);
    void componentEvent(Component const*, int) override;

protected:
    Memory* memory() const
    {
        return dynamic_cast<Memory*>(component);
    }

private:
    DSegLabel* contents;
};

}
