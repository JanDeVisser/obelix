/*
 * obelix/src/lexer/test/tlexer.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <gtest/gtest.h>

#include <range.h>
#include <file.h>
#include <grammarparser.h>
#include <parser.h>
#include "tast.h"

/* ----------------------------------------------------------------------- */

int tast_debug = 0;

/* ----------------------------------------------------------------------- */

class ASTParserTest : public ::testing::Test {
protected:
  void SetUp() override {
    const char *grammar_path = "/home/jan/projects/obelix/src/ast/test/basicflow.grammar";

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

  void parse(const char *str) {
    str_t *text = str_copy_chars(str);
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

  data_t * evaluate(const char *str, int expected) {
    parse(str);
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
    parser->data = result;
    return result;
  }

  void TearDown() override {
    parser_free(parser);
    grammar_free(grammar);
    grammar_parser_free(gp);
    file_free(file);
  }

  file_t           *file = NULL;
  grammar_parser_t *gp = NULL;
  grammar_t        *grammar = NULL;
  parser_t         *parser = NULL;
};

extern "C" {

__DLL_EXPORT__ parser_t * make_block(parser_t *parser) {
  ast_Block_t *block = ast_Block_create((char *) "main");
  datastack_push(parser->stack, block);
  parser->data = ast_Block_copy(block);
  return parser;
}

__DLL_EXPORT__ parser_t * add_statement(parser_t *parser) {
  ast_Expr_t  *statement = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Block_t *block = data_as_ast_Block(datastack_pop(parser->stack));

  ast_Block_add_statement(block, statement);
  datastack_push(parser->stack, block);
  return parser;
}

__DLL_EXPORT__ parser_t *make_pass(parser_t *parser) {
  ast_Pass_t *pass = ast_Pass_create();

  return parser_pushval(parser, (data_t *) pass);
}

__DLL_EXPORT__ parser_t * make_ternary(parser_t *parser) {
  ast_Expr_t    *condition = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Block_t   *block = ast_Block_create((char *) "if (true)");
  ast_Ternary_t *if_stmt = ast_Ternary_create(condition, data_as_ast_Expr(block), NULL);
  datastack_push(parser->stack, if_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

__DLL_EXPORT__ parser_t * set_false_block(parser_t *parser) {
  ast_Ternary_t *if_stmt = data_as_ast_Ternary(datastack_pop(parser->stack));
  ast_Block_t   *block = ast_Block_create((char *) "if (false)");
  if_stmt->false_value = data_as_ast_Expr(block);

  datastack_push(parser->stack, if_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

__DLL_EXPORT__ parser_t * set_empty_false_block(parser_t *parser) {
  ast_Ternary_t *if_stmt = data_as_ast_Ternary(datastack_pop(parser->stack));
  ast_Block_t   *block = ast_Block_create((char *) "if (false)(empty)");
  if_stmt->false_value = data_as_ast_Expr(block);
  datastack_push(parser->stack, if_stmt);
  return parser;
}

__DLL_EXPORT__ parser_t * make_loop(parser_t *parser) {
  char        *name;
  ast_Expr_t  *condition;
  ast_Block_t *block;
  ast_Loop_t  *while_stmt;

  condition = data_as_ast_Expr(datastack_pop(parser->stack));
  asprintf(&name, "while (%s)", ast_Expr_tostring(condition));
  block = ast_Block_create((char *) "while (%s");
  free(name);
  while_stmt = ast_Loop_create(condition, data_as_ast_Expr(block));
  datastack_push(parser->stack, while_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

__DLL_EXPORT__ parser_t * make_assignment(parser_t *parser) {
  ast_Expr_t       *expr = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Const_t      *name_expr = data_as_ast_Const(datastack_pop(parser->stack));
  data_t           *name = name_expr->value;
  ast_Assignment_t *assign_stmt = ast_Assignment_create(name_create(1, data_tostring(name)), expr);
  datastack_push(parser->stack, assign_stmt);
  return parser;
}

__DLL_EXPORT__ parser_t *make_prefix(parser_t *parser) {
  ast_Expr_t *value;
  token_t *sign;
  ast_Prefix_t *expr;

  value = (ast_Expr_t *) datastack_pop(parser->stack);
  sign = (token_t *) datastack_pop(parser->stack);
  expr = ast_Prefix_create((data_t *) sign, value);
  debug(tast, "expr: %s", ast_Prefix_tostring(expr));
  datastack_push(parser->stack, expr);
  return parser;
}

__DLL_EXPORT__ parser_t *make_infix(parser_t *parser) {
  ast_Expr_t *left;
  ast_Expr_t *right;
  token_t *op;
  ast_Infix_t *expr;

  right = (ast_Expr_t *) datastack_pop(parser->stack);
  op = (token_t *) datastack_pop(parser->stack);
  left = (ast_Expr_t *) datastack_pop(parser->stack);
  expr = ast_Infix_create(left, (data_t *) op, right);
  debug(tast, "expr: %s", ast_Infix_tostring(expr));
  datastack_push(parser->stack, expr);
  return parser;
}

__DLL_EXPORT__ parser_t *make_const(parser_t *parser) {
  data_t *number = token_todata(parser->last_token);
  ast_Const_t *expr = ast_Const_create(number);

  return parser_pushval(parser, (data_t *) expr);
}

__DLL_EXPORT__ parser_t *make_variable(parser_t *parser) {
  data_t *name = token_todata(parser->last_token);
  name_t *n = name_create(1, data_tostring(name));
  ast_Variable_t *expr = ast_Variable_create(n);

  data_free(name);
  name_free(n);
  return parser_pushval(parser, (data_t *) expr);
}

__DLL_EXPORT__ parser_t *assign_result(parser_t *parser) {
  ast_Expr_t *expr = (ast_Expr_t *) datastack_pop(parser->stack);
  dictionary_t *ctx = dictionary_create(NULL);

  dictionary_set(ctx, "x", int_to_data(12));
  data_t *result = ast_execute(expr, data_as_data(ctx));
  parser->data = result;
  return parser;
}

}

/* ----------------------------------------------------------------------- */

TEST_F(ASTParserTest, parser_create) {
}

TEST_F(ASTParserTest, parser_parse) {
  execute("(1+1)", 2);
}

TEST_F(ASTParserTest, parser_assign) {
  evaluate("a = 1 - 2", -1);
}

TEST_F(ASTParserTest, parser_block) {
  evaluate("a = 1+2   b=3", 3);
}

TEST_F(ASTParserTest, parser_if) {
  evaluate("a = 1+2 if a b=4 else b=5 end (b)", 4);
}

/* ----------------------------------------------------------------------- */

