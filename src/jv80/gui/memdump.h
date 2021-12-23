/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QWindow>
#include <QComboBox>
#include <QListView>

#include <jv80/cpu/backplane.h>
#include <jv80/cpu/memory.h>

namespace Obelix::JV80::GUI {

using namespace Obelix::JV80::CPU;

class MemModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit MemModel(MemoryBank const&, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QModelIndex indexOf(word addr = 0xFFFF) const;
    [[nodiscard]] word currentAddress() const { return m_address; }
    MemoryBank const& getBank() { return m_bank; }

signals:
    void numberPopulated(int number);

public slots:

private:
    MemoryBank const& m_bank;
    word m_address;

    [[nodiscard]] QVariant getRow(int) const;
};

class MemDump : public QWidget {
    Q_OBJECT

public:
    explicit MemDump(BackPlane*, QWidget* = nullptr);
    ~MemDump() override = default;
    [[nodiscard]] BackPlane* getSystem() const { return m_system; }
    void reload();
    void focusOnAddress(word addr = 0xFFFF);
    MemoryBank const& currentBank() { return m_model->getBank(); }

public slots:
    void focus();
    void selectBank();

signals:

protected:
private:
    BackPlane* m_system;
    std::shared_ptr<Memory> m_memory;
    MemModel* m_model = nullptr;
    QListView* m_view;
    QComboBox* m_banks;

    void createModel(word);
};

}
