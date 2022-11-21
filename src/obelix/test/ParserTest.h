/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

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
    Ptr<Runtime> runtime;
    bool show_tree = false;

protected:
    void SetUp() override
    {
        if (debugOn()) {
            Logger::get_logger().enable("all");
        }
        Config config { show_tree };
        runtime = make_typed<Runtime>(config, false);
    }

    Ptr<Scope> parse(std::string const& s)
    {
        return runtime->eval(s);
    }

    Ptr<Scope> parse(std::string const& s, Ptr<Scope>& scope)
    {
        auto ret = scope->eval(s);
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
