/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include <jv80/gui/terminal.h>

namespace Obelix::JV80::GUI {

#include "bitmap.inc"

VT100::VT100()
    : m_seq()
{
    m_handlers[Start] = [this](uchar key) {
        switch (key) {
        case '[':
            m_state = LBracket;
            break;
        case 'c':
            done(RIS);
            break;
        case 'D':
            done(IND);
            break;
        case 'E':
            done(NEL);
            break;
        case 'M':
            done(RI);
            break;
        case '7':
            done(DECSC_Save);
            break;
        case '8':
            done(DECSC_Restore);
            break;
        default:
            m_state = NoMatch;
            break;
        }
    };
    m_handlers[LBracket] = [this](uchar key) {
        if ((key >= '0') && (key <= '9')) {
            m_value1 = key - '0';
            m_state = LBracketNum;
        } else if (key == ';') {
            m_state = LBracketSemi;
        } else if ((key == 'H') || (key == 'f')) {
            done(Home);
        } else if (key == 'J') {
            done(ED0);
        } else if (key == 'K') {
            done(EL0);
        } else if (key == 'm') {
            done(SGR0);
        } else {
            m_state = NoMatch;
        }
    };
    m_handlers[LBracketSemi] = [this](uchar key) {
        if ((key == 'H') || (key == 'f')) {
            done(Home);
        } else {
            m_state = NoMatch;
        }
    };
    m_handlers[LBracketNum] = [this](uchar key) {
        if ((key >= '0') && (key <= '9')) {
            m_value1 = (key - '0') + m_value1 * 10; // FIXME Deal with theoretical values >= 100
        } else if (key == ';') {
            m_state = LBracketNumSemi;
        } else if (key == 'A') {
            done(CUU);
        } else if (key == 'B') {
            done(CUD);
        } else if (key == 'C') {
            done(CUF);
        } else if (key == 'D') {
            done(CUB);
        } else if (key == 'J') {
            if (m_value1 >= 3) {
                m_state = NoMatch;
            } else {
                done((Command)(ED0 + m_value1));
            }
        } else if (key == 'm') {
            if ((m_value1 == 3) || (m_value1 > 8)) {
                m_state = NoMatch;
            } else {
                done((Command)(SGR0 + m_value1));
            }
        } else {
            m_state = NoMatch;
        }
    };
    m_handlers[LBracketNumSemi] = [this](uchar key) {
        if ((key >= '0') && (key <= '9')) {
            m_value2 = key - '0';
            m_state = LBracketNumSemiNum;
        } else {
            m_state = NoMatch;
        }
    };
    m_handlers[LBracketNumSemiNum] = [this](uchar key) {
        if ((key >= '0') && (key <= '9')) {
            m_value2 = (key - '0') + m_value2 * 10; // FIXME Deal with theoretical values >= 100
        } else if (key == 'r') {
            done(DECSTBM);
        } else if ((key == 'H') || (key == 'f')) {
            done(CUP);
        } else {
            m_state = NoMatch;
        }
    };
}

VT100::Command VT100::command() const
{
    return m_command;
}

int VT100::value1() const
{
    return m_value1;
}

int VT100::value2() const
{
    return m_value2;
}

void VT100::done(Command cmd)
{
    m_state = Done;
    m_command = cmd;
}

VT100::FinalState VT100::handle(uchar key)
{
    m_seq.push_back(key);
    m_handlers[m_state](key);
    return (m_state < Done) ? InProgress : (FinalState)m_state;
}

/* ======================================================================= */

Terminal::Terminal(QWidget* parent)
    : StyledWidget(parent)
    , m_vt100()
{
    auto layout = new QGridLayout;
    layout->setHorizontalSpacing(0);
    layout->setVerticalSpacing(0);
    this->setLayout(layout);

    for (int ch = 0; ch < 128; ch++) {
        auto bm = QImage(bitmaps[ch], 8, 8, 1, QImage::Format_MonoLSB);
        bm = bm.scaled(bm.size() * 2);
        img[ch] = QPixmap::fromImage(bm);
        bm.invertPixels();
        img[ch + 128] = QPixmap::fromImage(bm);
    }
    for (int ix = 0; ix < SZ; ix++) {
        screen[ix] = ' ';
        auto label = new QLabel();
        label->setPixmap(img[(int)' ']);
        layout->addWidget(label, ix / W, ix % W);
    }
    drawScreen();
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Terminal::flashCursor);
    timer->start(500);
}

void Terminal::drawScreen()
{
    auto layout = dynamic_cast<QGridLayout*>(this->layout());
    for (int ix = 0; ix < SZ; ix++) {
        auto label = dynamic_cast<QLabel*>(layout->itemAtPosition(ix / W, ix % W)->widget());
        label->setPixmap(img[screen[ix]]);
    }
}

void Terminal::flashCursor()
{
    auto layout = dynamic_cast<QGridLayout*>(this->layout());
    auto label = dynamic_cast<QLabel*>(layout->itemAtPosition(m_cursor / W, m_cursor % W)->widget());
    label->setPixmap(img[screen[m_cursor] | mask]);
    mask ^= 0x80;
}

void Terminal::clearCursor()
{
    auto layout = dynamic_cast<QGridLayout*>(this->layout());
    auto label = dynamic_cast<QLabel*>(layout->itemAtPosition(m_cursor / W, m_cursor % W)->widget());
    label->setPixmap(img[screen[m_cursor]]);
}

