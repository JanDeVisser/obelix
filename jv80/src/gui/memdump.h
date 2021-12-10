/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QComboBox>
#include <QListView>
#include <QWindow>

#include <cpu/backplane.h>
#include <cpu/memory.h>

namespace Obelix::JV80::GUI {

using namespace Obelix::JV80::CPU;

class MemModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit MemModel(const Memory*, word, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QModelIndex indexOf(word addr = 0xFFFF) const;
    word currentAddress() const { return m_address; }
    MemoryBank& getBank() { return m_bank; }

signals:
    void numberPopulated(int number);

public slots:

private:
    word m_address;
    MemoryBank m_bank;

    QVariant getRow(int) const;
};

class MemDump : public QWidget {
    Q_OBJECT

public:
    explicit MemDump(BackPlane*, QWidget* = nullptr);
    ~MemDump() override = default;
    BackPlane* getSystem() const { return m_system; }
    void reload();
    void focusOnAddress(word addr = 0xFFFF);
    MemoryBank& currentBank() { return m_model->getBank(); }

public slots:
    void focus();
    void selectBank(int ix);

signals:

protected:
private:
    BackPlane* m_system;
    Memory* m_memory;
    MemModel* m_model = nullptr;
    QListView* m_view;
    QComboBox* m_banks;

    void createModel(word);
};

}
