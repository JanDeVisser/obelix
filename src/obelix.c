/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <stdio.h>

#include <expr.h>
#include <file.h>
#include <lexer.h>
#include <parser.h>
#include <stringbuffer.h>

static char *bnf_grammar =
    "# name=simple_expr init=dummy_init ignore_ws #"
    "program        := expr | $;"
    "expr           := add_expr | terminal_expr;"
    "add_expr       := terminal_expr add_op terminal_expr;"
    "terminal_expr  := atom_expr | mult_expr;"
    "mult_expr      := atom_expr mult_op atom_expr;"
    "atom_expr      := par_expr | number | func_call;"
    "par_expr       := ( expr );"
    "number         := 'd' | - 'd';"
    "mult_op        := * | /;"
    "add_op         := + | -;"
    "func_call      := func_name ( parlist ) ;"
    "func_name      := \"sin\" | \"cos\" | \"tan\" | \"exp\" | \"log\" ;"
    "parlist        := expr | expr, parlist ;";

void test_expr() {
  expr_t *expr, *e;
  data_t *result;
  int res;
  
  //expr = expr_funccall(NULL, _add);
  expr_int_literal(expr, 1);
  expr_int_literal(expr, 2);
  //e = expr_funccall(expr, _mult);
  expr_int_literal(e, 2);
  expr_int_literal(e, 3);
  
  result = expr_evaluate(expr);
  data_get(result, &res);
  printf("result: %d\n", res);
  data_free(result);
  expr_free(expr);
}

void test_parser(char *fname) {
  stringbuffer_t *sb_grammar;
  parser_t       *parser;

  sb_grammar = sb_create(bnf_grammar);
  if (sb_grammar) {
    parser = parser_read_grammar(sb_grammar);
    if (parser) {
      grammar_dump(parser -> grammar);
      parser_free(parser);
    }
    sb_free(sb);
  }
}

int main(int argc, char **argv) {
  test_parser((argc > 1) ? argv[1] : NULL);
}
