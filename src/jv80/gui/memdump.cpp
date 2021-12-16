/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QComboBox>
#include <QListView>
#include <QVBoxLayout>

#include <jv80/gui/memdump.h>

namespace Obelix::JV80::GUI {

MemModel::MemModel(const Memory* memory, word bank, QObject* parent)
    : QAbstractListModel(parent)
{
    m_bank = memory->bank(bank);
    if (!m_bank.valid()) {
        auto banks = memory->banks();
        if (!banks.empty()) {
            m_bank = *(banks.begin());
        } else {
            m_bank = MemoryBank();
        }
    }
    m_address = m_bank.start();
}

int MemModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_bank.size() / 8;
}

QVariant MemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    if (index.row() >= (m_bank.size() / 8) || index.row() < 0) {
        return {};
    }
    if (role != Qt::DisplayRole) {
        return {};
    }
    return getRow(index.row());
}

QVariant MemModel::getRow(int row) const
{
    word start = m_bank.start() + (row * 8);
    word end = (word)qMin(start + 8, (int)m_bank.end());

    auto s = QString("%1   ").arg(start, 4, 16, QLatin1Char('0'));
    for (word ix = start; ix < end; ix++) {
        s += QString(" %1").arg(m_bank[ix], 2, 16, QLatin1Char('0'));
    }
    return QVariant(s);
}

QModelIndex MemModel::indexOf(word addr) const
{
    auto row = (addr - m_bank.start()) / 8;
    auto col = 0;
    return createIndex(row, col);
}

/* ----------------------------------------------------------------------- */

MemDump::MemDump(BackPlane* system, QWidget* parent)
    : QWidget(parent)
    , m_system(system)
    , m_memory(system->memory())
{
    m_view = new QListView;
    createModel(0x0000);
    QFont font("IBM 3270", 12);
    QFontMetrics metrics(font);
    auto width = metrics.horizontalAdvance("0000    00 00 00 00 00 00 00 00");
    m_view->setMinimumWidth(width + 40);
    m_view->setFont(font);
    m_view->setFocusPolicy(Qt::ClickFocus);

    m_view->setStyleSheet("QListView::item { background: black; color : green; } "
                          "QListView::item:selected { background: green; color: black; }");

    m_banks = new QComboBox;
    for (auto& bank : system->memory()->banks()) {
        m_banks->addItem(QString(bank.name().c_str()), bank.start());
    }
    connect(m_banks, SIGNAL(currentIndexChanged(int)), this, SLOT(selectBank(int)));

    auto* layout = new QVBoxLayout;
    layout->addWidget(m_view);
    layout->addWidget(m_banks);
    setLayout(layout);
    setWindowTitle(tr("Memory"));
}

void MemDump::createModel(word addr)
{
    auto old_model = m_model;
    m_model = new MemModel(m_memory, addr);
    m_view->setModel(m_model);
    delete old_model;
    m_view->reset();
}

void MemDump::reload()
{
    auto curix = m_banks->currentIndex();
    word addr = m_banks->currentData().toInt();
    m_banks->clear();
    auto ix = 0;
    for (auto& bank : getSystem()->memory()->banks()) {
        m_banks->addItem(QString(bank.name().c_str()), bank.start());
        if ((ix == curix) && (addr != bank.start())) {
            curix = 0;
            addr = m_banks->itemData(0).toInt();
        }
        ix++;
    }
    m_banks->setCurrentIndex(curix);
    createModel(addr);
}

void MemDump::focusOnAddress(word addr)
{
    auto bank = m_memory->bank(addr);
    if (bank != m_model->getBank()) {
        createModel(addr);
        for (int ix = 0; ix < m_banks->count(); ix++) {
            if (m_banks->itemData(ix) == bank.start()) {
                m_banks->setCurrentIndex(ix);
            }
        }
    }
    m_view->setCurrentIndex(m_model->indexOf(addr));
}

void MemDump::focus()
{
    focusOnAddress(m_model->currentAddress());
}

void MemDump::selectBank(int ix)
{
    createModel(m_banks->currentData().toInt());
}

}
