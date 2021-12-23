
/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QDialog>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QObject>
#include <QPainter>
#include <QTabWidget>

#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/memory.h>
#include <jv80/cpu/register.h>
#include <jv80/cpu/registers.h>
#include <jv80/cpu/systembus.h>

#include <jv80/gui/componentview.h>
#include <jv80/gui/mainwindow.h>
#include <jv80/gui/systembusview.h>
#include <jv80/gui/terminal.h>

namespace Obelix::JV80::GUI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    createMenu();
    m_cpu = new CPU(this);
    connect(m_cpu, &CPU::executionEnded, this, &MainWindow::cpuStopped);
    connect(m_cpu, &CPU::executionInterrupted, this, &MainWindow::cpuStopped);
    setStyleSheet("MainWindow { background-color: black; }");

    auto widget = new QWidget;
    auto windowLayout = new QVBoxLayout;
    auto consoleLayout = new QHBoxLayout;
    auto layout = new QGridLayout;
    consoleLayout->addLayout(layout);
    windowLayout->addLayout(consoleLayout);
    widget->setLayout(windowLayout);

    auto system = m_cpu->getSystem();

    auto tabs = new QTabWidget();
    m_memdump = new MemDump(system);
    m_memdump->setFocusPolicy(Qt::NoFocus);
    tabs->addTab(m_memdump, "Memory");
    m_status = new QTextEdit();
    m_status->setFont(QFont("IBM 3270", 10));
    m_status->setStyleSheet("QTextEdit { color: green; background-color: black; }");
    tabs->addTab(m_status, "Log");
    consoleLayout->addWidget(tabs);

    m_terminal = new Terminal();
    connect(m_cpu, &CPU::terminalWrite, m_terminal, &Terminal::writeCharacter);
    connect(this, &MainWindow::keyPressed, m_cpu, &CPU::keyPressed);
    windowLayout->addWidget(m_terminal);

    auto busView = new SystemBusView(system->bus());
    layout->addWidget(busView, 0, 0, 1, 2);
    for (int r = 0; r < 4; r++) {
        auto reg = system->component(r);
        auto regView = new RegisterView(std::dynamic_pointer_cast<Register>(reg), widget);
        layout->addWidget(regView, r / 2 + 1, r % 2);
    }

    auto reg = system->component(IR);
    auto regView = new InstructionRegisterView(dynamic_pointer_cast<Controller>(reg), widget);
    layout->addWidget(regView, 3, 0);

    for (int r = 0; r < 4; r++) {
        auto addr_reg = system->component(8 + r);
        auto addr_regView = new AddressRegisterView(dynamic_pointer_cast<AddressRegister>(addr_reg), widget);
        layout->addWidget(addr_regView, r / 2 + 4, r % 2);
    }

    int row = 6;
    auto mem = system->component(MEMADDR);
    auto memView = new MemoryView(dynamic_pointer_cast<Memory>(mem), widget);
    layout->addWidget(memView, row++, 0);
    connect(memView, &ComponentView::valueChanged, m_memdump, &MemDump::focus);
    connect(memView, &MemoryView::imageLoaded, m_memdump, &MemDump::reload);
    connect(memView, &MemoryView::contentsChanged, m_memdump, &MemDump::reload);
    connect(memView, &MemoryView::configurationChanged, m_memdump, &MemDump::reload);

    m_history = new QLabel;
    m_history->setFont(QFont("IBM 3270", 15));
    m_history->setStyleSheet("QLabel { background-color: black; color: green; border: none; }");
    layout->addWidget(m_history, row++, 0, 1, 2);

    m_result = new QLabel;
    m_result->setFont(QFont("IBM 3270", 15));
    m_result->setStyleSheet("QLabel { background-color: black; color: green; border: none; }");
    layout->addWidget(m_result, row++, 0, 1, 2);

    auto w = new QWidget;
    auto hbox = new QHBoxLayout;
    w->setLayout(hbox);
    auto prompt = new QLabel(">");
    prompt->setFont(QFont("IBM 3270", 15));
    prompt->setStyleSheet("QLabel { background-color: black; color: green; }");
    hbox->addWidget(prompt);

    m_command = makeCommandLine();
    hbox->addWidget(m_command);
    layout->addWidget(w, row++, 0, 1, 2);
    m_command->setFocus();

    layout->setMenuBar(menuBar());
    setCentralWidget(widget);
    setWindowTitle(tr("Emu"));
}

