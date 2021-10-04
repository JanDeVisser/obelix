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
    Dictionary()
        : Object("dictionary")
    {
    }

    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] size_t size() const override { return m_dictionary.size(); }
    std::optional<Obj> evaluate(std::string const& name, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const& name) const override;

    template <class ObjClass>
    void put(std::string const& key, ObjClass value)
    {
        m_dictionary[key] = ptr_cast<Object>(value);
    }

    std::optional<Obj> get(std::string const& key)
    {
        if (!m_dictionary.contains(key))
            return {};
        return m_dictionary[key];
    }

    template <class ObjClass>
    Ptr<ObjClass> get_typed(std::string const& key)
    {
        return ptr_cast<ObjClass>(get(key));
    }

    [[nodiscard]] IteratorState * iterator_state(IteratorState::IteratorWhere where) override
    {
        auto end = m_dictionary.end();
        auto iter = end;
        if (where == IteratorState::IteratorWhere::Begin)
            iter = m_dictionary.begin();
        return new MapIteratorState(*this, iter, end);
    }

private:
    class MapIteratorState : public IteratorState {
    public:
        MapIteratorState(Object& container, std::unordered_map<std::string, Obj>::iterator iter,
                         std::unordered_map<std::string, Obj>::iterator end)
            : IteratorState(container)
            , m_iter(iter)
            , m_end(end)
        {
        }

        ~MapIteratorState() override = default;

       void increment(ptrdiff_t delta) override
        {
            assert(delta >= 0);
            while (delta-- > 0) {
                m_iter++;
            }
        }

        std::shared_ptr<Object> dereference() override
        {
            if (m_iter != m_end) {
                auto& pair = *m_iter;
                auto nvp = std::make_shared<NVP>(pair.first, pair.second);
                return std::dynamic_pointer_cast<Object>(nvp);
            }
            return nullptr;
        }

        [[nodiscard]] IteratorState * copy() const override
        {
            return new MapIteratorState(*this);
        }

        [[nodiscard]] bool equals(IteratorState const* other) const override
        {
            auto other_casted = dynamic_cast<MapIteratorState const*>(other);
            //        assert(container() == other_casted.container());
            return m_iter == other_casted->m_iter;
        }

    private:
        MapIteratorState(MapIteratorState const& original) = default;
        std::unordered_map<std::string, Obj>::iterator m_iter;
        std::unordered_map<std::string, Obj>::iterator m_end;
    };
    friend MapIteratorState;

    std::unordered_map<std::string, Obj> m_dictionary {};
};

}
