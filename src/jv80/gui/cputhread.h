/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <deque>
#include <mutex>
#include <sstream>

#include "../../../../../Qt/6.2.1/macos/lib/QtCore.framework/Headers/QThread"
#include "../../../../../Qt/6.2.1/macos/lib/QtGui.framework/Headers/QKeyEvent"

#include <jv80/cpu/backplane.h>
#include <jv80/cpu/iochannel.h>

namespace Obelix::JV80::GUI {

using namespace Obelix::JV80::CPU;

class Executor : public QThread {
    Q_OBJECT
    BackPlane* m_system;
    word m_address;

public:
    explicit Executor(BackPlane* system, QObject* parent = nullptr)
        : QThread(parent)
        , m_system(system)
        , m_address(0xFFFF)
    {
    }
    void startAddress(word address) { m_address = address; }

protected:
    void run() override
    {
        m_system->run(m_address);
    }
};

class CPU : public QObject {
    Q_OBJECT

public:
    explicit CPU(QObject* = nullptr);
    ~CPU() override = default;
    BackPlane* getSystem() { return m_system; }
    void setRunMode(SystemBus::RunMode) const;
    void openImage(const QString&, word addr = 0, bool writable = true);
    void openImage(QFile&, word addr = 0, bool writable = true);
    void openImage(QFile&&, word addr = 0, bool writable = true);

    void run(word = 0xFFFF);
    void continueExecution();
    void step(word = 0xFFFF);
    void tick(word = 0xFFFF);
    void reset();
    void interrupt();
    bool isRunning() const;
    bool isHalted() const;
    bool isSuspended() const;

public slots:
    void keyPressed(QKeyEvent*);

signals:
    void executionStart();
    void executionEnded(const QString&);
    void executionInterrupted(const QString&);
    void terminalWrite(int);

private:
    Executor* m_thread;
    BackPlane* m_system;
    IOChannel* m_keyboard;
    IOChannel* m_terminal;
    bool m_running;
    std::stringstream m_status {};
    std::list<int> m_pressedKeys;
    std::list<int> m_queuedKeys;
    std::mutex m_kbdMutex;

    void start(SystemBus::RunMode);

private slots:
    void finished();
};

}