void MainWindow::createMenu()
{
    auto fileMenu = new QMenu(tr("&File"), this);

    m_open = new QAction(tr("&Open"), this);
    m_open->setShortcuts(QKeySequence::Open);
    m_open->setStatusTip(tr("Open a memory image file"));
    connect(m_open, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu->addAction(m_open);

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    m_exit = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    m_exit->setShortcuts(QKeySequence::Quit);
    m_exit->setStatusTip(tr("Exit the application"));
    menuBar()->addMenu(fileMenu);

    //  connect(exitAction, &QAction::triggered, this, &QDialog::accept);
}

void MainWindow::openFile()
{
    QString selectedFilter;
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("QFileDialog::getOpenFileName()"),
        ".",
        tr("Binary Files (*.bin);;All Files (*)"),
        &selectedFilter);
    if (!fileName.isEmpty()) {
        m_cpu->openImage(fileName);
    }
}

void MainWindow::cpuStopped(const QString& status)
{
    auto t = m_status->toPlainText();
    if (!t.isEmpty()) {
        t += "\n";
    }
    t += status;
    m_status->setPlainText(t);
}

CommandLineEdit* MainWindow::makeCommandLine()
{
    auto ret = new CommandLineEdit();
    ret->setMaxLength(80);
    ret->setFont(QFont("IBM 3270", 15));
    ret->setStyleSheet("QLineEdit { background-color: black; font: \"IBM 3270\"; font-size: 15; color: green; border: none; }");
    connect(ret, SIGNAL(result(const QString&, bool, const QString&)), this, SLOT(commandResult(const QString&, bool, const QString&)));

    ret->addCommandDefinition(ret, "quit", 0, 0,
        [this, ret](Command& cmd) {
            if (ret->query("Are you sure you want to exit? (Y/N)", "yn") == "y") {
                close();
            }
        });

    ret->addCommandDefinition(ret, "run", 0, 1,
        [this, ret](Command& cmd) {
            if (!m_cpu->isRunning()) {
                word addr = 0xFFFF;
                if (cmd.numArgs() == 1) {
                    bool ok;
                    addr = cmd.arg(0).toUShort(&ok, 0);
                    if (!ok) {
                        cmd.setError(QString("Syntax error: unparsable address '%1").arg(cmd.arg(0)));
                        return;
                    }
                }
                ret->clearFocus();
                m_cpu->run(addr);
                cmd.setSuccess();
            } else {
                cmd.setError("CPU running");
            }
        });

    ret->addCommandDefinition(ret, "reset", 0, 0,
        [this](Command& cmd) {
            if (!m_cpu->isRunning()) {
                m_cpu->reset();
            } else {
                cmd.setError("CPU running");
            }
        });

    ret->addCommandDefinition(ret, "step", 0, 1,
        [this](Command& cmd) {
            if (m_cpu->isRunning()) {
                cmd.setError("CPU running");
            } else if (m_cpu->isHalted()) {
                cmd.setError("CPU halted");
            } else {
                word addr = 0xFFFF;
                if (cmd.numArgs() == 1) {
                    bool ok;
                    addr = cmd.arg(0).toUShort(&ok, 0);
                    if (!ok) {
                        cmd.setError(QString("Syntax error: unparsable address '%1").arg(cmd.arg(0)));
                        return;
                    }
                }
                m_cpu->step(addr);
            }
        });

    ret->addCommandDefinition(ret, "tick", 0, 1,
        [this](Command& cmd) {
            if (m_cpu->isRunning()) {
                cmd.setError("CPU running");
            } else if (m_cpu->isHalted()) {
                cmd.setError("CPU halted");
            } else {
                word addr = 0xFFFF;
                if (cmd.numArgs() == 1) {
                    bool ok;
                    addr = cmd.arg(0).toUShort(&ok, 0);
                    if (!ok) {
                        cmd.setError(QString("Syntax error: unparsable address '%1").arg(cmd.arg(0)));
                        return;
                    }
                }
                m_cpu->tick(addr);
            }
        });

    ret->addCommandDefinition(ret, "pause", 0, 0,
        [this](Command& cmd) {
            if (!m_cpu->isRunning()) {
                cmd.setError("CPU not running");
            } else if (m_cpu->isHalted()) {
                cmd.setError("CPU halted");
            } else {
                m_cpu->interrupt();
            }
        });

    ret->addCommandDefinition(ret, "continue", 0, 0,
        [this](Command& cmd) {
            if (m_cpu->isRunning()) {
                cmd.setError("CPU running");
            } else if (m_cpu->isHalted()) {
                cmd.setError("CPU halted");
            } else {
                m_cpu->continueExecution();
            }
        });

    ret->addCommandDefinition(ret, "clock", 0, 1,
        [this](Command& cmd) {
            bool ok;
            double speed;
            switch (cmd.numArgs()) {
            case 0:
                cmd.setResult(QString("%1 kHz").arg(m_cpu->getSystem()->clockSpeed()));
                break;
            case 1:
                speed = cmd.arg(0).toDouble(&ok);
                if (ok) {
                    if (!m_cpu->getSystem()->setClockSpeed(speed)) {
                        cmd.setError(QString("Frequency %1 kHz out of bounds").arg(speed));
                    }
                } else {
                    cmd.setError(QString("Requested frequency '%1' is not a number").arg(cmd.arg(0)));
                }
                break;
            }
        });

    ret->addCommandDefinition(
        ret, "load", 1, 3,
        [this](Command& cmd) {
            if (!QFile::exists(cmd.arg(0))) {
                cmd.setError(QString("File %1/%2 does not exist").arg(QDir::currentPath()).arg(cmd.arg(0)));
            } else if (!(QFile::permissions(cmd.arg(0)) | QFileDevice::ReadUser)) {
                cmd.setError(QString("File %1/%2 is not readable").arg(QDir::currentPath()).arg(cmd.arg(0)));
            } else {
                word addr = 0x0000;
                bool writable = true;
                bool ok;
                if (cmd.numArgs() == 3) {
                    auto lwr = cmd.arg(2).toLower();
                    if ((lwr == "rom") || (lwr == "false")) {
                        writable = false;
                    } else if ((lwr != "ram") && (lwr != "true")) {
                        cmd.setError(QString("Syntax error: unparsable writable flag '%1").arg(cmd.arg(2)));
                        return;
                    }
                }
                if (cmd.numArgs() >= 2) {
                    addr = (word)cmd.arg(1).toUShort(&ok, 0);
                    if (!ok) {
                        cmd.setError(QString("Syntax error: unparsable address '%1").arg(cmd.arg(1)));
                        return;
                    }
                }
                m_cpu->openImage(cmd.arg(0), addr, writable);
            }
        },
        [](const QStringList& args) {
            return fileCompletions(args);
        });

    ret->addCommandDefinition(ret, "mem", 1, 2,
        [this](Command& cmd) {
            byte value;
            word addr;
            bool ok;
            bool poke = false;
            byte v;
            int opcode;

            switch (cmd.numArgs()) {
            case 2:
                value = (byte)cmd.arg(1).toInt(&ok, 0);
                if (!ok) {
                    opcode = m_cpu->getSystem()->controller()->opcodeForInstruction(
                        cmd.arg(1).replace(QChar('_'), QChar(' ')).toStdString());
                    if (opcode == -1) {
                        cmd.setError(QString("Syntax error: unparsable value '%1").arg(cmd.arg(1)));
                        return;
                    }
                    value = (byte)opcode;
                }
                poke = true;
                // Fall through
            case 1:
                addr = (word)cmd.arg(0).toUShort(&ok, 0);
                if (!ok) {
                    cmd.setError(QString("Syntax error: unparsable address '%1").arg(cmd.arg(0)));
                    return;
                }
                if (poke) {
                    if (auto err = m_cpu->getSystem()->memory()->poke(addr, value); err.is_error())
                        fprintf(stderr, "Poke(%04x, %02x): %s\n", addr, value, SystemErrorCode_name(err.error()));
                }
                m_memdump->focusOnAddress(addr);
                v = m_cpu->getSystem()->memory()->peek(addr).value();
                if ((v >= 32) && (v <= 126)) {
                    cmd.setResult(QString::asprintf("*0x%04x = 0x%02x '%c' %s",
                        addr, v,
                        ((v >= 32) && (v <= 126)) ? (char)v : '.',
                        m_cpu->getSystem()->controller()->instructionWithOpcode(v).c_str()));
                } else {
                    cmd.setResult(QString::asprintf("*0x%04x = 0x%02x %s",
                        addr, v,
                        m_cpu->getSystem()->controller()->instructionWithOpcode(v).c_str()));
                }
                break;
            default:
                cmd.setError("Syntax error: too many arguments");
                break;
            }
        });

    ret->addCommandDefinition(ret, "bank", 2, 4,
        [this](Command& cmd) {
            BankCommand(this, cmd).execute();
        });

    return ret;
}

