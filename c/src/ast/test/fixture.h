/* 
 * This file is part of the obelix distribution (https://github.com/JanDeVisser/obelix).
 * Copyright (c) 2021 Jan de Visser.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gtest/gtest.h>

#include <file.h>
#include <grammarparser.h>
#include <parser.h>
#include "tast.h"

class ASTFixture : public ::testing::Test {
protected:
  virtual const char * get_grammar() const = 0;

  void SetUp() override {
    const char *grammar_path = get_grammar();

    logging_set_level("DEBUG");
    logging_enable("ast");
    logging_enable("parser");
    file = file_open(grammar_path);
    EXPECT_TRUE(file_isopen(file)) ;
    gp = grammar_parser_create((data_t *) file);
    EXPECT_TRUE(gp) ;
    grammar = grammar_parser_parse(gp);
    EXPECT_TRUE(grammar) ;
    parser = parser_create(gp -> grammar);
    EXPECT_TRUE(parser) ;
  }

  void parse(const char *s) {
    str_t *text = str(s);
    data_t *ret;

    // grammar_dump(grammar);
    parser->data = NULL;
    ret = parser_parse(parser, (data_t *) text);
    str_free(text);
    if (ret) {
      error("parser_parse: %s", data_tostring(ret));
    }
    EXPECT_FALSE(ret);
  }

  data_t * evaluate(const char *s, int expected) {
    parse(s);
    data_t *result = data_as_data(parser->data);
    EXPECT_TRUE(result) ;
    EXPECT_EQ(data_type(result), ASTBlock);

    return result;
  }

  data_t * execute(const char *str, int expected) {
    parse(str);
    data_t *script = data_as_data(parser->data);
    EXPECT_TRUE(script) ;
    EXPECT_TRUE(data_is_ast_Node(script));

    dictionary_t *ctx = dictionary_create(NULL);
    dictionary_set(ctx, "y", int_to_data(6));
    data_t *result = ast_execute(script, data_as_data(ctx));
    EXPECT_EQ(data_type(result), ASTConst);
    EXPECT_EQ(data_intval(data_as_ast_Const(result)->value), expected);
    return result;
  }

  void TearDown() override {
//    parser_free(parser);
//    grammar_free(grammar);
//    grammar_parser_free(gp);
//    file_free(file);
  }

  file_t           *file = NULL;
  grammar_parser_t *gp = NULL;
  grammar_t        *grammar = NULL;
  parser_t         *parser = NULL;
};

