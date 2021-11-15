//
// Created by Jan de Visser on 2021-09-20.
//

#pragma once

#include <unordered_map>
#include <core/Dictionary.h>
#include <core/List.h>
#include <core/Object.h>

namespace Obelix {

class Arguments : public Object {
public:
    Arguments();

    template <class... Args>
    explicit Arguments(Args&&... args)
        : Arguments()
    {
        add(std::forward<Args>(args)...);
    }

    Arguments(Ptr<List>, Ptr<Dictionary>);

    [[nodiscard]] size_t size() const override { return m_args->size(); }
    [[nodiscard]] bool empty() const override { return m_args->empty(); }
    [[nodiscard]] Obj const& get(size_t ix) const { return m_args[ix]; }
    [[nodiscard]] Obj const& at(size_t ix) override { return m_args[ix]; }
    [[nodiscard]] size_t kwsize() const { return m_kwargs->size(); }

    [[nodiscard]] std::optional<Obj> get(std::string const& keyword) const
    {
        return m_kwargs->get(keyword);
    }

    // FIXME: Move the template circus to the List class.
    template <class... Args>
    void add(Obj const& arg, Args&&... args)
    {
        add(arg);
        add(std::forward<Args>(args)...);
    }

    template <class ObjClass, class... Args>
    void add(Ptr<ObjClass> const& arg, Args&&... args)
    {
        add(to_obj(arg));
        add(std::forward<Args>(args)...);
    }


    template <class... Args>
    void add(Ptr<NVP> const& kwarg, Args&&... args)
    {
        add(kwarg);
        add(std::forward<Args>(args)...);
    }

    template <class... Args>
    void add(std::string arg, Args&&... args)
    {
        add(arg);
        add(std::forward<Args>(args)...);
    }

    template <class... Args>
    void add(long arg, Args&&... args)
    {
        add(arg);
        add(std::forward<Args>(args)...);
    }

// Why does this not work? Gives
// In file included from /Users/jan/projects/obelix2/src/core/test/Arguments.cpp:6:
// /Users/jan/projects/obelix2/src/core/Arguments.h:69:9: error: no matching member function for call to 'add'
//        add(std::forward<Args>(args)...);
// in the Arguments test?
//    template <class... Args>
//    void add(double arg, Args&&... args)
//    {
//        add(arg);
//        add(std::forward<Args>(args)...);
//    }

    template<class ObjClass>
    void add(Ptr<ObjClass> const& obj)
    {
        m_args->push_back(to_obj(obj));
    }

    void add(Obj const& obj)
    {
        m_args->push_back(obj);
    }

    void add(Ptr<NVP> const& nvp)
    {
        m_kwargs->put(nvp);
    }

    void add(std::string str)
    {
        m_args->push_back(make_obj<String>(str));
    }

    void add(long i)
    {
        m_args->push_back(make_obj<Integer>(i));
    }

    void add(double dbl)
    {
        m_args->push_back(make_obj<Float>(dbl));
    }

    [[nodiscard]] Ptr<List> const& arguments() const { return m_args; }
    [[nodiscard]] Ptr<Dictionary> const& kwargs() const { return m_kwargs; }
    std::optional<Obj> evaluate(std::string const&, Ptr<Arguments>) override;
    [[nodiscard]] std::optional<Obj> resolve(std::string const&) const override;

private:
    Ptr<List> m_args;
    Ptr<Dictionary> m_kwargs;
};

}