/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QWidget"

#include <jv80/gui/qled.h>

#include <jv80/cpu/systembus.h>
#include <jv80/gui/componentview.h>

namespace Obelix::JV80::GUI {

class SystemBusView : public QWidget
    , public ComponentListener {
    Q_OBJECT

signals:
    void valueChanged();

public:
    explicit SystemBusView(SystemBus& bus, QWidget* parent = nullptr);
    void componentEvent(Component const* sender, int ev) override;

private slots:
    void refresh();

private:
    SystemBus& systemBus;
    QLayout* layout;
    ByteWidget* data;
    ByteWidget* address;
    RegisterNameLabel* put;
    RegisterNameLabel* get;
    QLed* xdata;
    QLed* xaddr;
    QLed* io;
    QLedArray* op;
    QLabel* z;
    QLabel* c;
    QLabel* v;
};

}
