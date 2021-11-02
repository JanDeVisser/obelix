//
// Created by Jan de Visser on 2021-09-28.
//

#include <core/Object.h>
#include <core/Arguments.h>
#include <core/Dictionary.h>
#include <core/List.h>

#include <gtest/gtest.h>

TEST(Object, Instantiate)
{
    Obelix::Integer i(42);
    EXPECT_EQ(i.to_long().has_value(), true);
    EXPECT_EQ(i.to_long().value(), 42);
}

TEST(Object, InstantiateHandle)
{
    auto i = Obelix::make_typed<Obelix::Integer>(42);
    EXPECT_EQ(i->to_long().has_value(), true);
    EXPECT_EQ(i->to_long().value(), 42);
    auto o = Obelix::make_obj<Obelix::Integer>(42);
    EXPECT_EQ(o->to_long().has_value(), true);
    EXPECT_EQ(o->to_long().value(), 42);
}

TEST(Object, List)
{
    auto list = Obelix::make_typed<Obelix::List>();
    EXPECT_EQ(list->size(), 0);
    auto i = Obelix::make_obj<Obelix::Integer>(42);
    auto j = Obelix::make_obj<Obelix::Integer>(12);
    list->push_back(i);
    EXPECT_EQ(list->size(), 1);
    list->push_back(j);
    EXPECT_EQ(list->size(), 2);

    long sum = 0;
    for (auto& elem : list) {
        auto int_maybe = elem->to_long();
        EXPECT_TRUE(int_maybe.has_value());
        sum += int_maybe.value();
    }
    EXPECT_EQ(sum, 42 + 12);
}

TEST(Object, Dictionary)
{
    auto dict = Obelix::make_typed<Obelix::Dictionary>();
    EXPECT_EQ(dict->size(), 0);
    dict->put("42", Obelix::make_obj<Obelix::Integer>(42));
    EXPECT_EQ(dict->size(), 1);
    EXPECT_EQ(dict->get("42").value()->to_string(), "42");
    dict->put("12", Obelix::make_obj<Obelix::Integer>(12));
    EXPECT_EQ(dict->get("12").value()->to_string(), "12");
    EXPECT_EQ(dict->get("42").value()->to_string(), "42");
    EXPECT_EQ(dict->size(), 2);
    long sum = 0;
    for (Obelix::Ptr<Obelix::Object> const& elem : dict) {
        auto nvp = Obelix::ptr_cast<Obelix::NVP>(elem);
        auto int_maybe = nvp->value()->to_long();
        EXPECT_TRUE(int_maybe.has_value());
        sum += int_maybe.value();
    }
    EXPECT_EQ(sum, 42 + 12);
}

TEST(Object, HandleAdd)
{
    auto i = Obelix::make_obj<Obelix::Integer>(42);
    auto j = Obelix::make_obj<Obelix::Integer>(42);
    auto args = Obelix::make_typed<Obelix::Arguments>();
    args->add(j);
    auto sum_maybe = i->evaluate("+", args);
    EXPECT_TRUE(sum_maybe.has_value());
    auto sum = sum_maybe.value();
    EXPECT_TRUE(sum->to_long().has_value());
    EXPECT_EQ(sum->to_long().value(), 84);
}
