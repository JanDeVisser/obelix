/*
 * collections.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
#include <string.h>
#include <stdlib.h>

#include <core.h>
#include <resolve.h>
#include <testsuite.h>

static type_t _type_test = {
  .hash     = (hash_t) test_hash,
  .tostring = (tostring_t) test_tostring,
  .copy     = (copy_t) test_copy,
  .free     = (free_t) test_free,
  .cmp      = (cmp_t) test_cmp
};

type_t *type_test = &_type_test;

__DLL_EXPORT__ test_t * test_factory(char *data) {
  return test_create(data);
}

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

static Suite *_suite = NULL;

void add_tcase(TCase *tc) {
  if (_suite && tc) {
    suite_add_tcase(_suite, tc);
  }
}

extern void init_suite(int, char **);

int main(int argc, char **argv){
  int      number_failed;
  SRunner *sr;

  _suite = suite_create("default");
  init_suite(argc, argv);
  sr = srunner_create(_suite);
  //srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
