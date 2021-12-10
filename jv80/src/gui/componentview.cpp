/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QPainter>
#include <QStyleOption>
#include <gui/componentview.h>

namespace Obelix::JV80::GUI {

ComponentView::ComponentView(ConnectedComponent* comp, int width, QWidget* owner)
    : StyledWidget(owner)
{
    component = comp;
    component->setListener(this);
    layout = new QHBoxLayout;
    setLayout(layout);
    name = new ImpactLabel(component->name().c_str());
    name->setFontSize(20);
    value = new DSegLabel(QString("%1").arg(component->getValue(), width, 16, QLatin1Char('0')), width);
    value->setDSegStyle(DSegLabel::IBM3270);

    layout->addWidget(name);
    layout->addWidget(value);

    connect(this, &ComponentView::valueChanged, this, &ComponentView::refresh);
}

void ComponentView::componentEvent(Component const* sender, int ev)
{
    if ((ev == Component::EV_VALUECHANGED) && (component->bus()->runMode() != SystemBus::Continuous)) {
        emit valueChanged();
    }
}

void ComponentView::refresh()
{
    if (updater) {
        updater();
    } else {
        value->setValue(component->getValue());
    }
}

// -----------------------------------------------------------------------

InstructionRegisterView::InstructionRegisterView(Controller* reg, QWidget* parent)
    : ComponentView(reg, 10, parent)
{
    step = new DSegLabel("0", 1);
    layout->addWidget(step, 0, Qt::AlignRight);
    value->erase();

    updater = [this]() {
        step->setValue(controller()->getStep());
        value->setValue(QString("%1").arg(controller()->instruction().c_str(), 10, QLatin1Char(' ')));
    };

    connect(this, &InstructionRegisterView::stepChanged, this, &InstructionRegisterView::refresh);
}

void InstructionRegisterView::componentEvent(Component const* sender, int ev)
{
    if ((ev == Controller::EV_STEPCHANGED) && (component->bus()->runMode() != SystemBus::Continuous)) {
        emit stepChanged();
    } else {
        ComponentView::componentEvent(sender, ev);
    }
}
// -----------------------------------------------------------------------

MemoryView::MemoryView(Memory* reg, QWidget* parent)
    : ComponentView(reg, 4, parent)
{
    contents = new DSegLabel("", 2);
    contents->setValue((*reg)[reg->getValue()]);
    layout->addWidget(contents, 0, Qt::AlignRight);

    updater = [this]() {
        value->setValue(component->getValue());
        contents->setValue((*memory())[component->getValue()]);
    };
    connect(this, &MemoryView::contentsChanged, this, &MemoryView::refresh);
}

void MemoryView::componentEvent(Component const* sender, int ev)
{
    switch (ev) {
    case Memory::EV_IMAGELOADED:
        emit imageLoaded();
        // Fall through
    case Memory::EV_CONTENTSCHANGED:
        if (component->bus()->runMode() != SystemBus::Continuous) {
            emit contentsChanged();
        }
        break;
    case Memory::EV_CONFIGCHANGED:
        emit configurationChanged();
        break;
    default:
        ComponentView::componentEvent(sender, ev);
        break;
    }
}

// -----------------------------------------------------------------------

}
