/*
 * list.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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


#include "tcore.h"
#include <stdio.h>
#include <stdlib.h>

#include <data.h>

static void     setup(void);
static void     teardown(void);

int         type;
datalist_t *list_null;
datalist_t *list_array;
datalist_t *list_valist;

void check_list(datalist_t *l, int sz) {
  ck_assert_ptr_ne(l, NULL);
  ck_assert_int_eq(datalist_size(l), sz);
}

void setup(void) {
  array_t *a = array_create(0);

  list_null = datalist_create(NULL);
  check_list(list_null, 0);

  array_push(a, str_wrap("test"));
  array_push(a, data_true());
  array_push(a, data_null());
  array_push(a, int_to_data(42));
  list_array = datalist_create(a);
  check_list(list_array, array_size(a));

  list_valist = (datalist_t *) data_create(List, 2, str_wrap("test"), int_to_data(42));
  check_list(list_valist, 2);
}

void teardown(void) {
  datalist_free(list_null);
  datalist_free(list_array);
  datalist_free(list_valist);
}

START_TEST(test_datalist_create)
END_TEST

START_TEST(test_datalist_get)
  ck_assert_str_eq(data_tostring(data_uncopy(datalist_get(list_array, 0))), "test");
  ck_assert_int_eq(data_intval(data_uncopy(datalist_get(list_array, 3))), 42);
END_TEST

START_TEST(test_datalist_push)
  datalist_push(list_null, str_wrap("push"));
  check_list(list_null, 1);
  ck_assert_str_eq(data_tostring(data_uncopy(datalist_get(list_null, 0))), "push");
END_TEST

START_TEST(test_datalist_pop)
  data_t *pop;

  pop = datalist_pop(list_valist);
  check_list(list_valist, 1);
  ck_assert_int_eq(_data_intval(pop), 42);
  data_free(pop);
END_TEST

START_TEST(test_datalist_remove)
  data_t *removed;

  removed = datalist_remove(list_array, 1);
  check_list(list_array, 3);
  ck_assert_ptr_eq(removed, data_true());
  data_free(removed);
END_TEST

START_TEST(test_datalist_set)
  datalist_set(list_array, 2, str_wrap("at2"));
  check_list(list_array, 4);
  ck_assert_str_eq(data_tostring(data_uncopy(datalist_get(list_array, 2))), "at2");

  datalist_set(list_array, 6, str_wrap("at6"));
  check_list(list_array, 7);
  ck_assert_str_eq(data_tostring(data_uncopy(datalist_get(list_array, 6))), "at6");
END_TEST

void tdatalist_init(void) {
  TCase *tc = tcase_create("Datalist");

  tcase_add_checked_fixture(tc, setup, teardown);
  tcase_add_test(tc, test_datalist_create);
  tcase_add_test(tc, test_datalist_get);
  tcase_add_test(tc, test_datalist_push);
  tcase_add_test(tc, test_datalist_pop);
  tcase_add_test(tc, test_datalist_remove);
  tcase_add_test(tc, test_datalist_set);
  add_tcase(tc);
}
