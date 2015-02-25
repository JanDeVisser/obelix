/*
 * src/bin/test/scripts.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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
#include <loader.h>
#include <logging.h>
#include <name.h>
#include <namespace.h>
#include <script.h>
#include <testsuite.h>

#include "exception.h"

static void     _init_scripts(void) __attribute__((constructor(101)));
static void     _init_scripts_cases(void) __attribute__((constructor(300)));
static data_t * _run_script(char *);

#define SYSPATH    "../../../share/"
#define USERPATH   "./"

data_t * _run_script(char *script) {
  scriptloader_t *loader;
  name_t         *path;
  name_t         *name;
  data_t         *data;
  data_t         *ret;
  object_t       *obj;
  module_t       *mod;
  
  name = name_create(1, script);
  path = name_create(1, USERPATH);
  loader = scriptloader_create(SYSPATH, path, NULL);
  data = scriptloader_import(loader, name);
  if (data_is_module(data)) {
    mod = mod_copy(data_moduleval(data));
    data_free(data);
    obj = object_copy(mod_get(mod));
    ret = obj -> retval;
    object_free(obj);
    mod_free(mod);
  } else {
    ret = data;
  }
  scriptloader_free(loader);
  name_free(path);
  name_free(name);
  return ret;
}

START_TEST(t1)
  data_t *d;

  d = _run_script("t1");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 0);
  data_free(d);
END_TEST

START_TEST(t2)
  data_t  *d;
  error_t *e;

  d = _run_script("t2");
  ck_assert_int_eq(data_type(d), Error);
  e = data_errorval(d);
  ck_assert_int_eq(e -> code, ErrorName);
  data_free(d);
END_TEST

START_TEST(t3)
  data_t *d;

  d = _run_script("t3");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 2);
  data_free(d);
END_TEST

START_TEST(t4)
  data_t *d;

  d = _run_script("t4");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 1);
  data_free(d);
END_TEST

START_TEST(t5)
  data_t *d;

  d = _run_script("t5");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), -1);
  data_free(d);
END_TEST

START_TEST(t6)
  data_t *d;

  d = _run_script("t6");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 10);
  data_free(d);
END_TEST

void _init_scripts(void) {
  set_suite_name("Scripts");
}

void _init_scripts_cases(void) {
  TCase *tc = tcase_create("Scripts");

  tcase_add_test(tc, t1);
  tcase_add_test(tc, t2);
  tcase_add_test(tc, t3);
  tcase_add_test(tc, t4);
  tcase_add_test(tc, t5);
  tcase_add_test(tc, t6);
  add_tcase(tc);
}
