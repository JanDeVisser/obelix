/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/gui/blockcursorstyle.h>

namespace Obelix::JV80::GUI {

BlockCursorStyle::BlockCursorStyle(QStyle* style)
    : QProxyStyle(style)
{
}

int BlockCursorStyle::pixelMetric(PixelMetric metric,
    const QStyleOption* option,
    const QWidget* widget) const
{
    if (metric == QStyle::PM_TextCursorWidth) {
        return 10;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

}
