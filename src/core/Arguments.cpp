//
// Created by Jan de Visser on 2021-09-20.
//

#include <core/Arguments.h>

namespace Obelix {

Arguments::Arguments()
    : Object("arguments")
    , m_args(make_typed<List>())
    , m_kwargs(make_typed<Dictionary>())
{
}

#if 0
Arguments::Arguments(Obj const& arg1, Obj const& arg2)
    : Arguments()
{
    m_args->push_back(arg1);
    if (arg2->type() != "null")
        m_args->push_back(arg2);
}
#endif

Arguments::Arguments(Ptr<List> args, Ptr<Dictionary> kwargs)
    : Object("arguments")
    , m_args(std::move(args))
    , m_kwargs(std::move(kwargs))

{
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
        if ((num >= 0) && ((size_t)num < m_args->size()))
            return ptr_cast<Object>(m_args[num]);
    } else if (m_kwargs->contains(name)) {
        return ptr_cast<Object>(m_kwargs->get(name).value());
    }
    return {};
}

}