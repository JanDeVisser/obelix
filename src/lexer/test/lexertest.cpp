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

TEST_F(LexerTest, build_lexer) {
  lexa_build_lexer(lexa);
  ASSERT_TRUE(lexa->config);
}

TEST_F(LexerTest, tokenize) {
  lexa_build_lexer(lexa);
  ASSERT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str("Hello World"));
  ASSERT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  ASSERT_EQ(lexa->tokens, 4);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
}

TEST_F(LexerTest, newline) {
  lexa_build_lexer(lexa);
  ASSERT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str("Hello  World\nSecond Line"));
  ASSERT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  ASSERT_EQ(lexa->tokens, 8);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 4);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 1);
}

TEST_F(LexerTest, symbols) {
  lexa_build_lexer(lexa);
  ASSERT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello !@ /\\ * && World"));
  ASSERT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  ASSERT_EQ(lexa->tokens, 15);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 5);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeExclPoint), 1);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeAt), 1);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeSlash), 1);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeBackslash), 1);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeAsterisk), 1);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeAmpersand), 2);
}

TEST_F(LexerTest, ignore_ws) {
  scanner_config_t * ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignorews", data_true());
  scanner_config_setvalue(ws_config, "ignorenl", data_false());
  ASSERT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(" Hello  World\nSecond Line \n Third Line "));
  ASSERT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  ASSERT_EQ(lexa->tokens, 9);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 2);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 0);
}

TEST_F(LexerTest, ignore_nl) {
  scanner_config_t * ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignorews", data_false());
  scanner_config_setvalue(ws_config, "ignorenl", data_true());
  ASSERT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(" Hello  World\nSecond Line \n Third Line "));
  ASSERT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  ASSERT_EQ(lexa->tokens, 14);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 0);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 7);
}

TEST_F(LexerTest, ignore_all_ws) {
  scanner_config_t * ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignoreall", data_true());
  ASSERT_TRUE(lexa->config);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(" Hello  World\nSecond Line \n Third Line "));
  ASSERT_TRUE(lexa->stream);
  lexa_tokenize(lexa);
  ASSERT_EQ(lexa->tokens, 7);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeNewLine), 0);
  ASSERT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 0);
}
