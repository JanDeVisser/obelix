/*
 * /obelix/test/buffer.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <fcntl.h>
#include <unistd.h>

#include <check.h>
#include <file.h>
#include <stringbuffer.h>

START_TEST(test_sb_create)
  stringbuffer_t *sb;

  sb = sb_create("0123456789abcdefghijklmnopqrstuvwxyz\n");
  ck_assert_ptr_ne(sb, NULL);
  sb_free(sb);
END_TEST

START_TEST(test_sb_read)
  stringbuffer_t *sb;
  char buf[21];
  int ret;

  sb = sb_create("0123456789abcdefghijklmnopqrstuvwxyz\n");
  ck_assert_ptr_ne(sb, NULL);
  memset(buf, 0, 21);
  ret = sb_read(sb, buf, 20);
  ck_assert_int_eq(ret, 20);
  ck_assert_str_eq(buf, "0123456789abcdefghij");
  memset(buf, 0, 21);
  ret = sb_read(sb, buf, 20);
  ck_assert_int_eq(ret, 17);
  ck_assert_str_eq(buf, "klmnopqrstuvwxyz\n");
  ret = sb_read(sb, buf, 21);
  ck_assert_int_eq(ret, 0);
  sb_free(sb);
END_TEST


START_TEST(test_file_create)
  int fh;
  file_t *file;

  fh = open("buffertest.txt", O_RDONLY);
  ck_assert_int_gt(fh, 0);
  file = file_create(fh);
  ck_assert_ptr_ne(file, NULL);
  close(fh);
  file_free(file);
END_TEST

START_TEST(test_file_open)
  file_t *file;

  file = file_open("buffertest.txt");
  ck_assert_ptr_ne(file, NULL);
  ck_assert_int_gt(file -> fh, 0);
  file_free(file);
END_TEST

START_TEST(test_file_read)
  file_t *file;
  char buf[21];
  int ret;

  file = file_open("buffertest.txt");
  ck_assert_ptr_ne(file, NULL);
  ck_assert_int_gt(file -> fh, 0);
  memset(buf, 0, sizeof(buf));
  ret = file_read(file, buf, sizeof(buf) - 1);
  ck_assert_int_eq(ret, sizeof(buf) - 1);
  ck_assert_str_eq(buf, "0123456789abcdefghij");
  memset(buf, 0, sizeof(buf));
  ret = file_read(file, buf, sizeof(buf) - 1);
  ck_assert_int_eq(ret, 17);
  ck_assert_str_eq(buf, "klmnopqrstuvwxyz\n");
  ret = file_read(file, buf, sizeof(buf) - 1);
  ck_assert_int_eq(ret, 0);
  file_free(file);
END_TEST

void read_from_reader(reader_t *reader) {
  char buf[21];
  int ret;

  memset(buf, 0, sizeof(buf));
  ret = reader_read(reader, buf, sizeof(buf) - 1);
  ck_assert_int_eq(ret, sizeof(buf) - 1);
  ck_assert_str_eq(buf, "0123456789abcdefghij");
  memset(buf, 0, sizeof(buf));
  ret = reader_read(reader, buf, sizeof(buf) - 1);
  ck_assert_int_eq(ret, 17);
  ck_assert_str_eq(buf, "klmnopqrstuvwxyz\n");
  ret = reader_read(reader, buf, sizeof(buf) - 1);
  ck_assert_int_eq(ret, 0);
}

START_TEST(test_reader_read)
  file_t *file;
  stringbuffer_t *sb;

  file = file_open("buffertest.txt");
  ck_assert_ptr_ne(file, NULL);
  ck_assert_int_gt(file -> fh, 0);
  read_from_reader(file);
  file_free(file);

  sb = sb_create("0123456789abcdefghijklmnopqrstuvwxyz\n");
  ck_assert_ptr_ne(sb, NULL);
  read_from_reader(sb);
  sb_free(sb);
END_TEST




char * get_suite_name() {
  return "Buffer";
}

TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Buffer");

  tcase_add_test(tc, test_file_create);
  tcase_add_test(tc, test_file_open);
  tcase_add_test(tc, test_file_read);
  tcase_add_test(tc, test_sb_create);
  tcase_add_test(tc, test_sb_read);
  tcase_add_test(tc, test_reader_read);
  return tc;
}

