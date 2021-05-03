/*
 * obelix/src/lexer/test/number.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

// grep START_TEST number.c | sed -e 's/START_TEST[(]/  tcase_add_test\(tc, /g' -e 's/$/;/g'

#include "lexertest.h"

/* ----------------------------------------------------------------------- */

void _setup_number_lexer(void) {
  lexa = setup_with_scanners();
  lexa_add_scanner(lexa, "number");
  ck_assert_int_eq(dict_size(lexa -> scanners), 4);
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
}

START_TEST(test_lexa_integer)
  lexa_set_stream(lexa, (data_t *) str("Hello 1234 World"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeInteger), 1);
END_TEST

START_TEST(test_lexa_neg_integer)
  lexa_set_stream(lexa, (data_t *) str("Hello -1234 World"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeInteger), 1);
END_TEST

START_TEST(test_lexa_integer_nospace)
  lexa_set_stream(lexa, (data_t *) str("Hello -1234World"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 5);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeInteger), 1);
END_TEST

START_TEST(test_lexa_hex)
  lexa_set_stream(lexa, (data_t *) str("Hello 0x1234abcd World"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeHexNumber), 1);
END_TEST

START_TEST(test_lexa_hex_nothexdigit)
  lexa_set_stream(lexa, (data_t *) str("Hello 0x1234abcj World"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 7);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeHexNumber), 1);
END_TEST

START_TEST(test_lexa_float_unconfigured)
  lexa_set_config_value(lexa, "number", "float=0");
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.12 World"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 8);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeInteger), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeDot), 1);
END_TEST

START_TEST(test_lexa_float)
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56 World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
END_TEST

START_TEST(test_lexa_neg_float)
  lexa_set_stream(lexa, (data_t *) str("Hello -1234.56 World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
END_TEST

START_TEST(test_lexa_sci_float)
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56e+02 World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
END_TEST

START_TEST(test_lexa_sci_float_nosign)
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56e02 World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
END_TEST

START_TEST(test_lexa_sci_float_no_exp)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 1234.56e World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeError), 1);
END_TEST

START_TEST(test_lexa_sci_float_sign_no_exp)
  lexa_set_stream(lexa, (data_t *) str_copy_chars("Hello 1234.56e+ World"));
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeError), 1);
END_TEST

/* ------------------------------------------------------------------------ */

void create_number(void) {
  TCase *tc = tcase_create("Number");
  tcase_add_checked_fixture(tc, _setup_number_lexer, _teardown);
  tcase_add_test(tc, test_lexa_integer);
  tcase_add_test(tc, test_lexa_neg_integer);
  tcase_add_test(tc, test_lexa_integer_nospace);
  tcase_add_test(tc, test_lexa_hex);
  tcase_add_test(tc, test_lexa_hex_nothexdigit);
  tcase_add_test(tc, test_lexa_float_unconfigured);
  tcase_add_test(tc, test_lexa_float);
  tcase_add_test(tc, test_lexa_neg_float);
  tcase_add_test(tc, test_lexa_sci_float);
  tcase_add_test(tc, test_lexa_sci_float_nosign);
  tcase_add_test(tc, test_lexa_sci_float_no_exp);
  tcase_add_test(tc, test_lexa_sci_float_sign_no_exp);
  add_tcase(tc);
}
