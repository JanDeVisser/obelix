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

typedef struct _tokenizer_state {
  int           count;
  int           num_codes;
  token_code_t *codes;
} tokenizer_state_t;

typedef lexer_t * (*lexer_config_t)(lexer_t *);

static tokenizer_state_t * _test_lexer_tokenizer(token_t *token, tokenizer_state_t *state) {
  int   code;

  code = token_code(token);
  /* debug("Token #%d: %s", state -> count, token_tostring(token)); */
  ck_assert_int_le(state -> count, state -> num_codes);
  if (state -> count < state -> num_codes) {
    ck_assert_int_eq(code, state -> codes[state -> count]);
  } else {
    ck_assert_int_eq(code, TokenCodeEnd);
  }
  state -> count += 1;
  return state;
}

static lexer_t * _test_lexer_config_ignore_ws(lexer_t *lexer) {
  return lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
}

static lexer_t * _test_lexer_config_ignore_nl(lexer_t *lexer) {
  return lexer_set_option(lexer, LexerOptionIgnoreNewLines, 1);
}

static void _test_lexer(char *text, int num_codes, token_code_t *codes,
                        lexer_config_t config) {
  reader_t          *rdr;
  lexer_t           *lexer;
  tokenizer_state_t  state;

  state.count = 0;
  state.num_codes = num_codes;
  state.codes = codes;

  rdr = (reader_t *) str_wrap(text);
  lexer = lexer_create(rdr);
  ck_assert(lexer != NULL);
  if (config) {
    lexer = config(lexer);
  }
  lexer_tokenize(lexer, _test_lexer_tokenizer, &state);
  ck_assert_int_eq(state.count, num_codes + 1);
  lexer_free(lexer);
}

static token_code_t test_lexer_tokenize_codes[] = {
    TokenCodeInteger, TokenCodePlus, TokenCodeIdentifier
};

START_TEST(test_lexer_tokenize)
  _test_lexer("1+foo", 3, test_lexer_tokenize_codes, NULL);
END_TEST


static token_code_t test_lexer_tokenize_ignore_ws_codes[] = {
  TokenCodeInteger, TokenCodeIdentifier, TokenCodeIdentifier, TokenCodeIdentifier
};

START_TEST(test_lexer_tokenize_ignore_ws)
  _test_lexer(" 1\tfoo\nbar \t quux", 4, test_lexer_tokenize_ignore_ws_codes, _test_lexer_config_ignore_ws);
END_TEST

static token_code_t test_lexer_tokenize_ignore_nl_codes[] = {
  TokenCodeWhitespace, TokenCodeInteger, TokenCodeWhitespace,
  TokenCodeIdentifier, TokenCodeWhitespace, TokenCodeIdentifier,
  TokenCodeWhitespace, TokenCodeIdentifier
};

START_TEST(test_lexer_tokenize_ignore_nl)
  _test_lexer(" 1\tfoo\nbar \t quux", 8, test_lexer_tokenize_ignore_nl_codes, _test_lexer_config_ignore_nl);
END_TEST

static token_code_t test_lexer_tokenize_block_comment_codes[] = {
  TokenCodeInteger, TokenCodePlus, TokenCodeInteger, TokenCodeMinus, TokenCodeInteger
};

START_TEST(test_lexer_tokenize_block_comment)
  _test_lexer("1 + 1 - /* INCOMMENT */ 2", 5, test_lexer_tokenize_block_comment_codes, _test_lexer_config_ignore_ws);
END_TEST

static lexer_t * _test_lexer_config_keyword(lexer_t *lexer) {
  lexer_add_keyword(lexer, 200, ":=");
  return lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
}

static token_code_t test_lexer_tokenize_keyword_codes[] = {
  TokenCodeIdentifier, 200, TokenCodeInteger, TokenCodePlus, TokenCodeInteger,
  TokenCodeMinus, TokenCodeInteger
};

START_TEST(test_lexer_tokenize_keyword)
  _test_lexer("foo := 1 + 1 - /* foo := INCOMMENT */ 2",
              7, test_lexer_tokenize_keyword_codes, _test_lexer_config_keyword);
