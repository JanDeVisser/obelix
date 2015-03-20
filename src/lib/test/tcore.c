/*
 * dict.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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


#include <check.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <core.h>
#include <testsuite.h>

static void _set_suite_name(void) __attribute__((constructor(101)));
static void _init_tcore(void) __attribute__((constructor(300)));

START_TEST(test_strrand)
  char str[11];
  int  ix;
  
  for (ix = 0; ix < 500; ix++) {
    strrand(str, 10);
    ck_assert_int_eq(strlen(str), 10);
  }
END_TEST

START_TEST(test_strhash)
  char str[11];
  int ix;
  double total;
  unsigned int min;
  unsigned int max;
  unsigned int hash;
  
  min = INT_MAX;
  max = 0;
  total = 0;
  for (ix = 0; ix < 500; ix++) {
    strrand(str, 10);
    hash = strhash(str);
    /* debug("ix: %d str: %s hash: %u", ix, str, hash); */
    if (hash < min) min = hash;
    if (hash > max) max = hash;
    total += hash;
  }
  debug("test_strhash --> %u - %f %u", min, total/500.0, max);
END_TEST

static void _set_suite_name(void) {
  set_suite_name("Core Library");
}

static void _init_tcore(void) {
  TCase *tc = tcase_create("Core");

  tcase_add_test(tc, test_strrand);
  tcase_add_test(tc, test_strhash);
  add_tcase(tc);
}
