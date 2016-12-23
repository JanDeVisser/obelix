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

void _setup_comment_lexer(void) {
  lexa = setup_with_scanners();
  lexa_add_scanner(lexa, "comment: marker=/* */;marker=//;marker=^#");
  ck_assert_int_eq(dict_size(lexa -> scanners), 4);
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
}

START_TEST(test_lexa_run_comment_lexer)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("BeforeComment /* comment */ AfterComment"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_unterminated_comment)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("UnterminatedComment /* comment"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeError), 1);
END_TEST

START_TEST(test_lexa_asterisk_comment)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("BeforeCommentWithAsterisk /* comment * comment */ AfterComment"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_eol_comment)
  lexa_set_stream(lexa, (data_t *) str_copy_chars(
    "BeforeLineEndComment // comment * comment */ World\n"
    "LineAfterLineEndComment"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
END_TEST

/* ----------------------------------------------------------------------- */

void create_comment(void) {
  TCase *tc = tcase_create("Comment");
  tcase_add_checked_fixture(tc, _setup_comment_lexer, _teardown);
  tcase_add_test(tc, test_lexa_run_comment_lexer);
  tcase_add_test(tc, test_lexa_unterminated_comment);
  tcase_add_test(tc, test_lexa_asterisk_comment);
  tcase_add_test(tc, test_lexa_eol_comment);
  add_tcase(tc);
}
