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


#include "tlexer.h"
#include <file.h>

lexa_t *lexa;

/* ----------------------------------------------------------------------- */

lexa_t * setup(void) {
  lexa_t *ret;

  ret = lexa_create();
  ck_assert_ptr_ne(ret, NULL);
  return ret;
}

lexa_t * setup_with_scanners(void) {
  lexa_t *lexa;

  lexa = setup();
  lexa_add_scanner(lexa, "identifier");
  lexa_add_scanner(lexa, "whitespace");
  lexa_add_scanner(lexa, "qstring: quotes='`\"");
  return lexa;
}

void teardown(lexa_t *lexa) {
  lexa_free(lexa);
}

void _setup_with_scanners(void) {
  lexa = setup_with_scanners();
}

void _teardown(void) {
  teardown(lexa);
}

/* ----------------------------------------------------------------------- */

START_TEST(test_lexa_build_lexer)
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
END_TEST

START_TEST(test_lexa_tokenize)
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 4);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
END_TEST

START_TEST(test_lexa_newline)
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello  World\nSecond Line"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 8);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 4);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeNewLine), 1);
END_TEST

START_TEST(test_lexa_symbols)
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello !@ /\\ * && World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 15);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeExclPoint), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeAt), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeSlash), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeBackslash), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeAsterisk), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeAmpersand), 2);
END_TEST

START_TEST(test_lexa_ignore_ws)
  scanner_config_t *ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignorews", data_true());
  scanner_config_setvalue(ws_config, "ignorenl", data_false());
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(" Hello  World\nSecond Line \n Third Line "));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 9);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeNewLine), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 0);
END_TEST

START_TEST(test_lexa_ignore_nl)
  scanner_config_t *ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignorews", data_false());
  scanner_config_setvalue(ws_config, "ignorenl", data_true());
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(" Hello  World\nSecond Line \n Third Line "));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 14);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeNewLine), 0);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 7);
END_TEST

START_TEST(test_lexa_ignore_all_ws)
  scanner_config_t *ws_config;

  lexa_build_lexer(lexa);
  ws_config = lexa_get_scanner(lexa, "whitespace");
  scanner_config_setvalue(ws_config, "ignoreall", data_true());
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(" Hello  World\nSecond Line \n Third Line "));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 7);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeNewLine), 0);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 0);
END_TEST

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

void create_lexa(void) {
  TCase *tc = tcase_create("Lexa");
  tcase_add_checked_fixture(tc, _setup_with_scanners, _teardown);
  tcase_add_test(tc, test_lexa_build_lexer);
  tcase_add_test(tc, test_lexa_tokenize);
  tcase_add_test(tc, test_lexa_newline);
  tcase_add_test(tc, test_lexa_symbols);
  tcase_add_test(tc, test_lexa_ignore_ws);
  tcase_add_test(tc, test_lexa_ignore_nl);
  tcase_add_test(tc, test_lexa_ignore_all_ws);
  add_tcase(tc);
}

extern void init_suite(int argc, char **argv) {
  create_lexa();
  create_qstring();
  create_number();
  create_comment();
  create_keyword();
}
