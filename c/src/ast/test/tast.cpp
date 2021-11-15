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

TEST(ASTTest, ConstCreate) {
  ast_Const_t *expr;

  expr = ast_Const_create(int_to_data(2));
  EXPECT_TRUE(expr) ;
  EXPECT_EQ(data_type(expr), ASTConst);
  EXPECT_EQ(data_type(expr->value), Int);
  EXPECT_EQ(data_intval(expr->value), 2);
  debug(tast, "%s", data_tostring(expr));
};

TEST(ASTTest, test_ast_Infix_create) {
  ast_Infix_t *expr;
  ast_Expr_t *left;
  ast_Expr_t *right;
  data_t     *op;

  left = data_as_ast_Expr(ast_Const_create(int_to_data(3)));
  op = (data_t *) str_to_data("+");
  right = data_as_ast_Expr(ast_Const_create(int_to_data(2)));
  expr = ast_Infix_create(left, op, right);
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
}

TEST(ASTTest, test_ast_Prefix_create) {
  ast_Prefix_t *expr;
  ast_Expr_t   *operand;
  data_t       *op;

  op = (data_t *) str_to_data("-");
  operand = data_as_ast_Expr(ast_Const_create(int_to_data(2)));
  expr = ast_Prefix_create(op, operand);
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
}

TEST(ASTTest, test_ast_Ternary_create) {
  ast_Expr_t    *condition;
  ast_Expr_t    *true_value;
  ast_Expr_t    *false_value;
  name_t        *name;
  ast_Ternary_t *expr;

  name = name_create(1, "x");
  condition = (ast_Expr_t *) ast_Infix_create(
    (ast_Expr_t *) ast_Variable_create(name),
    str_to_data("=="),
    (ast_Expr_t  *) ast_Const_create(int_to_data(3)));
  name_free(name);
  true_value = data_as_ast_Expr(ast_Const_create(str_to_data("TRUE")));
  false_value = data_as_ast_Expr(ast_Const_create(str_to_data("FALSE")));
  expr = ast_Ternary_create(condition, true_value, false_value);
  EXPECT_TRUE(expr) ;
  ast_Expr_free(condition);
  ast_Expr_free(true_value);
  ast_Expr_free(false_value);

  debug(tast, "%s", data_tostring(expr));
}

TEST(ASTTest, test_ast_Variable_create) {
  ast_Variable_t *expr;
  name_t         *name;

  name = name_create(1, "x");
  expr = ast_Variable_create(name);
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
}

TEST(ASTTest, test_ast_Const_call) {
  data_t *expr;
  data_t *ret;

  expr = (data_t *) ast_Const_create(int_to_data(2));
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
  ret = ast_execute(expr, data_null());
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(ret), 2);
}

TEST(ASTTest, test_ast_Infix_call) {
  ast_Infix_t *expr;
  ast_Expr_t *left;
  ast_Expr_t *right;
  data_t     *op;
  data_t     *ret;

  left = data_as_ast_Expr(ast_Const_create(int_to_data(3)));
  op = (data_t *) str_to_data("+");
  right = data_as_ast_Expr(ast_Const_create(int_to_data(2)));
  expr = ast_Infix_create(left, op, right);
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
  ret = ast_execute(expr, data_null());
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_EQ(data_type(ret), Int);
  EXPECT_EQ(data_intval(ret), 5);
}

TEST(ASTTest, test_ast_Prefix_call) {
  ast_Prefix_t *expr;
  ast_Expr_t   *operand;
  data_t       *op;
  data_t       *ret;

  op = (data_t *) str_to_data("-");
  operand = data_as_ast_Expr(ast_Const_create(int_to_data(2)));
  expr = ast_Prefix_create(op, operand);
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
  ret = ast_execute(expr, data_null());
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_EQ(data_type(ret), Int);
  EXPECT_EQ(data_intval(ret), -2);
}

TEST(ASTTest, test_ast_Ternary_call) {
  ast_Expr_t    *condition;
  ast_Expr_t    *true_value;
  ast_Expr_t    *false_value;
  name_t        *name;
  ast_Ternary_t *expr;
  dictionary_t  *ctx;
  data_t        *ret;

  name = name_create(1, "x");
  condition = (ast_Expr_t *) ast_Infix_create(
    (ast_Expr_t *) ast_Variable_create(name),
    str_to_data("=="),
    (ast_Expr_t  *) ast_Const_create(int_to_data(3)));
  name_free(name);
  true_value = data_as_ast_Expr(ast_Const_create(str_to_data("TRUE")));
  false_value = data_as_ast_Expr(ast_Const_create(str_to_data("FALSE")));
  expr = ast_Ternary_create(condition, true_value, false_value);
  EXPECT_TRUE(expr) ;
  ast_Expr_free(condition);
  ast_Expr_free(true_value);
  ast_Expr_free(false_value);

  debug(tast, "%s", data_tostring(expr));

  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(3));
  ret = ast_execute(expr, data_as_data(ctx));
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_STREQ(data_tostring(ret), "TRUE");
}

