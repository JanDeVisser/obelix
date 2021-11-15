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

#include "fixture.h"

/* ----------------------------------------------------------------------- */

int tast_debug = 0;

/* ----------------------------------------------------------------------- */

class ASTFlowControl : public ASTFixture {
protected:
  const char * get_grammar() const {
    return "/home/jan/projects/obelix/src/ast/test/basicflow.grammar";
  }
};

/* ----------------------------------------------------------------------- */

TEST_F(ASTFlowControl, parser_create) {
}

TEST_F(ASTFlowControl, parser_parse) {
  execute("(1+1)", 2);
}

TEST_F(ASTFlowControl, parser_assign) {
  evaluate("a = 1 - 2", -1);
}

TEST_F(ASTFlowControl, parser_block) {
  evaluate("a = 1+2   b=3", 3);
}

TEST_F(ASTFlowControl, parser_if) {
  evaluate("a = 1+2 if a b=4 else b=5 end (b)", 4);
}

TEST_F(ASTFlowControl, parser_if_elif) {
  evaluate("x = 0 y = 2 if x b=1 elif y b=2 else b=3 end (b)", 2);
}

TEST_F(ASTFlowControl, parser_while) {
  evaluate("x = 3 while x x = x - 1 end (x)", 0);
}

/* ----------------------------------------------------------------------- */

