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


#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include <file.h>
#include <lexa.h>
#include <nvp.h>
#include <testsuite.h>

lexa_t *lexa;

static lexa_t * setup(void);
static lexa_t * setup_with_scanners(void);
static void     teardown();

static void     _setup_with_scanners(void);
static void     _teardown();

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
  ck_assert_int_eq(lexa -> tokens, 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
END_TEST

START_TEST(test_lexa_newline)
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello  World\nSecond Line"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 9);
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
  ck_assert_int_eq(lexa -> tokens, 16);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeExclPoint), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeAt), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeSlash), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeBackslash), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeAsterisk), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeAmpersand), 2);
END_TEST

/* ----------------------------------------------------------------------- */

START_TEST(test_lexa_qstring)
  lexa_build_lexer(lexa);
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'single quotes' `backticks` \"double quotes\" World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 11);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 4);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeSQuotedStr), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeBQuotedStr), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeDQuotedStr), 1);
END_TEST

START_TEST(test_lexa_qstring_noclose)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'no close quote"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeError), 1);
END_TEST

int ok = 0;

void _escaped_backslashfilter(token_t *token) {
  if (token_code(token) == TokenCodeSQuotedStr) {
    ok = (strcmp(token_token(token), "escaped backslash \\") == 0) ? 1 : -1;
  }
}

START_TEST(test_lexa_qstring_escaped_backslash)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'escaped backslash \\\\'"));
  lexa_set_tokenfilter(lexa, _escaped_backslashfilter);
  lexa_tokenize(lexa);
  ck_assert_int_eq(ok, 1);
  teardown(lexa);
END_TEST

/* ----------------------------------------------------------------------- */

void _setup_comment_lexer(void) {
  lexa = setup_with_scanners();
  lexa_add_scanner(lexa, "comment: marker=/* */,marker=//,marker=^#");
  ck_assert_int_eq(dict_size(lexa -> scanners), 4);
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
}

START_TEST(test_lexa_run_comment_lexer)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello /* comment */ World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_unterminated_comment)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello /* comment"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeError), 1);
END_TEST

START_TEST(test_lexa_asterisk_comment)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello /* comment * comment */ World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_eol_comment)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello // comment * comment */ World\nSecond line"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 8);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

/* ----------------------------------------------------------------------- */

extern void init_suite(void) {
  TCase *tc = tcase_create("Lexa");
  tcase_add_checked_fixture(tc, _setup_with_scanners, _teardown);
  tcase_add_test(tc, test_lexa_build_lexer);
  tcase_add_test(tc, test_lexa_tokenize);
  tcase_add_test(tc, test_lexa_newline);
  tcase_add_test(tc, test_lexa_symbols);
  add_tcase(tc);

  tc = tcase_create("QString");
  tcase_add_checked_fixture(tc, _setup_with_scanners, _teardown);
  tcase_add_test(tc, test_lexa_qstring);
  tcase_add_test(tc, test_lexa_qstring_noclose);
  tcase_add_test(tc, test_lexa_qstring_escaped_backslash);
  add_tcase(tc);

  tc = tcase_create("Comment");
  tcase_add_checked_fixture(tc, _setup_comment_lexer, _teardown);
  tcase_add_test(tc, test_lexa_run_comment_lexer);
  tcase_add_test(tc, test_lexa_unterminated_comment);
  tcase_add_test(tc, test_lexa_asterisk_comment);
  tcase_add_test(tc, test_lexa_eol_comment);
  add_tcase(tc);
}
