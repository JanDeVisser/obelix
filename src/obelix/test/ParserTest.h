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
    bool show_tree = false;

protected:
    void SetUp() override
    {
        if (debugOn()) {
            Logger::get_logger().enable("parser");
        }
    }

    Ptr<Scope> parse(std::string const& s)
    {
        Runtime::Config config { show_tree };
        runtime = new Runtime(config, false);
        return runtime->evaluate(s);
    }

    Ptr<Scope> parse(std::string const& s, Ptr<Scope> const& scope)
    {
        Runtime::Config config { show_tree };
        runtime = new Runtime(config, false);
        auto ret = runtime->evaluate(s, scope);
        EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
        if (ret->result().code == ExecutionResultCode::Error) {
            auto errors = ptr_cast<List>(ret->result().return_value);
            for (auto const& error : errors) {
                fprintf(stderr, "%s\n", error->to_string().c_str());
            }
        }
        return ret;
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