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


 #include "tcore.h"
 #include <stdio.h>
 #include <stdlib.h>

#include <dict.h>

#define MANY 500

typedef struct _test_dict_ctx {
  dict_t  *dict;
  char    *keybuf;
  char   **keys;
  int      size;
} test_dict_ctx_t;

static void _setup(void) {
}

static void _teardown(void) {
}

test_dict_ctx_t * ctx_create(int num) {
  test_t           *test;
  test_dict_ctx_t  *ret;
  dict_t           *dict;
  int               ix;
  char              valbuf[10];
  char             *keybuf;
  char            **keys;

  mark_point();
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_key_type(dict, coretype(CTString));
  ck_assert_ptr_eq(dict -> key_type.cmp, strcmp);
  ck_assert_ptr_eq(dict -> key_type.hash, strhash);
  ck_assert_ptr_eq(dict -> key_type.free, free);
  // dict_set_hash(dict, (hash_t) strhash);
  // dict_set_free_key(dict, (visit_t) free);
  dict_set_free_data(dict, (visit_t) test_free);
  keybuf = stralloc(num * 11);
  keys = (char **) resize_ptrarray(NULL, num, 0);
  ck_assert_ptr_ne(keys, NULL);
  mark_point();
  for (ix = 0; ix < num; ix++) {
    keys[ix] = keybuf + (ix * 11);
    strrand(keys[ix], 10);
    sprintf(valbuf, "%d", ix);
    test = test_create(valbuf);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_ptr_eq(dict_put(dict, strdup(keys[ix]), test), dict);
    ck_assert_int_eq(dict_size(dict), ix + 1);
  }
  ret = NEW(test_dict_ctx_t);
  ck_assert_ptr_ne(ret, NULL);
  ret -> dict = dict;
  ret -> keys = keys;
  ret -> keybuf = keybuf;
  ret -> size = num;
  return ret;
}

void ctx_free(test_dict_ctx_t *ctx) {
  dict_free(ctx -> dict);
  free(ctx -> keys);
  free(ctx -> keybuf);
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
  void   *d;

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
  test_t          *test;
  int              ix;

  ctx = ctx_create(MANY);
  /* dict_dump(ctx -> dict); */

  for (ix = 0; ix < ctx -> size; ix++) {
    mark_point();
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
    mark_point();
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
    mark_point();
    success = dict_remove(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_eq(success, ctx -> dict);
    ck_assert_int_eq(dict_size(ctx -> dict), ctx -> size - ix - 1);
  }
  ctx_free(ctx);
END_TEST


void test_dict_visitor(entry_t *entry) {
  test_t *t;

  mark_point();
  t = (test_t *) entry -> value;
  t -> flag = 1;
}


int * test_dict_reducer(entry_t *entry, int *sum) {
  test_t *t;

  mark_point();
  t = (test_t *) entry -> value;
  *sum += t -> flag;
  return sum;
}


START_TEST(test_dict_visit_reduce)
  test_dict_ctx_t *ctx;
  test_t          *test;
  int              ix;
  int              sum;

  ctx = ctx_create(MANY);
  mark_point();
  dict_visit(ctx -> dict, (visit_t) test_dict_visitor);
  for (ix = 0; ix < ctx -> size; ix++) {
    mark_point();
    test = (test_t *) dict_get(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_eq(test -> flag, 1);
  }
  sum = 0;
  dict_reduce(ctx -> dict, (reduce_t) test_dict_reducer, &sum);
  ck_assert_int_eq(sum, MANY);
  ctx_free(ctx);
END_TEST

START_TEST(test_dictiter)
  test_dict_ctx_t *ctx;
  test_t          *test;
  dictiterator_t  *di;
  int              sum;
  int              ix;
  entry_t         *entry;
  int              num = MANY;

  ctx = ctx_create(num);
  mark_point();
  for (di = di_create(ctx -> dict); di_has_next(di); ) {
    entry = di_next(di);
    test = (test_t *) entry -> value;
    test -> flag = 1;
  }
  for (ix = 0; ix < ctx -> size; ix++) {
    mark_point();
    test = (test_t *) dict_get(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_eq(test -> flag, 1);
  }
  sum = 0;
  for (di_head(di); di_has_next(di); ) {
    entry = di_next(di);
    test = (test_t *) entry -> value;
    sum += test -> flag;
  }
  ck_assert_int_eq(sum, num);
  di_free(di);
  ctx_free(ctx);
END_TEST

START_TEST(test_dictiter_backwards)
  test_dict_ctx_t *ctx;
  test_t          *test;
  dictiterator_t  *di;
  int              sum;
  int              ix;
  entry_t         *entry;
  int              num = MANY;

  ctx = ctx_create(num);
  mark_point();
  di = di_create(ctx -> dict);
  for (di_tail(di); di_has_prev(di); ) {
    entry = di_prev(di);
    test = (test_t *) entry -> value;
    test -> flag = 1;
  }
  for (ix = 0; ix < ctx -> size; ix++) {
    mark_point();
    test = (test_t *) dict_get(ctx -> dict, ctx -> keys[ix]);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_eq(test -> flag, 1);
  }
  sum = 0;
  for (di_tail(di); di_has_prev(di); ) {
    entry = di_prev(di);
    test = (test_t *) entry -> value;
    sum += test -> flag;
  }
  ck_assert_int_eq(sum, num);
  di_free(di);
  ctx_free(ctx);
END_TEST

void dict_init(void) {
  TCase *tc = tcase_create("Dict");

  tcase_add_checked_fixture(tc, _setup, _teardown);
  tcase_add_test(tc, test_dict_create);
  tcase_add_test(tc, test_dict_put_one);
  tcase_add_test(tc, test_dict_put_one_get_one);
  tcase_add_test(tc, test_dict_put_many);
  tcase_add_test(tc, test_dict_clear);
  tcase_add_test(tc, test_dict_has_key);
  tcase_add_test(tc, test_dict_remove);
  tcase_add_test(tc, test_dict_visit_reduce);
  tcase_add_test(tc, test_dictiter);
  tcase_add_test(tc, test_dictiter_backwards);
  add_tcase(tc);
}
