/*
 * This file is part of obelix (https://github.com/JanDeVisser/obelix}).
 *
 * (c) Copyright Jan de Visser 2014-2021
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstring>
#include <cstdio>
#include <gtest/gtest.h>
#include <data.h>

constexpr const char * TEST_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr const char * TEST_STRING_LOWER = "abcdefghijklmnopqrstuvwxyz0123456789";
constexpr const char * TEST_STRING_UPTO_5 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";
constexpr const char * TEST_STRING_MUTATION_0 = "QBCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr const char * TEST_STRING_MUTATION_5 = "ABCDEQGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr const char * TEST_STRING_MUTATION_35 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345678Q";
constexpr const char * TEST_SLICE = "KLMNOPQRST";
constexpr const char * TEST_SLICE_START = "ABCDEFGHIJ";
constexpr const char * TEST_SLICE_END = "0123456789";
constexpr const char * ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr const char * DIGITS = TEST_SLICE_END;
constexpr const char * FMT = "%d + %d = %d";
constexpr int          TEST_STRING_LEN = 36;
constexpr int          ALPHABET_LEN = 26;

class StrTest : public ::testing::Test {
public:
  str_t       *str { NULL };
  data_t      *data { NULL };
  exception_t *e { NULL };
protected:
  void SetUp() override {
  }

  void TearDown() override {
    if (data != data_as_data(str)) {
      data_release(data);
    }
    data_release(str);
    str = NULL;
    data = NULL;
    e = NULL;
  }

  str_t * str_append_va_list_maker(const char *fmt, ...) const {
    va_list args;
    va_start(args, fmt);
    str_t *ret = str_append_vprintf(str, fmt, args);
    va_end(args);
    return ret;
  };

};
