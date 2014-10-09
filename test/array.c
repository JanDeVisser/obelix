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

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/array.h"
#include "collections.h"

START_TEST(test_array_create)
  array_t *array;
  
  fprintf(stderr, "1 test_array_create\n");
  array = array_create(4);
  fprintf(stderr, "2 test_array_create\n");
  
  ck_assert_int_eq(array_size(array), 0);
  ck_assert_int_eq(array_capacity(array), 4);
END_TEST

TCase * tc_array(void) {
  TCase *tc;
  tc = tcase_create("Array");

  tcase_add_test(tc, test_array_create);

  return tc;
}
 


