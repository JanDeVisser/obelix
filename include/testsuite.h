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

#ifdef NO_CHECK

#define START_TEST(function) void function(void) {
#define END_TEST }

#define ck_assert_ptr_eq(p1, p2)     if ((p1) != (p2)) { debug("ck_assert_ptr_eq(%s,%s) tripped", #p1, #p2); assert(0); }
#define ck_assert_ptr_ne(p1, p2)     if ((p1) == (p2)) { debug("ck_assert_ptr_ne(%s,%s) tripped", #p1, #p2); assert(0); }
#define ck_assert_int_eq(i1, i2)     if ((i1) != (i2)) { debug("ck_assert_int_eq(%s,%s) tripped", #i1, #i2); assert(0); }
#define ck_assert_int_ne(i1, i2)     if ((i1) == (i2)) { debug("ck_assert_int_ne(%s,%s) tripped", #i1, #i2); assert(0); }
#define ck_assert_str_eq(s1, s2)     if (strcmp((s1), (s2))) { debug("ck_assert_str_eq(%s,%s) tripped", #s1, #s2); assert(0); }
#define ck_assert_str_ne(s1, s2)     if (!strcmp((s1), (s2))) { debug("ck_assert_str_ne(%s,%s) tripped", #s1, #s2); assert(0); }
#define mark_point()                 debug("mark_point")
#define tcase_add_test(tc, function) function()
#define add_tcase(tc)

typedef void TCase;

#else
#include <check.h>
#endif

#include <data.h>

typedef struct _test {
  char *data;
  int flag;
} test_t;

#ifdef __cplusplus
extern "C" {
#endif

__DLL_EXPORT__ test_t * test_factory(char *);
extern test_t *         test_create(char *);
extern test_t *         test_copy(test_t *);
extern unsigned int     test_hash(test_t *);
extern int              test_cmp(test_t *, test_t *);
extern char *           test_tostring(test_t *);
extern void             test_free(test_t *);

extern type_t *         type_test;

#ifndef NO_CHECK
extern void	        set_suite_name(char *);
extern void         add_tcase(TCase *);

extern data_t *     run_script(char *);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TESTSUITE_H__ */
