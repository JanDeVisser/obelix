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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dict.h>

#define NO_CHECK
#include <testsuite.h>

#define MANY 500

test_t * test_create(char *data) {
  test_t *ret;

  ret = NEW(test_t);
  if (ret) {
    ret -> data = (data) ? strdup(data) : NULL;
    ret -> flag = 0;
  }
  return ret;
}

test_t * test_copy(test_t *test) {
  return test_create(test -> data);
}

int test_cmp(test_t *test, test_t *other) {
  return strcmp(test -> data, other -> data);
}

unsigned int test_hash(test_t *test) {
  return (test -> data) ? strhash(test -> data) : 0;
}

char * test_tostring(test_t *test) {
  static char buf[100];
  snprintf(buf, 100, "%s [%d]", test -> data, test -> flag);
  return buf;
}

void test_free(test_t *test) {
  if (test && test -> data) {
    free(test -> data);
  }
  free(test);
}



static void _init_tdict(void) /* __attribute__((constructor(300))) */;

typedef struct _test_dict_ctx {
  dict_t *dict;
  char **keys;
  int size;
} test_dict_ctx_t;

test_dict_ctx_t * ctx_create(int num) {
  test_t *test;
  test_dict_ctx_t *ret;
  dict_t *dict;
  int ix;
  char valbuf[10];
  char **keys;

  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (visit_t) free);
  dict_set_free_data(dict, (visit_t) test_free);
  keys = (char **) resize_ptrarray(NULL, MANY, 0);
  ck_assert_ptr_ne(keys, NULL);
  for (ix = 0; ix < num; ix++) {
    keys[ix] = malloc(11);
    strrand(keys[ix], 10);
    sprintf(valbuf, "%d", ix);
    test = test_create(valbuf);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_ne(dict_put(dict, strdup(keys[ix]), test), FALSE);
    ck_assert_int_eq(dict_size(dict), ix + 1);
  }
  ret = NEW(test_dict_ctx_t);
  ck_assert_ptr_ne(ret, NULL);
  ret -> dict = dict;
  ret -> keys = keys;
  ret -> size = num;
  return ret;
}

void ctx_free(test_dict_ctx_t *ctx) {
  int ix;

  dict_free(ctx -> dict);
  for (ix = 0; ix < ctx -> size; ix++) {
    free(ctx -> keys[ix]);
  }
  free(ctx -> keys);
  free(ctx);
}

START_TEST(test_dict_create)
  dict_t *dict;
  
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_free(dict);
END_TEST


START_TEST(test_dict_put_one)
  dict_t *dict;
  
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (free_t) free);
  dict_set_free_data(dict, (free_t) free);
  dict_put(dict, strdup("key1"), strdup("data1"));
  ck_assert_int_eq(dict_size(dict), 1);
  dict_free(dict);
END_TEST


START_TEST(test_dict_put_one_get_one)
  dict_t *dict;
  void *d;
  
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (free_t) free);
  dict_set_free_data(dict, (free_t) free);
  dict_put(dict, strdup("key1"), strdup("data1"));
  ck_assert_int_eq(dict_size(dict), 1);
  d = dict_get(dict, "key1");
  ck_assert_ptr_ne(d, NULL);
  ck_assert_str_eq(d, "data1");
  dict_free(dict);
END_TEST


START_TEST(test_dict_put_many)
  test_dict_ctx_t *ctx;
  test_t *test;
  dict_t *dict;
  int ix;
  char **keys;

  ctx = ctx_create(MANY);
  
  for (ix = 0; ix < ctx -> size; ix++) {
    test = (test_t *) dict_get(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_eq(atoi((char *) test -> data), ix);
  }
  ctx_free(ctx);
END_TEST


START_TEST(test_dict_clear)
  test_dict_ctx_t *ctx;

  ctx = ctx_create(MANY);

  dict_clear(ctx -> dict);
  ck_assert_int_eq(dict_size(ctx -> dict), 0);
  /* dict_dump(ctx -> dict); */
  ctx_free(ctx);
END_TEST


START_TEST(test_dict_has_key)
  test_dict_ctx_t *ctx;
  int ix;
  char keybuf[21];
  int success;

  ctx = ctx_create(MANY);
  for (ix = 0; ix < ctx -> size; ix++) {
    success = dict_has_key(ctx -> dict, ctx -> keys[ix]);
    ck_assert_int_eq(success, TRUE);
    sprintf(keybuf, "%s%s", ctx -> keys[ix], ctx -> keys[ix]);
    success = dict_has_key(ctx -> dict, keybuf);
    ck_assert_int_eq(success, FALSE);
  }
  ctx_free(ctx);
END_TEST

START_TEST(test_dict_remove)
  test_dict_ctx_t *ctx;
  int              ix;
  dict_t          *success;

  ctx = ctx_create(MANY);
  for (ix = 0; ix < ctx -> size; ix++) {
    success = dict_remove(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_eq(success, ctx -> dict);
    ck_assert_int_eq(dict_size(ctx -> dict), ctx -> size - ix - 1);
    success = dict_remove(ctx -> dict, ctx -> keys[ix]);
    ck_assert_int_eq(success, FALSE);
  }
  ctx_free(ctx);
END_TEST


void test_dict_visitor(entry_t *entry) {
  test_t *t;

  t = (test_t *) entry -> value;
  t -> flag = 1;
}


int * test_dict_reducer(entry_t *entry, int *sum) {
  test_t *t;

  t = (test_t *) entry -> value;
  *sum += t -> flag;
  return sum;
}


START_TEST(test_dict_visit_reduce)
  test_dict_ctx_t *ctx;
  test_t *test;
  int ix;
  int sum;

  ctx = ctx_create(MANY);
  dict_visit(ctx -> dict, (visit_t) test_dict_visitor);
  for (ix = 0; ix < ctx -> size; ix++) {
    test = (test_t *) dict_get(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_eq(test -> flag, 1);
  }
  sum = 0;
  dict_reduce(ctx -> dict, (reduce_t) test_dict_reducer, &sum);
  ck_assert_int_eq(sum, MANY);
  ctx_free(ctx);
END_TEST


  
static void _init_tdict(void) {
  //TCase *tc = tcase_create("Dict");

  tcase_add_test(tc, test_dict_create);
  tcase_add_test(tc, test_dict_put_one);
  tcase_add_test(tc, test_dict_put_one_get_one);
  tcase_add_test(tc, test_dict_put_many);
  tcase_add_test(tc, test_dict_clear);
  tcase_add_test(tc, test_dict_has_key);
  tcase_add_test(tc, test_dict_remove);
  tcase_add_test(tc, test_dict_visit_reduce);
  //add_tcase(tc);
}

int main(int argc, char **argv) {
  _init_tdict();
}
