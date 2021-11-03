//
// Created by Jan de Visser on 2021-10-07.
//

#pragma once

#include <gtest/gtest.h>
#include <obelix/Parser.h>
#include <obelix/Runtime.h>

namespace Obelix {

class ParserTest : public ::testing::Test {
public:
    Runtime* runtime;

protected:
    void SetUp() override
    {
        if (debugOn()) {
            Logger::get_logger().enable("lexer");
            Logger::get_logger().enable("parser");
        }
    }

    Ptr<Scope> parse(std::string const& s)
    {
        Runtime::Config config { false };
        runtime = new Runtime(config, false);
        return runtime->evaluate(s);
    }

    void TearDown() override
    {
    }

    virtual bool debugOn()
    {
        return false;
    }
};

}