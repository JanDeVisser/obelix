/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by Jan de Visser on 2021-09-29.
//

#include <core/Arguments.h>
#include <core/Dictionary.h>
#include <core/Logging.h>

namespace Obelix {

class MapIterator : public Object {
public:
    MapIterator(std::unordered_map<std::string, Obj>::const_iterator iter, std::unordered_map<std::string, Obj>::const_iterator end)
        : Object(TypeMapIterator)
        , m_iter(iter)
        , m_end(end)
    {
    }

    ~MapIterator() override = default;

    [[nodiscard]] Ptr<Object> copy() const override
    {
        return make_obj<MapIterator>(m_iter, m_end);
    }

    std::optional<Ptr<Object>> next() override
    {
        if (m_iter != m_end) {
            auto& pair = *m_iter;
            m_current = make_obj<NVP>(pair.first, pair.second);
            m_iter++;
            return m_current;
        } else {
            return {};
        }
    }

private:
    std::unordered_map<std::string, Obj>::const_iterator m_iter;
    std::unordered_map<std::string, Obj>::const_iterator m_end;
    Ptr<Object> m_current {};
};

Dictionary::Dictionary()
    : Object(TypeObject)
{
}

std::string Dictionary::to_string() const
{
    std::vector<std::string> pairs;
    for (const auto& p : m_dictionary) {
        pairs.emplace_back(p.first + ": " + p.second->to_string());
    }
    return "{ " + join(pairs, ", ") + " }";
}

std::optional<Obj> Dictionary::get(std::string const& key, Ptr<Object> default_result) const
{
    auto it = m_dictionary.find(key);
    if (it != m_dictionary.end())
        return (*it).second;
    return default_result;
}

std::optional<Obj> Dictionary::evaluate(std::string const& name, Ptr<Arguments> args)
{
    if (name == "has") {
        assert(!args->empty());
        return make_obj<Boolean>(contains(args[0]->to_string()));
    }
    if (name == "get") {
        assert(args->size() == 2);
        auto key = args[0]->to_string();
        auto default_value = args[1];
        if (contains(key))
            return get(key);
        return default_value;
    }
    return Object::evaluate(name, args);
}

std::optional<Obj> Dictionary::resolve(std::string const& name) const
{
    if (m_dictionary.contains(name)) {
        return get(name);
    }
    return Object::resolve(name);
}

std::optional<Obj> Dictionary::assign(std::string const& name, Obj const& value)
{
    m_dictionary[name] = value;
    return value;
}

std::optional<Ptr<Object>> Dictionary::iterator() const
{
    auto iter = m_dictionary.begin();
    auto end = m_dictionary.end();
    return make_obj<MapIterator>(iter, end);
}

void Dictionary::put(Ptr<NVP> nvp)
{
    m_dictionary[nvp->name()] = nvp->value();
}

[[nodiscard]] bool Dictionary::contains(std::string const& key) const
{
    return m_dictionary.contains(key);
}

[[nodiscard]] std::optional<Obj> Dictionary::get(std::string const& key) const
{
    auto it = m_dictionary.find(key);
    if (it != m_dictionary.end())
        return (*it).second;
    return {};
}

}