void MainWindow::commandResult(const QString& line, bool ok, const QString& result)
{
    addHistory(line);
    m_result->setText((result != "") ? result : (ok) ? "OK"
                                                     : "ERROR");
    m_result->setStyleSheet(QString("color: %1").arg((ok) ? "green" : "red"));
}

void MainWindow::addHistory(const QString& cmd)
{
    m_history->setText(cmd);
}

QVector<QString> MainWindow::fileCompletions(const QStringList& args)
{
    QVector<QString> ret;
    if (args.size() != 2) {
        return ret;
    }
    auto path = args[1].split("/");
    QString dirPath(path.mid(0, path.size() - 1).join("/"));
    QString entry(path[path.size() - 1]);
    QDir dir(dirPath);
    for (auto& e : dir.entryInfoList()) {
        if (((entry == "") || e.fileName().startsWith(entry)) && (e.fileName() != ".")) {
            ret.append(QString("%1 %2%3").arg(args[0], e.filePath(), (e.isDir()) ? "/" : ""));
        }
    }
    return ret;
}

QString MainWindow::query(const QString& prompt, const QString& options)
{
    return m_command->query(prompt, options);
}

void MainWindow::keyPressEvent(QKeyEvent* ev)
{
    emit keyPressed(ev);
    QMainWindow::keyPressEvent(ev);
}

