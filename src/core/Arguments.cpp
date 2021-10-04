//
// Created by Jan de Visser on 2021-09-20.
//

#include <core/Arguments.h>

namespace Obelix {

Arguments::Arguments()
    : Object("arguments")
    , m_args(make_typed<List>())
{
}

Arguments::Arguments(Obj const& args)
    : Arguments()
{
    for (auto& arg : args) {
        m_args->push_back(arg);
    }
}

Arguments::Arguments(Obj const& args, std::unordered_map<std::string, Obj> kwargs)
    : Object("arguments")
    , m_args(make_typed<List>())
    , m_kwargs(move(kwargs))

{
    for (auto& arg : args) {
        m_args->push_back(arg);
    }
}

std::optional<Obj> Arguments::evaluate(std::string const& name, Ptr<Arguments> args)
{
    return {};
}

std::optional<Obj> Arguments::resolve(std::string const& name) const
{
    auto num_maybe = Obelix::to_long(name);
    if (num_maybe.has_value()) {
        auto num = num_maybe.value();
        if ((num >= 0) && (num < m_args.size()))
            return ptr_cast<Object>(m_args[num]);
    } else if (m_kwargs.contains(name)) {
        return ptr_cast<Object>(m_kwargs.at(name));
    }
    return {};
}

}