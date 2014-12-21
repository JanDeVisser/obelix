/*
 * /obelix/test/resolve.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include <check.h>
#include "collections.h"
#include <lexer.h>
#include <str.h>

START_TEST(test_lexer_create)
  reader_t *rdr;
  lexer_t  *lexer;

  rdr = (reader_t *) str_wrap("1 + 1");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_free(lexer);
END_TEST

int * _test_lexer_tokenizer(token_t *token, int *count) {
  int   code;
  char *t;
  char  buf[20];

  code = token_code(token);
  t = token_token(token);
  debug("Token #%d: %s", *count, token_tostring(token));
  switch (*count) {
    case 0:
      ck_assert_int_eq(code, TokenCodeInteger);
      ck_assert_str_eq(t, "1");
      break;
    case 1:
      ck_assert_int_eq(code, TokenCodePlus);
      break;
    case 2:
      ck_assert_int_eq(code, TokenCodeIdentifier);
      //ck_assert_str_eq(t, "foo");
      break;
    case 3:
      ck_assert_int_eq(code, TokenCodeEnd);
      break;
    default:
      ck_assert_msg(0, "To many tokens %d -> [%d]", *count, code);
      break;
  }
  *count += 1;
  return count;
}

START_TEST(test_lexer_tokenize)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap("1+foo");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_tokenize(lexer, _test_lexer_tokenizer, &count);
  ck_assert_int_eq(count, 4);
  lexer_free(lexer);
END_TEST


int * _test_lexer_tokenizer_ignore_ws(token_t *token, int *count) {
  int   code;
  char *t;
  char  buf[20];

  code = token_code(token);
  t = token_token(token);
  debug("Token #%d: %s", *count, token_tostring(token));
  ck_assert_int_ne(code, TokenCodeWhitespace);
  *count += 1;
  return count;
}

START_TEST(test_lexer_tokenize_ignore_ws)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap(" 1\tfoo\nbar \t quux");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_ignore_ws, &count);
  lexer_free(lexer);
END_TEST


int * _test_lexer_tokenizer_ignore_nl(token_t *token, int *count) {
  int   code;
  char *t;
  char  buf[20];

  code = token_code(token);
  t = token_token(token);
  debug("Token #%d: %s", *count, token_tostring(token));
  ck_assert_int_ne(code, TokenCodeNewLine);
  *count += 1;
  return count;
}

START_TEST(test_lexer_tokenize_ignore_nl)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap(" 1\tfoo\nbar \t quux");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_set_option(lexer, LexerOptionIgnoreNewLines, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_ignore_nl, &count);
  lexer_free(lexer);
END_TEST


int * _test_lexer_tokenizer_comment(token_t *token, int *count) {
  int   code;
  char *t;
  char  buf[20];

  code = token_code(token);
  t = token_token(token);
  debug("Token #%d: %s", *count, token_tostring(token));
  ck_assert_int_ne(token_token(token), "INCOMMENT");
  *count += 1;
  return count;
}

START_TEST(test_lexer_tokenize_block_comment)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap("1 + 1 - /* INCOMMENT */ 2");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_comment, &count);
  lexer_free(lexer);
END_TEST


START_TEST(test_lexer_tokenize_keyword)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap("foo := 1 + 1 - /* foo := INCOMMENT */ 2");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_add_keyword(lexer, 200, ":=");
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_comment, &count);
  lexer_free(lexer);
END_TEST

START_TEST(test_lexer_tokenize_line_comment)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap("foo := 1 + 1 - // bar := INCOMMENT \n 2");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_add_keyword(lexer, 200, ":=");
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_comment, &count);
  lexer_free(lexer);
END_TEST

static char * pascal_prog =
    "PROGRAM foo;\n"
    "PROCEDURE bar(x: INTEGER);\n"
    "BEGIN\n"
    "  PRINT x;\n"
    "END;\n"
    "BEGIN\n"
    "  bar(3);\n"
    "END";

int * _test_lexer_tokenizer_count_keywords(token_t *token, int *count) {
  int   code;
  char *t;
  char  buf[50];

  code = token_code(token);
  t = token_token(token);
  debug("Token #%d: %s", *count, token_tostring(token));
  if (code >= 200) *count += 1;
  return count;
}

START_TEST(test_lexer_tokenize_keywords)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap(pascal_prog);
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_add_keyword(lexer, 200, ":=");
  lexer_add_keyword(lexer, 201, "PROGRAM");
  lexer_add_keyword(lexer, 202, "PROCEDURE");
  lexer_add_keyword(lexer, 203, "BEGIN");
  lexer_add_keyword(lexer, 204, "END");
  lexer_add_keyword(lexer, 205, "INTEGER");
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_count_keywords, &count);
  ck_assert_int_eq(count, 7);
  lexer_free(lexer);
END_TEST

START_TEST(test_lexer_tokenize_overlapping_keywords)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap("E EL ELS ELSE ELSEE ELSIE");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_add_keyword(lexer, 201, "ELSE");
  lexer_add_keyword(lexer, 202, "ELSIE");
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_count_keywords, &count);
  ck_assert_int_eq(count, 2);
  lexer_free(lexer);
END_TEST

int * _test_lexer_tokenizer_quotedstrings(token_t *token, int *count) {
  int   code;
  char *t;
  char  buf[50];

  code = token_code(token);
  t = token_token(token);
  debug("Token #%d: %s", *count, token_tostring(token));
  if ((code == TokenCodeDQuotedStr) || (code == TokenCodeSQuotedStr)) *count += 1;
  return count;
}

START_TEST(test_lexer_tokenize_quotedstrings)
  reader_t *rdr;
  lexer_t  *lexer;
  int       count;

  rdr = (reader_t *) str_wrap("foo \"double quotes\" + 'single quotes' /* \"INCOMMENT\" */");
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  lexer_add_keyword(lexer, 200, ":=");
  lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
  count = 0;
  lexer_tokenize(lexer, _test_lexer_tokenizer_quotedstrings, &count);
  ck_assert_int_eq(count, 2);
  lexer_free(lexer);
END_TEST


char * get_suite_name() {
  return "Lexer";
}

TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Lexer");

  tcase_add_test(tc, test_lexer_create);
  tcase_add_test(tc, test_lexer_tokenize);
  tcase_add_test(tc, test_lexer_tokenize_ignore_ws);
  tcase_add_test(tc, test_lexer_tokenize_ignore_nl);
  tcase_add_test(tc, test_lexer_tokenize_block_comment);
  tcase_add_test(tc, test_lexer_tokenize_keyword);
  tcase_add_test(tc, test_lexer_tokenize_line_comment);
  tcase_add_test(tc, test_lexer_tokenize_keywords);
  /* tcase_add_test(tc, test_lexer_tokenize_overlapping_keywords); */
  tcase_add_test(tc, test_lexer_tokenize_quotedstrings);
  return tc;
}


