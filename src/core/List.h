//
// Created by Jan de Visser on 2021-09-24.
//

#pragma once

#include <core/Object.h>

namespace Obelix {

class List : public Object {
public:
    List()
        : Object(TypeList)
    {
    }

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] size_t size() const override { return m_list.size(); }
    std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const& name) const override;

    template <class ObjClass, class... Args>
    void emplace_back(Args&&... args)
    {
        m_list.push_back(make_obj<ObjClass>(std::forward<Args>(args)...));
    }

    template<class ObjClass>
    void push_back(Ptr<ObjClass> const& elem)
    {
        m_list.push_back(elem);
    }

    Obj const& at(size_t ix) override
    {
        assert(ix < size());
        return m_list.at(ix);
    }

    [[nodiscard]] Obj const& at(size_t ix) const override
    {
        assert(ix < size());
        return m_list.at(ix);
    }

    template<class ObjClass>
    Ptr<ObjClass> at_typed(size_t ix)
    {
        return ptr_cast<ObjClass>(at(ix));
    }

private:
    std::vector<Obj> m_list {};
};

}
