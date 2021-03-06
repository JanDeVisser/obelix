/*
 * /obelix/test/resolve.c - bufright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
 * You should have received a buf of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tcore.h"
#include <resolve.h>

typedef test_t * (*testfactory_t)(char *);
typedef void *   (*helloworld_t)(char *);

#define OBL_DIR_EQ        OBL_DIR "="
#define OBL_DIR_EQ_LEN    (strlen(OBL_DIR_EQ))

#include <stdio.h>
static void set_obldir(char *argv0) {
#ifdef HAVE_PUTENV
  char *ptr;
  char  buf[MAX_PATH + OBL_DIR_EQ_LEN + 1];
  char *path = buf + OBL_DIR_EQ_LEN;

  strcpy(buf, OBL_DIR_EQ);
  strncpy(path, argv0, MAX_PATH);
  path[MAX_PATH] = 0;
  ptr = strrchr(path, '/');
  if (!ptr) {
    ptr = strrchr(path, '\\');
  }
  if (ptr) {
    *ptr = 0;
    putenv(buf);
  }
#endif /* HAVE_PUTENV */
}

START_TEST(test_resolve_get)
  resolve_t *resolve;

  resolve = resolve_get();
  ck_assert_ptr_ne(resolve, NULL);
END_TEST

START_TEST(test_resolve_open)
  resolve_t *resolve;

  resolve = resolve_get();
  ck_assert_ptr_ne(resolve, NULL);
  ck_assert_ptr_ne(resolve_open(resolve, "libtestlib.so"), NULL);
END_TEST

START_TEST(test_resolve_resolve)
  resolve_t     *resolve;
  testfactory_t  tc;
  test_t        *test;

  resolve = resolve_get();
  ck_assert_ptr_ne(resolve, NULL);
  tc = (testfactory_t) resolve_resolve(resolve, "test_factory");
  ck_assert_ptr_ne(tc, NULL);
  test = tc("test");
  ck_assert_str_eq(test -> data, "test");
END_TEST

START_TEST(test_resolve_library)
  ck_assert_int_ne(resolve_library("libtestlib.so"), 0);
END_TEST


START_TEST(test_resolve_function)
  testfactory_t  tc;
  test_t        *test;

  tc = (testfactory_t) resolve_function("test_factory");
  ck_assert_ptr_ne(tc, NULL);
  test = tc("test");
  ck_assert_str_eq(test -> data, "test");
END_TEST

START_TEST(test_resolve_foreign_function)
  helloworld_t  hw;
  void         *test;

  mark_point();
  ck_assert_int_ne(resolve_library("libtestlib.so"), 0);
  hw = (helloworld_t) resolve_function("testlib_helloworld");
  ck_assert_ptr_ne(hw, NULL);
  test = hw("test");
  ck_assert_ptr_ne(test, NULL);
END_TEST

void resolve_init(char *argv0) {
  TCase *tc = tcase_create("Resolve");

  set_obldir(argv0);
  tcase_add_test(tc, test_resolve_get);
  tcase_add_test(tc, test_resolve_open);
  tcase_add_test(tc, test_resolve_resolve);
  tcase_add_test(tc, test_resolve_library);
  tcase_add_test(tc, test_resolve_function);
  tcase_add_test(tc, test_resolve_foreign_function);
  add_tcase(tc);
}
