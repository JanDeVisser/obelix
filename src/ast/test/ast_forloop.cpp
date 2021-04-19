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

class ASTForLoop : public ASTFixture {
protected:
  const char * get_grammar() const {
    return "/home/jan/projects/obelix/src/ast/test/forloop.grammar";
  }
};

/* ----------------------------------------------------------------------- */

TEST_F(ASTForLoop, parser_list) {
  evaluate("x = [ 1, 2 ] end (42)", 42);
}

TEST_F(ASTForLoop, parser_for_loop) {
  evaluate("x = [1,2,3] s = 0 for y in x s=s+y end (s)", 6);
}

/* ----------------------------------------------------------------------- */