BankCommand::BankCommand(MainWindow* win, Command& command)
    : cmd(command)
    , window(win)
{
}

void BankCommand::execute()
{
    QString lwr;

    if (window->cpu()->isRunning()) {
        cmd.setError("Error: Memory configuration cannot be changed when CPU is running");
        return;
    }

    lwr = cmd.arg(0).toLower();
    if (lwr == "add") {
        add();
    } else if (lwr == "del") {
        del();
    } else {
        cmd.setError(QString("Syntax error: invalid subcommand '%1").arg(cmd.arg(0)));
    }
}

word BankCommand::parseWord(int ix, QString&& err)
{
    bool ok;

    word ret = cmd.arg(ix).toUShort(&ok, 0);
    if (!ok) {
        cmd.setError(err.arg(cmd.arg(ix)));
    }
    return ret;
}

void BankCommand::add()
{
    word addr;
    word size;
    bool writable = true;
    QString lwr;

    if (cmd.numArgs() < 3) {
        cmd.setError("Syntax error: 'add' subcommand requires at least 2 parameters");
        return;
    }
    addr = parseWord(1, "Syntax error: unparsable address '%1'");
    if (!cmd.success()) {
        return;
    }
    size = parseWord(2, "Syntax error: unparsable size '%1'");
    if (!cmd.success()) {
        return;
    }
    if (cmd.numArgs() == 4) {
        lwr = cmd.arg(3).toLower();
        if ((lwr == "rom") || (lwr == "false")) {
            writable = false;
        } else if ((lwr != "ram") && (lwr != "true")) {
            cmd.setError(QString("Syntax error: unparsable writable flag '%1").arg(cmd.arg(2)));
            return;
        }
    }

    if (!window->cpu()->getSystem()->memory()->disjointFromAll(addr, size)) {
        cmd.setError(
            QString("Error: A block of size %1 at address %2 overlaps an existing block")
                .arg(size, addr));
    } else {
        if (!window->cpu()->getSystem()->memory()->add(addr, size, writable)) {
            cmd.setError(
                QString("Error: Could not add memory block of size %1 at address %2").arg(size, addr));
        } else {
            window->focusOnAddress(addr);
        }
    }
}

void BankCommand::del()
{
    word addr;

    if (cmd.numArgs() != 2) {
        cmd.setError("Syntax error: The 'del' subcommand requires exactly one parameter");
        return;
    }
    addr = parseWord(1, "Syntax error: unparsable address '%1'");
    if (!cmd.success()) {
        return;
    }

    auto bank = window->cpu()->getSystem()->memory()->bank(addr);
    if (!bank.valid()) {
        cmd.setError(QString("Error: No memory bank has address '%1'").arg(cmd.arg(1)));
        return;
    }
    if (window->query(QString("Are you sure you want to delete the %1 block size %2 starting at %3? (Y/N)")
                          .arg((bank.writable()) ? "RAM" : "ROM ")
                          .arg(bank.size(), 4, 16, QChar('0'))
                          .arg(bank.start(), 4, 16, QChar('0')),
            "yn")
        == "y") {
        bool wasSelected = window->currentBank() == bank;
        window->cpu()->getSystem()->memory()->remove(bank.start(), bank.size());
        if (wasSelected) {
            window->focusOnAddress(0x0000);
        }
    }
}

}

//#include "mainwindow.moc"
