/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QHBoxLayout>
#include <QSvgRenderer>
#include <QWidget>

namespace Obelix::JV80::GUI {

class QLed : public QWidget {
    Q_OBJECT

public:
    enum Shape {
        Circle = 0,
        Round = 1,
        Square = 2,
        Triangle = 3,
    };

    enum Colour {
        Red = 0,
        Green = 1,
        Yellow = 2,
        Grey = 3,
        Orange = 4,
        Purple = 5,
        Blue = 6,
    };

private:
    static QColor COLOURS[7];
    static const char* SHAPES[4];

    bool m_value = false;
    Colour m_colourOn = Red;
    Colour m_colourOff = Grey;
    Shape m_shape = Circle;
    bool m_clickable = false;

    bool isPressed = false;
    QSvgRenderer renderer;

signals:
    void clicked();
    void pressed(bool);

public:
    QLed(Shape = Circle, QWidget* parent = nullptr);

    bool value() { return m_value; }
    void setValue(bool);
    void toggleValue() { setValue(!m_value); }
    Colour colourOn() { return m_colourOn; }
    void setColourOn(Colour);
    Colour colourOff() { return m_colourOn; }
    void setColourOff(Colour);
    Shape shape() { return m_shape; }
    void setShape(Shape);
    bool isClickable() { return m_clickable; }
    void setClickable(bool);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
};

class QLedArray : public QWidget {
    Q_OBJECT

public:
    QLedArray(int, int = 16, QWidget* = nullptr);
    int numberOfLeds() const { return m_numLeds; }
    void setColour(int, QLed::Colour);
    void setColourForAll(QLed::Colour);
    void setColourForNibble(int, QLed::Colour);
    void setColourForByte(int, QLed::Colour);
    QLed::Colour colour(int) const;
    void setShape(QLed::Shape);
    QLed::Shape shape() const;
    void setValue(unsigned int);
    unsigned int value() const;

private:
    unsigned int m_value;
    int m_numLeds;
    QHBoxLayout* m_layout;
    QLed** m_leds;
};

}
