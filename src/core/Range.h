/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/Object.h>

namespace Obelix {

class Range : public Object {
public:
    Range(Obj const&, Obj const&);

    [[nodiscard]] std::string to_string() const override;
    std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments>) const override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const& name) const override;
    [[nodiscard]] std::optional<Obj> iterator() const override;

    [[nodiscard]] Obj const& low() const { return m_low; }
    [[nodiscard]] Obj const& high() const { return m_high; }

private:
    Obj m_low;
    Obj m_high;
};

}
