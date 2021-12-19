/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>

namespace Obelix {

class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void(void)> callback)
        : m_callback(move(callback))
    {
    }

    ~ScopeGuard()
    {
        m_callback();
    }

private:
    std::function<void(void)> m_callback;
};

}
