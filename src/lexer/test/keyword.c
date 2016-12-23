/*
 * obelix/src/lexer/test/keyword.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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
#include <nvp.h>

static int _prepare_with_big(void) {
  scanner_config_t *kw_config;
  token_t          *token;
  int               ret;

  lexa_add_scanner(lexa, "keyword: keyword=Big");
  lexa_build_lexer(lexa);
  kw_config = lexa_get_scanner(lexa, "keyword");
  ck_assert_ptr_ne(kw_config, NULL);
  token = (token_t *) data_get_attribute((data_t *) kw_config, "Big");
  ck_assert_ptr_ne(token, NULL);
  ret = token_code(token);
  token_free(token);
  return ret;
}

static void _prepare_with_big_bad(int *big, int *bad) {
  scanner_config_t *kw_config;
  token_t          *token;

  lexa_add_scanner(lexa, "keyword: keyword=Big;keyword=Bad");
  lexa_build_lexer(lexa);
  kw_config = lexa_get_scanner(lexa, "keyword");
  ck_assert_ptr_ne(kw_config, NULL);
  token = (token_t *) data_get_attribute((data_t *) kw_config, "Big");
  ck_assert_ptr_ne(token, NULL);
  *big = token_code(token);
  token_free(token);
  token = (token_t *) data_get_attribute((data_t *) kw_config, "Bad");
  ck_assert_ptr_ne(token, NULL);
  *bad = token_code(token);
  token_free(token);
}

static int _prepare_with_abc(void) {
  scanner_config_t *kw_config;
  token_t          *token;
  int               ret;
  str_t            *kw;
  int               ix;
  int_t            *size;
  static char      *_keywords[] = {
    "abb",
    "aca",
    "aba",
    "aaa",
    "aab",
    "abc",
    "aac",
    "acc",
    "acb",
    NULL
  };

  lexa_add_scanner(lexa, "keyword");
  lexa_build_lexer(lexa);
  kw_config = lexa_get_scanner(lexa, "keyword");
  ck_assert_ptr_ne(kw_config, NULL);

  for (ix = 0; _keywords[ix]; ix++) {
    kw = str_wrap(_keywords[ix]);
    scanner_config_setvalue(kw_config, "keyword", (data_t *) kw);
    str_free(kw);
  }

  token = (token_t *) data_get_attribute((data_t *) kw_config, "abc");
  ck_assert_ptr_ne(token, NULL);
  ret = token_code(token);
  token_free(token);
  ck_assert(ret);

  size = (int_t *) data_get_attribute((data_t *) kw_config, "num_keywords");
  ck_assert_int_eq(data_intval((data_t *) size), ix);

  return ret;
}

static void _tokenize(char *str, int total_count, int big_count) {
  int code;

  code = _prepare_with_big();
  lexa_set_stream(lexa, (data_t *) str_copy_chars(str));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, total_count);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, code), big_count);
}

static void _tokenize_big_bad(char *str, int total_count, int big_count, int bad_count) {
  int big;
  int bad;

  _prepare_with_big_bad(&big, &bad);
  lexa_set_stream(lexa, (data_t *) str_copy_chars(str));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, total_count);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, big), big_count);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, bad), bad_count);
}

START_TEST(test_lexa_keyword)
  _tokenize("Big", 2, 1);
END_TEST

START_TEST(test_lexa_keyword_space)
  _tokenize("Big ", 3, 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
END_TEST

START_TEST(test_lexa_keyword_is_prefix)
  _tokenize("Bigger", 2, 0);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 1);
END_TEST

START_TEST(test_lexa_keyword_and_identifiers)
  _tokenize("Hello Big World", 6, 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_keyword_two_keywords)
  _tokenize("Hello Big Big Beautiful World", 10, 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 4);
END_TEST

START_TEST(test_lexa_keyword_two_keywords_separated)
  _tokenize("Hello Big Beautiful Big World", 10, 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 4);
END_TEST

START_TEST(test_lexa_keyword_big_bad_big)
  _tokenize_big_bad("Hello Big World", 6, 1, 0);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_keyword_big_bad_bad)
  _tokenize_big_bad("Hello Bad World", 6, 0, 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

START_TEST(test_lexa_keyword_big_bad_big_bad)
  _tokenize_big_bad("Hello Big Bad World", 8, 1, 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 3);
END_TEST

START_TEST(test_lexa_keyword_big_bad_bad_big)
  _tokenize_big_bad("Hello Bad Big World", 8, 1, 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 3);
END_TEST

START_TEST(test_lexa_keyword_abc)
  int abc = _prepare_with_abc();

  lexa_set_stream(lexa, (data_t *) str_copy_chars("yyz abc ams"));
  lexa_tokenize(lexa);
  ck_assert_int_eq(lexa -> tokens, 6);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, abc), 1);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  ck_assert_int_eq(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
END_TEST

void create_keyword(void) {
  TCase *tc = tcase_create("Keyword");
  tcase_add_checked_fixture(tc, _setup_with_scanners, _teardown);
  tcase_add_test(tc, test_lexa_keyword);
  tcase_add_test(tc, test_lexa_keyword_space);
  tcase_add_test(tc, test_lexa_keyword_is_prefix);
  tcase_add_test(tc, test_lexa_keyword_and_identifiers);
  tcase_add_test(tc, test_lexa_keyword_two_keywords);
  tcase_add_test(tc, test_lexa_keyword_two_keywords_separated);
  tcase_add_test(tc, test_lexa_keyword_big_bad_big);
  tcase_add_test(tc, test_lexa_keyword_big_bad_bad);
  tcase_add_test(tc, test_lexa_keyword_big_bad_big_bad);
  tcase_add_test(tc, test_lexa_keyword_big_bad_bad_big);
  tcase_add_test(tc, test_lexa_keyword_abc);
  add_tcase(tc);
}
