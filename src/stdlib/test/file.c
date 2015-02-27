/*
 * src/stdlib/test/file.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <data.h>
#include <exception.h>
#include <loader.h>
#include <logging.h>
#include <name.h>
#include <namespace.h>
#include <script.h>
#include <testsuite.h>

extern int File;
static void     _init_file(void) __attribute__((constructor(300)));
static data_t * _file_open(char *);
static data_t * _file_readline(data_t *);
static data_t * _file_close(data_t *);

data_t * _file_open(char *name) {
  data_t      *dummy = data_create(Bool, 0);
  array_t     *args = data_array_create(1);
  data_t      *f;

  array_push(args, data_create(String, "file.txt"));
  f = data_execute(dummy, "open", args, NULL);
  array_free(args);
  data_free(dummy);
  return f;
}

data_t * _file_readline(data_t *f) {
  return data_execute(f, "readline", NULL, NULL);
}

data_t * _file_close(data_t *f) {
  data_t *ret;
  ret = data_execute(f, "close", NULL, NULL);
  data_free(f);
  return ret;
}

START_TEST(file_registered)
  typedescr_t *type_file;

  type_file = typedescr_get_byname("file");
  ck_assert_ptr_ne(type_file, NULL);
  ck_assert_str_eq(type_file -> type_name, "file");
END_TEST

START_TEST(file_open_close)
  typedescr_t *type_file = typedescr_get_byname("file");
  data_t      *f;
  data_t      *ret;

  ck_assert_int_ne(File, -1);
  ck_assert_ptr_ne(type_file, NULL);
  f = _file_open("file.txt");
  ck_assert_ptr_ne(f, NULL);
  ck_assert_int_eq(data_type(f), type_file -> type);

  ret = _file_close(f);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Bool);
  ck_assert_int_ne(data_intval(ret), 0);
END_TEST

START_TEST(file_readline)
  data_t *f;
  int     ix;
  data_t *d;

  f = _file_open("file.txt");
  for (d = _file_readline(f); 
       data_type(d) == String; 
       d = _file_readline(f)) {
    debug("%s", data_charval(d));
    ix++;
    data_free(d);
  }
  data_free(d);
  d = _file_close(f);
  data_free(d);
  ck_assert_int_eq(ix, 3);
END_TEST

START_TEST(file_script)
  data_t *d;

  d = run_script("file");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 0);
  data_free(d);
END_TEST


void _init_file(void) {
  TCase *tc = tcase_create("File");

  tcase_add_test(tc, file_registered);
  tcase_add_test(tc, file_open_close);
  tcase_add_test(tc, file_readline);
  tcase_add_test(tc, file_script);
  add_tcase(tc);
}