TEST(ASTTest, test_ast_Variable_call) {
  data_t       *expr;
  name_t       *name;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(1, "x");
  expr = (data_t *) ast_Variable_create(name);
  EXPECT_TRUE(expr) ;
  name_free(name);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(2));
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(ret), 2);
}

TEST(ASTTest, test_ast_Variable_doesnt_exist_call) {
  data_t       *expr;
  name_t       *name;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(1, "x");
  expr = (data_t *) ast_Variable_create(name);
  EXPECT_TRUE(expr) ;
  name_free(name);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "y", int_to_data(2));
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_EQ(data_type(ret), Exception);
}

TEST(ASTTest, test_ast_Tree_call) {
  ast_Expr_t   *var;
  ast_Expr_t   *two;
  ast_Expr_t   *three;
  ast_Expr_t   *infix_1;
  ast_Expr_t   *infix_2;

  name_t       *name;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(1, "x");
  var = (ast_Expr_t *) ast_Variable_create(name);
  name_free(name);

  two = (ast_Expr_t *) ast_Const_create(int_to_data(2));
  infix_1 = (ast_Expr_t *) ast_Infix_create(var, (data_t *) str_to_data("*"), two);

  three = (ast_Expr_t *) ast_Const_create(int_to_data(3));
  infix_2 = (ast_Expr_t *) ast_Infix_create(infix_1, (data_t *) str_to_data("+"), three);

  debug(tast, "%s", data_tostring(infix_2));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(4));
  ret = ast_execute(infix_2, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_EQ(data_type(ret), Int);
  EXPECT_EQ(data_intval(ret), 11);
}

TEST(ASTTest, test_ast_Call_call) {
  ast_Call_t   *expr;
  name_t       *name;
  ast_Expr_t   *self;
  ast_Expr_t   *arg;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(2, "x", "sum");
  self = data_as_ast_Expr(ast_Variable_create(name));
  arg = data_as_ast_Expr(ast_Const_create(int_to_data(2)));
  expr = ast_Call_create(self);
  EXPECT_TRUE(expr) ;
  ast_Call_add_argument(expr, arg);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(3));
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(ret), 5);
}

TEST(ASTTest, test_ast_Call_call_parse) {
  ast_Call_t   *expr;
  name_t       *name;
  ast_Expr_t   *self;
  ast_Expr_t   *arg;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(2, "x", "sum");
  self = data_as_ast_Expr(ast_Variable_create(name));
  arg = data_as_ast_Expr(ast_Const_create(int_to_data(2)));
  expr = ast_Call_create(self);
  EXPECT_TRUE(expr) ;
  ast_Call_add_argument(expr, arg);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  ret = ast_parse(expr, (data_t *) ctx);
  EXPECT_EQ(data_type(ret), ASTCall);

  dictionary_set(ctx, "x", int_to_data(3));
  ret = ast_execute(ret, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(ret), 5);
}


TEST(ASTTest, test_ast_Assignment_call) {
  ast_Assignment_t *expr;
  name_t           *name;
  ast_Expr_t       *value;
  dictionary_t     *ctx;
  data_t           *ret;

  value = (ast_Expr_t *) ast_Const_create(int_to_data(2));
  name = name_create(1, "x");
  expr = ast_Assignment_create(name, value);
  EXPECT_TRUE(expr) ;
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "x")), 2);
}

TEST(ASTTest, test_ast_Block_call) {
  ast_Block_t      *expr;
  ast_Assignment_t *ass;
  name_t           *name;
  ast_Expr_t       *value;
  ast_Expr_t       *arg;
  data_t           *ret;
  dictionary_t     *ctx;
  datalist_t       *statements;

  statements = datalist_create(NULL);
  value = (ast_Expr_t *) ast_Const_create(int_to_data(2));
  name = name_create(1, "x");
  ass = ast_Assignment_create(name, value);
  datalist_push(statements, ass);
  ast_Assignment_free(ass);
  value = (ast_Expr_t *) ast_Const_create(int_to_data(3));
  name = name_create(1, "y");
  ass = ast_Assignment_create(name, value);
  datalist_push(statements, ass);
  ast_Assignment_free(ass);
  expr = ast_Block_create();
  debug(tast, "%s", data_tostring(expr));
  expr -> statements = statements;
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "x")), 2);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "y")), 3);
}

