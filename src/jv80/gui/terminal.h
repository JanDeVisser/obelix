/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <vector>

#include <QKeyEvent>
#include <QWidget>

#include <jv80/gui/componentview.h>

#define H 25
#define W 80
#define SZ W* H

namespace Obelix::JV80::GUI {

class VT100 {
public:
    enum Command {
        None = 0,
        SGR0,
        SGR1,
        SGR2,
        Dummy,
        SGR4,
        SGR5,
        SGR7,
        SGR8,
        DECSTBM,
        CUU,
        CUD,
        CUF,
        CUB,
        Home,
        CUP,
        IND,
        RI,
        NEL,
        DECSC_Save,
        DECSC_Restore,
        EL0,
        EL1,
        EL2,
        ED0,
        ED1,
        ED2,
        RIS,
    };

    enum FinalState {
        InProgress,
        Matched = 10,
        CantMatch = 11
    };

private:
    enum State {
        Start = 0,
        LBracket,
        LBracketNum,
        LBracketSemi,
        LBracketNumSemi,
        LBracketNumSemiNum,
        Done = 10,
        NoMatch = 11
    };

    State m_state = State::Start;
    Command m_command = None;
    int m_value1 = 0;
    int m_value2 = 0;
    std::vector<uchar> m_seq;
    std::function<void(uchar)> m_handlers[Done];

public:
    VT100();
    [[nodiscard]] Command command() const;
    [[nodiscard]] int value1() const;
    [[nodiscard]] int value2() const;
    void done(Command);
    FinalState handle(uchar);
};

class Terminal : public StyledWidget {
    Q_OBJECT

    int m_cursor = 0;
    int m_saved = 0;
    uchar screen[SZ];
    QPixmap img[256];
    uchar mask = 0;
    bool m_local = false;
    std::optional<VT100> m_vt100;

protected:
    void keyPressEvent(QKeyEvent* ev) override;
    void drawScreen();
    void flashCursor();
    void clearCursor();
    void setCursor(int, bool = false);
    void executeVT100Command(VT100::Command, int, int);
    void handleVT100(int);
    void setChar(uchar);
    void scrollUp(int);
    void scrollDown(int);

public:
    constexpr static int ESC = 0x1B;

    explicit Terminal(QWidget* = nullptr);
    void setLocalEcho(bool local) { m_local = local; }
    void moveCursor(int, int, bool = false);
    void setCursor(int, int, bool = false);
    void home();
    void up(int = 1);
    void down(int = 1);
    void left(int = 1);
    void right(int = 1);
    void nextLine();
    void send(int);

public slots:
    void writeCharacter(int ch) { send(ch); }

signals:
    void keyPressed(QKeyEvent* ev);
};

}
