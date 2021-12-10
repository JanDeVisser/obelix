/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QProxyStyle>

namespace Obelix::JV80::GUI {

class BlockCursorStyle : public QProxyStyle {
public:
    explicit BlockCursorStyle(QStyle* = nullptr);
    int pixelMetric(PixelMetric, const QStyleOption*, const QWidget*) const override;
};

}
