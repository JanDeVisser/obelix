/*
 * obelix/src/lexer/test/tlexer.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstring>
#include <cstdio>
#include <gtest/gtest.h>
#include <lexa.h>
#include <heap.h>

class LexerTest : public ::testing::Test {
public:
  lexa_t * lexa { NULL };

protected:
  void SetUp() override {
    if (debugOn()) {
      logging_enable("lexer");
      logging_set_level("DEBUG");
    }
    lexa = lexa_create();
    withScanners();
    ASSERT_TRUE(lexa);
  }

  lexa_t * withScanners() const {
    lexa_add_scanner(lexa, "identifier");
    lexa_add_scanner(lexa, "whitespace");
    lexa_add_scanner(lexa, "qstring: quotes='`\"");
    return lexa;
  }

  void TearDown() override {
    data_release(lexa);
    lexa = NULL;
    heap_gc();
  }

  virtual bool debugOn() {
    return false;
  }

};
