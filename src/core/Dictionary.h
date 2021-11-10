//
// Created by Jan de Visser on 2021-09-29.
//


#pragma once

#include <optional>
#include <unordered_map>
#include <core/Object.h>

namespace Obelix {

class Dictionary : public Object {
public:
    Dictionary();

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] size_t size() const override { return m_dictionary.size(); }
    std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const& name) const override;
    [[nodiscard]] std::optional<Obj> assign(std::string const&, std::string const&, Obj const&) override;
    [[nodiscard]] std::optional<Obj> get(std::string const& key, Ptr<Object> default_result) const;
    [[nodiscard]] IteratorState* iterator_state(IteratorState::IteratorWhere where) override;
    void put(Ptr<NVP> nvp);
    [[nodiscard]] bool contains(std::string const& key) const;
    [[nodiscard]] std::optional<Obj> get(std::string const& key) const;

    template<class ObjClass>
    void put(std::string const& key, ObjClass value)
    {
        m_dictionary[key] = ptr_cast<Object>(value);
    }

    template<class ObjClass>
    Ptr<ObjClass> get_typed(std::string const& key)
    {
        return ptr_cast<ObjClass>(get(key));
    }

private:
    friend class MapIteratorState;

    std::unordered_map<std::string, Obj> m_dictionary {};
};

}
