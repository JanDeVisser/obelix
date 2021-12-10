/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyleOption>
#include <QTextEdit>
#include <QThread>

#include <cpu/backplane.h>

#include <gui/commands.h>
#include <gui/cputhread.h>
#include <gui/memdump.h>
#include <gui/terminal.h>

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
