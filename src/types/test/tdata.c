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

#include <math.h>
#include <stdarg.h>

#include <collections.h>
#include <array.h>
#include <data.h>
#include <exception.h>

#define TEST_STRING     "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define TEST_STRING_LEN 36

data_t * execute(data_t *self, char *name, int numargs, ...) {
  va_list  arglist;
  array_t *args;
  data_t  *ret;
  data_t  *d;
  int      ix;
  int      type;
  long     intval;
  double   dblval;
  char    *ptr;
  
  args = data_array_create(numargs);
  va_start(arglist, numargs);
  for (ix = 0; ix < numargs; ix++) {
    type = va_arg(arglist, int);
    d = NULL;
    switch (type) {
      case Int:
        intval = va_arg(arglist, long);
        d = data_create(Int, intval);
        break;
      case Float:
        intval = va_arg(arglist, double);
        d = data_create(Float, dblval);
        break;
      case String:
        ptr = va_arg(arglist, char *);
        d = data_create(String, ptr);
        break;
      case Bool:
        intval = va_arg(arglist, long);
        d = data_create(Bool, intval);
        break;
      default:
        debug("Cannot do type %d. Ignored", type);
        ptr = va_arg(arglist, char *);
        break;
    }
    if (d) {
      array_push(args, d);
    }
  }
  va_end(arglist);
  ret = data_execute(self, name, args, NULL);
  if (ret && data_is_error(ret)) {
    debug("Error executing '%s'.'%s': %s", data_debugstr(self),
	  name, data_tostring(ret));
  }
  array_free(args);
  return ret;
}

START_TEST(data_string)
  data_t  *data = data_create(String, TEST_STRING);
  data_t  *ret;
  error_t *e;
  
  ck_assert_ptr_ne(data, NULL);
  ck_assert_int_eq(strcmp((char *) data -> ptrval, TEST_STRING), 0);
  ck_assert_int_eq(data_count, 1);
  
  ret = execute(data, "len", 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Int);
  ck_assert_int_eq(data_intval(ret), TEST_STRING_LEN);
  data_free(ret);
  ck_assert_int_eq(data_count, 1);
  
  ret = execute(data, "len", 1, Int, 10);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorArgCount);
  data_free(ret);

  ret = execute(data, "at", 1, Int, 10);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "K");
  data_free(ret);

  ret = execute(data, "at", 1, Int, 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "A");
  data_free(ret);

  ret = execute(data, "at", 1, Int, TEST_STRING_LEN-1);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "9");
  data_free(ret);

  ret = execute(data, "at", 1, Int, -1);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorRange);
  data_free(ret);

  ret = execute(data, "at", 1, Int, TEST_STRING_LEN);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorRange);
  data_free(ret);

  ret = execute(data, "at", 2, Int, 10, Int, 20);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorArgCount);
  data_free(ret);

  ret = execute(data, "at", 1, String, "string");
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorType);
  data_free(ret);

  ret = execute(data, "slice", 2, Int, 0, Int, 1);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "A");
  data_free(ret);

  ret = execute(data, "slice", 2, Int, -2, Int, 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "89");
  data_free(ret);

  ret = execute(data, "+", 2, String, "0123456789", String, "0123456789");
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_int_eq(strlen(data_charval(ret)), 56);
  data_free(ret);

  ret = execute(data, "+", 2, String, "0123456789", Int, 10);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorType);
  data_free(ret);

  data_free(data);
  ck_assert_int_eq(data_count, 0);
END_TEST

START_TEST(data_int)
  data_t  *d1 = data_create(Int, 1);
  data_t  *d2 = data_create(Int, 1);
  data_t  *sum;
  array_t *args;

  ck_assert_int_eq(d1 -> intval, 1);
  ck_assert_int_eq(d2 -> intval, 1);
  ck_assert_int_eq(data_count, 2);
  args = data_array_create(1);
  array_push(args, d2);
  ck_assert_int_eq(array_size(args), 1);
  sum = data_execute(d1, "+", args, NULL);
  //ck_assert_int_eq(data_count, 3);
  ck_assert_int_eq(sum -> type, Int);
  ck_assert_int_eq(sum -> intval, 2);
  data_free(sum);
  //ck_assert_int_eq(data_count, 2);

  array_clear(args);
  d2 = data_create(Int, 1);
  array_push(args, data_copy(d1));
  array_push(args, d2);
  array_push(args, data_copy(d2));
  sum = data_execute(NULL, "+", args, NULL);
  //ck_assert_int_eq(data_count, 3);
  ck_assert_int_eq(sum -> type, Int);
  ck_assert_int_eq(sum -> intval, 3);

  array_free(args);
  data_free(d1);
  data_free(sum);
  //ck_assert_int_eq(data_count, 0);
END_TEST

START_TEST(data_parsers)
  data_t *d;

  d = data_parse(String, TEST_STRING);
  ck_assert_ptr_ne(d, NULL);
  ck_assert_int_eq(d -> type, String);
  ck_assert_str_eq(d -> ptrval, TEST_STRING);
  data_free(d);

  d = data_parse(Int, "42");
  ck_assert_ptr_ne(d, NULL);
  ck_assert_int_eq(d -> type, Int);
  ck_assert_int_eq(d -> intval, 42);
  data_free(d);

  d = data_parse(Float, "3.14");
  ck_assert_ptr_ne(d, NULL);
  ck_assert_int_eq(d -> type, Float);
  ck_assert(fabs(d -> dblval - 3.14) < 0.001);
  data_free(d);

  /* We don't parse & round decimals. Is that right? */
  d = data_parse(Int, "3.14");
  ck_assert_ptr_eq(d, NULL);

  d = data_parse(Float, "42");
  ck_assert_ptr_ne(d, NULL);
  ck_assert_int_eq(d -> type, Float);
  ck_assert(fabs(d -> dblval - 42.0) < 0.001);
  data_free(d);

END_TEST

START_TEST(test_data_cmp)
  data_t *i1 = data_create(Int, 1);
  data_t *i2 = data_create(Int, 2);
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

char * get_suite_name() {
  return "Data";
}

TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Data");

  tcase_add_test(tc, data_string);
  tcase_add_test(tc, data_int);
  tcase_add_test(tc, data_parsers);
  tcase_add_test(tc, test_data_cmp);
  return tc;
}


int main(void){
  int number_failed;
  Suite *s;
  SRunner *sr;
  TCase *tc;
  int ix;

  s = suite_create(get_suite_name());
  for (ix = 0; TRUE; ix++) {
    tc = get_testcase(ix);
    if (tc) {
      suite_add_tcase(s, tc);
    } else {
      break;
    }
  }
  sr = srunner_create(s);

  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
