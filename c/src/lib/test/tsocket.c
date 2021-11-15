/*
 * /obelix/lib/test/tsocket.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <socket.h>
#include <testsuite.h>

static void _init_tsocket(void) __attribute__((constructor(300)));

START_TEST(test_socket_create)
  socket_t *s = socket_create("www.google.com", 80);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_int_gt(s -> sockfile -> fh, 0);
  socket_free(s);
END_TEST

START_TEST(test_socket_create_byservice)
  socket_t *s = socket_create_byservice("www.google.com", "http");
  ck_assert_ptr_ne(s, NULL);
  ck_assert_int_gt(s -> sockfile -> fh, 0);
  socket_free(s);
END_TEST

START_TEST(test_socket_read)
  socket_t *s = socket_create("www.google.com", 80);
  char     *send = "GET /\n\n";
  char     *reply = "HTTP/1.0 302 Found";
  char      buf[101];
  int       ret;
  
  ck_assert_ptr_ne(s, NULL);
  ck_assert_int_gt(s -> sockfile -> fh, 0);
  
  ret = file_write(s -> sockfile, send, strlen(send));
  ck_assert_int_eq(ret, strlen(send));
  ret = file_read(s -> sockfile, buf, 100);
  ck_assert_int_ge(ret, strlen(reply));
  buf[strlen(reply)] = 0;
  ck_assert_str_eq(buf, reply);
  socket_free(s);
END_TEST

static void _init_tsocket(void) {
  TCase *tc = tcase_create("Socket");

  tcase_add_test(tc, test_socket_create);
  tcase_add_test(tc, test_socket_create_byservice);
  tcase_add_test(tc, test_socket_read);
  add_tcase(tc);
}

