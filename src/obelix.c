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

void test_lexer(char *fname) {
  file_t *fh;
  lexer_t *lexer;
  token_t *token;

  fh = (fname) ? file_open(fname) : file_create(fileno(stdin));
  if (fh >= 0) {
    lexer = lexer_create(fh);
    if (lexer) {
      for (token = lexer_next_token(lexer); token; token = lexer_next_token(lexer)) {
        printf("[%d]%s", token_code(token), token_token(token));
        token_free(token);
      }
      printf("\n");
      lexer_free(lexer);
    }
    close(fh);
  }
}

int main(int argc, char **argv) {
  test_lexer((argc > 1) ? argv[1] : NULL);
}
