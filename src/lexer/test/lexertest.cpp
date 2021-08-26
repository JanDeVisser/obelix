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


#include "lexertest.h"
#include <file.h>

/* ----------------------------------------------------------------------- */

TEST_F(LexerTest, BuildLexer) {
  lexa_build_lexer(lexa);
  EXPECT_TRUE(lexa->config);
}

TEST_F(LexerTest, Tokenize) {
  lexa_build_lexer(lexa);
  EXPECT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str("Hello World"));
  EXPECT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa->tokens, 4);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
}

TEST_F(LexerTest, Newline) {
  lexa_build_lexer(lexa);
  EXPECT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str("Hello  World\nSecond Line"));
  EXPECT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa->tokens, 8);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 4);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 1);
}

TEST_F(LexerTest, Symbols) {
  lexa_build_lexer(lexa);
  EXPECT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str("Hello !@ /\\ * && World"));
  EXPECT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa->tokens, 15);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 5);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeExclPoint), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeAt), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeSlash), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeBackslash), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeAsterisk), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeAmpersand), 2);
}

TEST_F(LexerTest, IgnoreWS) {
  scanner_config_t * ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignorews", data_true());
  scanner_config_setvalue(ws_config, "ignorenl", data_false());
  EXPECT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str(" Hello  World\nSecond Line \n Third Line "));
  EXPECT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa->tokens, 9);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 0);
}

TEST_F(LexerTest, IgnoreNL) {
  scanner_config_t * ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignorews", data_false());
  scanner_config_setvalue(ws_config, "ignorenl", data_true());
  EXPECT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str(" Hello  World\nSecond Line \n Third Line "));
  EXPECT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa->tokens, 14);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 0);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 7);
}

TEST_F(LexerTest, IgnoreAllWS) {
  scanner_config_t * ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignoreall", data_true());
  EXPECT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str(" Hello  World\nSecond Line \n Third Line "));
  EXPECT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa->tokens, 7);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 0);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 0);
}