TEST(ASTTest, test_ast_Loop_call) {
  ast_Infix_t      *cond;
  ast_Infix_t      *inc;
  ast_Loop_t       *loop;
  ast_Assignment_t *ass_x;
  ast_Assignment_t *ass_y;
  ast_Block_t      *block;
  datalist_t       *statements;
  name_t           *x;
  name_t           *y;
  ast_Expr_t       *value;
  ast_Expr_t       *arg;
  data_t           *ret;
  dictionary_t     *ctx;

  x = name_create(1, "x");
  y = name_create(1, "y");
  cond = ast_Infix_create((ast_Expr_t *) ast_Variable_create(x),
                          (data_t *) str_copy_chars("<"),
                          (ast_Expr_t *) ast_Const_create(int_to_data(10)));

  block = ast_Block_create();
  statements = datalist_create(NULL);
  ass_y = ast_Assignment_create(y, (ast_Expr_t *) ast_Variable_create(x));
  datalist_push(statements, ass_y);
  inc = ast_Infix_create((ast_Expr_t *) ast_Variable_create(x),
                          (data_t *) str_copy_chars("+"),
                          (ast_Expr_t *) ast_Const_create(int_to_data(1)));
  ass_x = ast_Assignment_create(x, (ast_Expr_t *) inc);
  datalist_push(statements, ass_x);
  block -> statements = statements;
  loop = ast_Loop_create((ast_Expr_t *) cond, (ast_Expr_t *) block);
  debug(tast, "%s", data_tostring(loop));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(0));
  ret = ast_execute(loop, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "x")), 10);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "y")), 9);
}

TEST(ASTTest, test_ast_Generator_loop) {
  ast_Generator_t  *gen;
  ast_Infix_t      *cond;
  ast_Infix_t      *inc;
  ast_Loop_t       *loop;
  ast_Assignment_t *ass_x;
  ast_Assignment_t *ass_y;
  name_t           *x;
  name_t           *y;
  data_t           *ret;
  dictionary_t     *ctx;

  x = name_create(1, "x");
  y = name_create(1, "y");

  gen = ast_Generator_create(range_create(int_to_data(0), int_to_data(10)));
  ass_x = ast_Assignment_create(x, (ast_Expr_t *) gen);

  ass_y = ast_Assignment_create(y, (ast_Expr_t *) ast_Variable_create(x));
  loop = ast_Loop_create((ast_Expr_t *) ass_x, (ast_Expr_t *) ass_y);
  debug(tast, "%s", data_tostring(loop));
  ctx = dictionary_create(NULL);
  ret = ast_execute(loop, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  EXPECT_NE(data_type(ret), Exception);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "x")), 9);
  EXPECT_EQ(data_intval(dictionary_get(ctx, "y")), 9);
}

/* ----------------------------------------------------------------------- */

class ASTParserTest : public ::testing::Test {
protected:
  void SetUp() override {
    const char *grammar_path = "/home/jan/projects/obelix/src/ast/test/ast.grammar";

    logging_set_level("DEBUG");
    logging_enable("ast");
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
    EXPECT_NE(result, nullptr) ;
    EXPECT_EQ(data_type(result), Int);
    EXPECT_EQ(expected, data_intval(result));
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

extern "C" {

extern parser_t *make_prefix(parser_t *parser) {
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

extern parser_t *make_infix(parser_t *parser) {
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

extern parser_t *make_const(parser_t *parser) {
  data_t *number = token_todata(parser->last_token);
  ast_Const_t *expr = ast_Const_create(number);

  return parser_pushval(parser, (data_t *) expr);
}

extern parser_t *make_variable(parser_t *parser) {
  data_t *name = token_todata(parser->last_token);
  name_t *n = name_create(1, data_tostring(name));
  ast_Variable_t *expr = ast_Variable_create(n);

  data_free(name);
  name_free(n);
  return parser_pushval(parser, (data_t *) expr);
}

extern parser_t *assign_result(parser_t *parser) {
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
  evaluate("1+1", 2);
}

TEST_F(ASTParserTest, parser_stack_order) {
  evaluate("1 - 2", -1);
}

TEST_F(ASTParserTest, parser_parens) {
  evaluate("2 * (3 + 4)", 14);
}

TEST_F(ASTParserTest, parser_signed_number) {
  evaluate("1 - -2", 3);
}

TEST_F(ASTParserTest, parser_two_pairs_of_parens) {
  evaluate("(1+2) * (3 + 4)", 21);
}

TEST_F(ASTParserTest, parser_nested_parens) {
  evaluate("2 * ((3*2) + 4)", 20);
}

TEST_F(ASTParserTest, parser_precedence) {
  evaluate("2 * (4 + 3*2)", 20);
}

TEST_F(ASTParserTest, parser_variable) {
  evaluate("2 * (4 + 3*x)", 80);
}

TEST_F(ASTParserTest, parser_variable_does_not_exist) {
  parse("2 * (4 + 3*y)");
  data_t *result = data_as_data(parser->data);
  EXPECT_TRUE(result) ;
  EXPECT_TRUE(data_is_ast_Node(result));
}

TEST_F(ASTParserTest, parser_variable_execute) {
  execute("2 * (4*2 + 3*y)", 52);
}

/* ----------------------------------------------------------------------- */

