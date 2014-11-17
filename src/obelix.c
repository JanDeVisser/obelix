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
#include <unistd.h>

#include <expr.h>
#include <file.h>
#include <lexer.h>
#include <parser.h>
#include <resolve.h>
#include <stringbuffer.h>

static char *bnf_grammar =
    "%\n"
    "name: simple_expr\n"
    "init: dummy_init \n"
    "ignore_ws: true\n"
    "%\n"
    "program                   := statements ;\n"
    "statements                := statement statements | ;\n"
    "statement                 := 'i' \":=\" expr | \"read\" 'i' | \"write\" expr ;\n"
    "expr [ init: start_expr ] := term termtail ;\n"
    "termtail                  := add_op term termtail | ;\n"
    "term                      := factor factortail ; \n"
    "factortail                := mult_op factor factortail | ;\n"
    "factor                    := ( expr ) | 'i' | 'd' ;\n"
    "add_op                    := + | - ;\n"
    "mult_op                   := * | / ;\n";

parser_t * dummy_init(parser_t *parser) {
  debug("---> init <-----");
  return parser;
}

parser_t * start_expr(parser_t *parser) {
  debug("---> start_expr <-----");
  return parser;
}

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

void test_parser(char *gname, char *tname) {
  file_t *       *file_grammar;
  reader_t       *greader;
  stringbuffer_t *sb_text;
  reader_t       *treader;
  parser_t       *parser;

  if (!gname) {
    greader = (reader_t *) sb_create(bnf_grammar);
  } else {
    greader = (reader_t *) file_open(gname);
  }
  if (!tname) {
    treader = (reader_t *) file_create(fileno(stdin));
  } else {
    treader = (reader_t *) file_open(tname);
  }
  if (greader) {
    parser = parser_read_grammar(greader);
    if (parser) {
      grammar_dump(parser -> grammar);
      parser_parse(parser, treader);
      parser_free(parser);
    }
  }
}

extern void foo(void) {
  puts("foo!");
}

int main(int argc, char **argv) {
  char *grammar;
  int   opt;

  grammar = NULL;
  while ((opt = getopt(argc, argv, "g:")) != -1) {
    switch (opt) {
      case 'g':
        grammar = optarg;
        break;
    }
  }
  test_parser(grammar, (argc > optind) ? argv[optind] : NULL);
}
