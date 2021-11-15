/*
 * include/testsuite.h - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __TESTSUITE_H__
#define __TESTSUITE_H__

#include <data.h>

typedef struct _test {
  char *data;
  int flag;
} test_t;

#ifdef __cplusplus
extern "C" {
#endif

extern test_t *         test_factory(const char *);
extern test_t *         test_create(const char *);
extern test_t *         test_copy(test_t *);
extern unsigned int     test_hash(test_t *);
extern int              test_cmp(test_t *, test_t *);
extern char *           test_tostring(test_t *);
extern void             test_free(test_t *);

extern type_t *         type_test;

#ifdef __cplusplus
}
#endif

#endif /* __TESTSUITE_H__ */