END_TEST

START_TEST(test_lexer_tokenize_line_comment)
  _test_lexer("foo := 1 + 1 - // bar := INCOMMENT \n 2",
              7, test_lexer_tokenize_keyword_codes, _test_lexer_config_keyword);
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

static token_code_t test_lexer_tokenize_keywords_codes[] = {
  201, TokenCodeIdentifier, TokenCodeSemiColon,
  202, TokenCodeIdentifier, TokenCodeOpenPar, TokenCodeIdentifier,
    TokenCodeColon, 205, TokenCodeClosePar, TokenCodeSemiColon,
  203, TokenCodeIdentifier, TokenCodeIdentifier, TokenCodeSemiColon,
  204, TokenCodeSemiColon, 203,
  TokenCodeIdentifier, TokenCodeOpenPar, TokenCodeInteger,
    TokenCodeClosePar, TokenCodeSemiColon, 204
};

static lexer_t * _test_lexer_config_keywords(lexer_t *lexer) {
  lexer_add_keyword(lexer, 200, ":=");
  lexer_add_keyword(lexer, 201, "PROGRAM");
  lexer_add_keyword(lexer, 202, "PROCEDURE");
  lexer_add_keyword(lexer, 203, "BEGIN");
  lexer_add_keyword(lexer, 204, "END");
  lexer_add_keyword(lexer, 205, "INTEGER");
  return lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
}

START_TEST(test_lexer_tokenize_keywords)
  _test_lexer(pascal_prog,
              24, test_lexer_tokenize_keywords_codes, _test_lexer_config_keywords);
END_TEST

static token_code_t test_lexer_tokenize_overlapping_keywords_codes[] = {
  TokenCodeIdentifier, TokenCodeIdentifier, TokenCodeIdentifier,
  201, 201, TokenCodeIdentifier, 202
};

static lexer_t * _test_lexer_config_overlapping_keywords(lexer_t *lexer) {
  lexer_add_keyword(lexer, 201, "ELSE");
  lexer_add_keyword(lexer, 202, "ELSIE");
  return lexer_set_option(lexer, LexerOptionIgnoreWhitespace, 1);
}

START_TEST(test_lexer_tokenize_overlapping_keywords)
  _test_lexer("E EL ELS ELSE ELSEE ELSIE",
              7, test_lexer_tokenize_overlapping_keywords_codes,
              _test_lexer_config_overlapping_keywords);
END_TEST

static token_code_t test_lexer_tokenize_quotedstrings_codes[] = {
  TokenCodeIdentifier, TokenCodeDQuotedStr, TokenCodePlus,
  TokenCodeSQuotedStr
};

START_TEST(test_lexer_tokenize_quotedstrings)
  _test_lexer("foo \"double quotes\" + 'single quotes' /* \"INCOMMENT\" */",
              4, test_lexer_tokenize_quotedstrings_codes, _test_lexer_config_keyword);
END_TEST

static char * test_numbers_str =
    "1 3.14 0xDEADBEEF -3 -2.72 3.43e13 -23.2e-12 01 01.2 0.3 0.3e+12 -0xFE";

static token_code_t test_numbers_codes[] = {
  TokenCodeInteger, TokenCodeFloat, TokenCodeHexNumber,
  TokenCodeInteger, TokenCodeFloat, TokenCodeFloat,
  TokenCodeFloat, /* TokenCodeHexNumber, */ TokenCodeInteger,
  TokenCodeFloat, TokenCodeFloat, TokenCodeFloat, TokenCodeHexNumber
};

START_TEST(test_lexer_tokenize_numbers)
  _test_lexer(test_numbers_str, 12, test_numbers_codes, _test_lexer_config_ignore_ws);
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
  tcase_add_test(tc, test_lexer_tokenize_overlapping_keywords);
  tcase_add_test(tc, test_lexer_tokenize_quotedstrings);
  tcase_add_test(tc, test_lexer_tokenize_numbers);
  return tc;
}


