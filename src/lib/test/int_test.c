/*
 * /obelix/test/tdata.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <testsuite.h>

#include "types.h"

void _init_int_test(void) __attribute__((constructor(300)));

START_TEST(data_int)
  data_t  *d1 = int_to_data(1);
  data_t  *d2 = int_to_data(1);
  data_t  *sum;
  array_t *args;

  ck_assert_int_eq(d1 -> intval, 1);
  ck_assert_int_eq(d2 -> intval, 1);

  args = data_array_create(1);
  array_push(args, d2);
  ck_assert_int_eq(array_size(args), 1);
  
  sum = data_execute(d1, "+", args, NULL);
  ck_assert_int_eq(sum -> type, Int);
  ck_assert_int_eq(sum -> intval, 2);
  data_free(sum);

  array_clear(args);
  d2 = int_to_data(1);
  array_push(args, data_copy(d1));
  array_push(args, d2);
  array_push(args, data_copy(d2));
  sum = data_execute(NULL, "+", args, NULL);
  ck_assert_int_eq(sum -> type, Int);
  ck_assert_int_eq(sum -> intval, 3);

  array_free(args);
  data_free(d1);
  data_free(sum);
  ck_assert_int_eq(data_count(), 0);
END_TEST

START_TEST(int_parse)
  data_t *d;

  d = data_parse(Int, "42");
  ck_assert_ptr_ne(d, NULL);
  ck_assert_int_eq(d -> type, Int);
  ck_assert_int_eq(d -> intval, 42);
  data_free(d);

  d = data_parse(Int, "abc");
  ck_assert_ptr_eq(d, NULL);

  /* We don't parse & round decimals. Is that right? */
  d = data_parse(Int, "3.14");
  ck_assert_ptr_eq(d, NULL);

END_TEST

START_TEST(int_cmp)
  data_t *i1 = int_to_data(1);
  data_t *i2 = int_to_data(2);
  data_t *f1 = data_create(Float, 3.14);
  data_t *b1 = data_create(Bool, FALSE);
  data_t *ret;
  int cmp;
  
  cmp = data_cmp(i1, i2);
  ck_assert_int_lt(cmp, 0);
  cmp = data_cmp(i1, f1);
  ck_assert_int_lt(cmp, 0);
  cmp = data_cmp(i1, b1);
  ck_assert_int_gt(cmp, 0);
  cmp = data_cmp(f1, b1);
  ck_assert_int_gt(cmp, 0);
  
  ret = execute(f1, ">", 1, Bool, FALSE);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Bool);
  ck_assert_int_eq(ret -> intval, TRUE);

  data_free(ret);
  data_free(i1);
  data_free(i2);
  data_free(f1);
  data_free(b1);
END_TEST


void _init_int_test(void) {
  TCase *tc = tcase_create("Int");

  tcase_add_test(tc, data_int);
  tcase_add_test(tc, int_parse);
  tcase_add_test(tc, int_cmp);
  add_tcase(tc);
}
