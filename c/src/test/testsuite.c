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
#include <stdlib.h>

#include <application.h>
#include <testsuite.h>

static type_t _type_test = {
  .hash     = (hash_t) test_hash,
  .tostring = (tostring_t) test_tostring,
  .copy     = (copy_t) test_copy,
  .free     = (free_t) test_free,
  .cmp      = (cmp_t) test_cmp
};

type_t *type_test = &_type_test;

static app_description_t   _app_descr_testsuite = {
    .name        = "obelix testrunner",
    .shortdescr  = "Testrunner for obelix test cases",
    .description = "Application framework facilitating running test cases for the obelix environment",
    .legal       = "(c) Jan de Visser <jan@finiandarcy.com> 2014-2017",
    .options     = {
        { .longopt = NULL, .shortopt = 0, .description = NULL, .flags = 0 }
    }
};

extern test_t * test_factory(const char *data) {
  return test_create(data);
}

test_t * test_create(const char *data) {
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
