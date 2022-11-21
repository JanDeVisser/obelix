/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <core/Object.h>

namespace Obelix {

class SimpleIterator : public Object {
public:
    explicit SimpleIterator(Object const&, size_t = 0);
    ~SimpleIterator() override = default;

    [[nodiscard]] Ptr<Object> copy() const override;
    std::optional<Ptr<Object>> next() override;

private:
    Object const& m_container;
    size_t m_index { 0 };
};

}