void Terminal::scrollUp(int lines)
{
    for (int ix = W; ix < SZ; ix++) {
        screen[ix - W] = screen[ix];
    }
    for (int ix = SZ - W; ix < SZ; ix++) {
        screen[ix] = ' ';
    }
    m_cursor -= W;
    if (lines > 1) {
        scrollUp(lines - 1);
    }
    drawScreen();
}

void Terminal::scrollDown(int lines)
{
    for (int ix = SZ - (2 * W); ix >= 0; ix--) {
        screen[ix + W] = screen[ix];
    }
    for (int ix = 0; ix < W; ix++) {
        screen[ix] = ' ';
    }
    m_cursor += W;
    if (lines > 1) {
        scrollDown(lines - 1);
    }
    drawScreen();
}

void Terminal::setChar(uchar ch)
{
    screen[m_cursor++] = ch;
    if (m_cursor >= SZ) {
        scrollUp(1);
        m_cursor -= W;
    }
    drawScreen();
}

void Terminal::moveCursor(int deltaRow, int deltaCol, bool scroll)
{
    setCursor(m_cursor + ((W * deltaRow) + deltaCol), scroll);
}

void Terminal::setCursor(int row, int col, bool scroll)
{
    setCursor(row * W + col, scroll);
}

void Terminal::setCursor(int new_cursor, bool scroll)
{
    clearCursor();
    if (((new_cursor >= 0) && (new_cursor < SZ)) || scroll) {
        m_cursor = new_cursor;
    }
    if (scroll) {
        if (new_cursor < 0) {
            while (m_cursor < 0) {
                scrollDown(1);
            }
        } else {
            while (m_cursor >= SZ) {
                scrollUp(1);
            }
        }
    }
}

void Terminal::home()
{
    clearCursor();
    m_cursor = 0;
}

void Terminal::up(int lines)
{
    setCursor(m_cursor - (lines * W));
}

void Terminal::down(int lines)
{
    setCursor(m_cursor + (lines * W));
}

void Terminal::left(int cols)
{
    setCursor(m_cursor - cols);
}

void Terminal::right(int cols)
{
    setCursor(m_cursor + cols);
}

void Terminal::nextLine()
{
    moveCursor(1, -(m_cursor % W), true);
}

void Terminal::executeVT100Command(VT100::Command command, int value1, int value2)
{
    switch (command) {
    case VT100::CUP:
        setCursor(value1, value2);
        break;
    case VT100::CUU:
        moveCursor(-value1, 0);
        break;
    case VT100::CUD:
        moveCursor(value1, 0);
        break;
    case VT100::CUF:
        moveCursor(0, value1);
        break;
    case VT100::CUB:
        moveCursor(0, -value1);
        break;
    case VT100::Home:
        home();
        break;
    case VT100::NEL:
        nextLine();
        break;
    case VT100::DECSC_Save:
        m_saved = m_cursor;
        break;
    case VT100::DECSC_Restore:
        if (m_saved >= 0) {
            m_cursor = m_saved;
            m_saved = -1;
        }
        break;
    default:
        break;
    }
}

void Terminal::handleVT100(int key)
{
    if (!m_vt100 && (key == ESC)) {
        m_vt100 = VT100();
    } else {
        switch (m_vt100->handle(key)) {
        case VT100::Matched:
            executeVT100Command(m_vt100->command(), m_vt100->value1(), m_vt100->value2());
            // Fall through
        case VT100::CantMatch:
            m_vt100.reset();
            break;
        default:
            break;
        }
    }
}

void Terminal::send(int key)
{
    if (m_vt100 || (key == ESC)) {
        handleVT100(key);
    } else if ((key >= ' ') && (key <= '~')) {
        setChar((uchar)key);
    } else if (key == '\n') {
        clearCursor();
        moveCursor(1, -(m_cursor % W));
    }
}

void Terminal::keyPressEvent(QKeyEvent* ev)
{
    emit keyPressed(ev);
    if (m_local) {
        if (m_vt100 || (ev->key() == Qt::Key_Escape)) {
            handleVT100(ev->key());
        } else if ((ev->key() >= Qt::Key_A) && (ev->key() <= Qt::Key_Z)) {
            auto ch = (uchar)ev->key() + 32;
            if (ev->modifiers() & Qt::ShiftModifier) {
                ch -= 32;
            }
            setChar(ch);
        } else if ((ev->key() >= Qt::Key_Space) && (ev->key() <= Qt::Key_AsciiTilde)) {
            setChar((uchar)ev->key());
        } else if ((ev->key() == Qt::Key_Up) && (m_cursor >= W)) {
            up();
        } else if ((ev->key() == Qt::Key_Down) && (m_cursor < (SZ - W))) {
            down();
        } else if ((ev->key() == Qt::Key_Left) && (m_cursor > 0)) {
            left();
        } else if ((ev->key() == Qt::Key_Right) && (m_cursor < (SZ - 1))) {
            right();
        } else if ((ev->key() == Qt::Key_Backspace) && (m_cursor > 0)) {
            screen[--m_cursor] = ' ';
            drawScreen();
        } else if ((ev->key() == Qt::Key_Enter) || (ev->key() == Qt::Key_Return)) {
            nextLine();
        }
    }
}

}
