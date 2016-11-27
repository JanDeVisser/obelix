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

#include "tnet.h"

/* ----------------------------------------------------------------------- */

static uri_t *uri = NULL;

static void _create_uri(char *str) {
  uri = uri_create(str);
  ck_assert(uri);
  if (uri -> error) {
    printf("uri -> error: %s\n", data_tostring(uri -> error));
  }
  ck_assert_ptr_eq(uri -> error, NULL);
}

static void _teardown(void) {
  uri_free(uri);
}

/* ----------------------------------------------------------------------- */

START_TEST(test_uri_create)
  _create_uri("http://www.google.com");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "www.google.com");
END_TEST

START_TEST(test_uri_create_ipv4)
  _create_uri("http://192.168.0.1");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "192.168.0.1");
END_TEST

START_TEST(test_uri_create_localhost)
  _create_uri("http://localhost");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "localhost");
END_TEST

START_TEST(test_uri_create_localhost_8080)
  _create_uri("http://localhost:8080");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "localhost");
  ck_assert_int_eq(uri -> port, 8080);
END_TEST

START_TEST(test_uri_auth)
  _create_uri("http://user:password@www.google.com");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "www.google.com");
  ck_assert_str_eq(uri -> user, "user");
  ck_assert_str_eq(uri -> password, "password");
END_TEST

START_TEST(test_uri_path)
  _create_uri("http://www.google.com/path1/path2/path3");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "www.google.com");
  ck_assert(uri -> path);
  ck_assert_str_eq(name_tostring_sep(uri -> path, "/"), "path1/path2/path3");
END_TEST

START_TEST(test_uri_trailing_slash)
  _create_uri("http://www.google.com/");
  ck_assert_str_eq(uri -> scheme, "http");
  ck_assert_str_eq(uri -> host, "www.google.com");
END_TEST

START_TEST(test_uri_relative_file)
  _create_uri("file:some/path");
  ck_assert_str_eq(uri -> scheme, "file");
  ck_assert_str_eq(name_tostring_sep(uri -> path, "/"), "some/path");
END_TEST

START_TEST(test_uri_absolute_file)
  _create_uri("file:/some/path");
  ck_assert_str_eq(uri -> scheme, "file");
  ck_assert_str_eq(name_tostring_sep(uri -> path, "/"), "//some/path");
END_TEST

START_TEST(test_uri_query)
  _create_uri("http://localhost/some/url?param1=value1&param2=value2");
  ck_assert_str_eq((char *) dict_get(uri -> query, "param1"), "value1");
  ck_assert_str_eq((char *) dict_get(uri -> query, "param2"), "value2");
END_TEST

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

void create_uri(void) {
  TCase *tc = tcase_create("URI");
  tcase_add_checked_fixture(tc, NULL, _teardown);
  tcase_add_test(tc, test_uri_create);
  tcase_add_test(tc, test_uri_create_ipv4);
  tcase_add_test(tc, test_uri_create_localhost);
  tcase_add_test(tc, test_uri_create_localhost_8080);
  tcase_add_test(tc, test_uri_auth);
  tcase_add_test(tc, test_uri_path);
  tcase_add_test(tc, test_uri_trailing_slash);
  tcase_add_test(tc, test_uri_relative_file);
  tcase_add_test(tc, test_uri_absolute_file);
  tcase_add_test(tc, test_uri_query);
  add_tcase(tc);
}

extern void init_suite(int argc, char **argv) {
  create_uri();
}
