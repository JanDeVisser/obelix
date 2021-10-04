//
// Created by Jan de Visser on 2021-09-20.
//

#pragma once

#include <unordered_map>
#include <core/List.h>
#include <core/Object.h>

namespace Obelix {

class Arguments : public Object {
public:
    Arguments();
    explicit Arguments(Obj const& args);
    Arguments(Obj const&, std::unordered_map<std::string, Obj>);
#if 0
    explicit Arguments(int, ...);
#endif

    [[nodiscard]] size_t size() const override { return m_args->size(); }
    [[nodiscard]] Obj const& get(size_t ix) const { return m_args[ix]; }
    [[nodiscard]] Obj const& at(size_t ix) const { return m_args[ix]; }

    [[nodiscard]] std::optional<Obj> get(std::string const& keyword) const
    {
        if (m_kwargs.contains(keyword))
            return m_kwargs.at(keyword);
        return {};
    }

    void add(Obj const& obj)
    {
        m_args->push_back(obj);
    }

    [[nodiscard]] Ptr<List> const& arguments() const { return m_args; }
    [[nodiscard]] std::unordered_map<std::string, Obj> const& kwargs() const { return m_kwargs; }
    std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const&) const override;

private:
    Ptr<List> m_args;
    std::unordered_map<std::string, Obj> m_kwargs {};
};

}