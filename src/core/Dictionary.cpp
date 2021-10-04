//
// Created by Jan de Visser on 2021-09-29.
//

#include <core/Dictionary.h>

namespace Obelix {

std::string Dictionary::to_string() const
{
    std::vector<std::string> pairs;
    for (const auto & p : m_dictionary) {
        pairs.emplace_back(p.first + ": " + p.second.to_string());
    }
    return "{ " + join(pairs, ", ") + " }";
}

std::optional<Obj> Dictionary::evaluate(std::string const& name, Ptr<Arguments> args)
{
    return Object::evaluate(name, args);
}

[[nodiscard]] std::optional<Obj> Dictionary::resolve(std::string const& name) const
{
    return Object::resolve(name);
}

}