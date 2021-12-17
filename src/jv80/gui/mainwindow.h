/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "../../../../../Qt/6.2.1/macos/lib/QtCore.framework/Headers/QThread"
#include "../../../../../Qt/6.2.1/macos/lib/QtGui.framework/Headers/QAction"
#include "../../../../../Qt/6.2.1/macos/lib/QtGui.framework/Headers/QPainter"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QLabel"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QLineEdit"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QMainWindow"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QMenuBar"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QProxyStyle"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QPushButton"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QStyleOption"
#include "../../../../../Qt/6.2.1/macos/lib/QtWidgets.framework/Headers/QTextEdit"

#include <jv80/cpu/backplane.h>

#include <jv80/gui/commands.h>
#include <jv80/gui/cputhread.h>
#include <jv80/gui/memdump.h>
#include <jv80/gui/terminal.h>

namespace Obelix::JV80::GUI {

class MainWindow : public QMainWindow {
    Q_OBJECT

private slots:
    void cpuStopped(const QString&);
    void openFile();

public:
    explicit MainWindow(QWidget* parent = nullptr);
    CPU* cpu() { return m_cpu; }
    QString query(const QString&, const QString&);
    void focusOnAddress(word addr) { m_memdump->focusOnAddress(addr); }
    MemoryBank& currentBank() { return m_memdump->currentBank(); }

private:
    CPU* m_cpu = nullptr;
    QAction* m_exit = nullptr;
    QAction* m_open = nullptr;
    QLabel* m_history;
    QLabel* m_result;
    CommandLineEdit* m_command;

    MemDump* m_memdump = nullptr;
    QTextEdit* m_status = nullptr;
    Terminal* m_terminal = nullptr;

    CommandLineEdit* makeCommandLine();

private slots:
    void commandResult(const QString&, bool, const QString&);

protected:
    void createMenu();
    void addHistory(const QString&);
    static QVector<QString> fileCompletions(const QStringList&);
    void keyPressEvent(QKeyEvent*);

signals:
    void keyPressed(QKeyEvent* ev);
};

struct BankCommand {
    Command& cmd;
    MainWindow* window;

    BankCommand(MainWindow*, Command&);
    void execute();
    void add();
    void del();

    word parseWord(int, QString&&);
};

}