/*
 * obelix/src/lexer/test/qstring.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

/* ----------------------------------------------------------------------- */

START_TEST(test_lexa_qstring)
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
char *filter;

void _test_filter(token_t *token) {
  if (token_code(token) == TokenCodeSQuotedStr) {
    ok = (strcmp(token_token(token), filter) == 0) ? 1 : -1;
  }
}

START_TEST(test_lexa_qstring_escaped_backslash)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'escaped backslash \\\\'"));
  filter = "escaped backslash \\";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  ck_assert_int_eq(ok, 1);
END_TEST

START_TEST(test_lexa_qstring_escaped_quote)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'escaped quote \\''"));
  filter = "escaped quote '";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  ck_assert_int_eq(ok, 1);
END_TEST

START_TEST(test_lexa_qstring_no_escape)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'escape \\"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeError), 1);
END_TEST

START_TEST(test_lexa_qstring_newline)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'escaped\\nnewline''"));
  filter = "escaped\nnewline";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  ck_assert_int_eq(ok, 1);
END_TEST

START_TEST(test_lexa_qstring_gratuitous_escape)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 'escaped \\$ dollarsign''"));
  filter = "escaped $ dollarsign";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  ck_assert_int_eq(ok, 1);
END_TEST

/* ------------------------------------------------------------------------ */

void create_qstring(void) {
  TCase *tc = tcase_create("QString");
  tcase_add_checked_fixture(tc, _setup_with_scanners, _teardown);
  tcase_add_test(tc, test_lexa_qstring);
  tcase_add_test(tc, test_lexa_qstring_noclose);
  tcase_add_test(tc, test_lexa_qstring_escaped_backslash);
  tcase_add_test(tc, test_lexa_qstring_escaped_quote);
  tcase_add_test(tc, test_lexa_qstring_no_escape);
  tcase_add_test(tc, test_lexa_qstring_newline);
  tcase_add_test(tc, test_lexa_qstring_gratuitous_escape);
  add_tcase(tc);
}

