/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Obelix {

template<typename Callback>
class ScopeGuard {
public:
    explicit ScopeGuard(Callback const& callback)
        : m_callback(callback)
    {
    }

    ~ScopeGuard()
    {
        m_callback();
    }

private:
    Callback const& m_callback;
};

}
