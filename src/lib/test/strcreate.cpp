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

#include "strtest.h"
#include <array.h>
#include <list.h>

TEST_F(StrTest, Create) {
  string = str_create(10);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_is_null(string));
  ASSERT_STREQ(string->buffer, "");
  ASSERT_EQ(string->bufsize, 10);
}

TEST_F(StrTest, CopyChars) {
  string = str(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, CopyCharsNull) {
  string = str(NULL);
  ASSERT_TRUE(string);
  ASSERT_FALSE(string->buffer);
  ASSERT_TRUE(str_is_null(string));
}

TEST_F(StrTest, StrWrap) {
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_EQ(str_len(string), TEST_STRING_LEN);
  ASSERT_EQ(string->buffer, TEST_STRING);
  ASSERT_EQ(string->bufsize, 0);
}

TEST_F(StrTest, StrWrapRelease) {
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_EQ(str_len(string), TEST_STRING_LEN);
  ASSERT_EQ(string->buffer, TEST_STRING);
  ASSERT_EQ(string->bufsize, 0);
  data_release(string);
  ASSERT_EQ(((data_t *) string)->is_live, 0);
}

TEST_F(StrTest, StrWrapNull) {
  string = str_wrap(NULL);
  ASSERT_TRUE(string);
  ASSERT_TRUE(str_is_null(string));
}

TEST_F(StrTest, StrAdopt) {
  char *copy = strdup(TEST_STRING);
  string = str_adopt(copy);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, StrAdoptNull) {
  string = str_adopt(NULL);
  ASSERT_TRUE(string);
  ASSERT_TRUE(str_is_null(string));
}

TEST_F(StrTest, StrCopyNChars) {
  string = str_n(TEST_STRING, 10);
  ASSERT_TRUE(string);
  ASSERT_EQ(strncmp(str_chars(string), TEST_STRING, 10), 0);
  ASSERT_EQ(string->bufsize, 11);
}

TEST_F(StrTest, StrCopyNCharsNIsZero) {
  string = str_n(TEST_STRING, 0);
  ASSERT_TRUE(string);
  ASSERT_EQ(string->buffer[0], 0);
  ASSERT_EQ(string->bufsize, 1);
  ASSERT_EQ(str_len(string), 0);
}

TEST_F(StrTest, StrCopyNCharsNIsNegative) {
  string = str_n(TEST_STRING, -2);
  ASSERT_TRUE(string);
  ASSERT_STREQ(string->buffer, TEST_STRING);
}

TEST_F(StrTest, StrCopyNCharsNExactStringLength) {
  string = str_n(TEST_STRING, TEST_STRING_LEN);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, StrCopyNCharsNLargerThanStringLength) {
  string = str_n(TEST_STRING, 40);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
}

/* -- str_from_data ------------------------------------------------------ */

TEST_F(StrTest, FromDataStr) {
  data = data_create(String, TEST_STRING);
  string = str_from_data(data);
  ASSERT_TRUE(string);
  ASSERT_EQ((data_t *) string, data);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, FromDataInt) {
  data = int_to_data(42);
  string = str_from_data(data);
  ASSERT_TRUE(string);
  ASSERT_NE((data_t *) string, data);
  ASSERT_EQ(strcmp(str_chars(string), "42"), 0);
  ASSERT_EQ(string->bufsize, 3);
}

TEST_F(StrTest, FromDataNull) {
  string = str_from_data(NULL);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_chars(string));
}

TEST_F(StrTest, FromDataDataNull) {
  string = str_from_data(data_null());
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_chars(string));
}

/* -- str_printf --------------------------------------------------------- */

TEST_F(StrTest, Printf) {
  char *s;
  asprintf(&s, FMT, 1, 1, 2);
  string = str_printf(FMT, 1, 1, 2);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), s), 0);
  ASSERT_EQ(string->bufsize, strlen(s) + 1);
  free(s);
}

TEST_F(StrTest, PrintfNull) {
  string = str_printf(NULL, 1, 1, 2);
  ASSERT_FALSE(string);
}

/* -- str_vprintf -------------------------------------------------------- */

TEST_F(StrTest, VPrintf) {
  char *s;
  asprintf(&s, FMT, 1, 1, 2);
  auto va_list_maker = [&](const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return str_vprintf(fmt, args);
    va_end(args);
  };
  string = va_list_maker(FMT, 1, 1, 2);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), s), 0);
  ASSERT_EQ(string->bufsize, strlen(s) + 1);
  free(s);
}

TEST_F(StrTest, VPrintfNull) {
  auto va_list_maker = [&](const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    return str_vprintf(fmt, args);
    va_end(args);
  };
  string = va_list_maker(NULL, 1, 1, 2);
  ASSERT_FALSE(string);
}

/* -- str_duplicate ------------------------------------------------------ */

TEST_F(StrTest, Duplicate) {
  str_t *s = str_wrap(TEST_STRING);
  string = str_duplicate(s);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
  str_free(s);
}

TEST_F(StrTest, DuplicateNull) {
  string = str_duplicate(NULL);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_chars(string));
}

/* -- str_deepcopy ------------------------------------------------------- */

/*
 * str_deepcopy is an alias of str_duplicate, but this may change.
 */

TEST_F(StrTest, DeepCopy) {
  str_t *s = str_wrap(TEST_STRING);
  string = str_deepcopy(s);
  ASSERT_TRUE(string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_EQ(string->bufsize, TEST_STRING_LEN + 1);
  str_free(s);
}

TEST_F(StrTest, DeepCopyNull) {
  string = str_deepcopy(NULL);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_chars(string));
}

/* -- str_slice ---------------------------------------------------------- */

TEST_F(StrTest, Slice) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 10, 20);
  ASSERT_EQ(strcmp(slice->buffer, TEST_SLICE), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceStart) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 0, 10);
  ASSERT_EQ(strcmp(slice->buffer, TEST_SLICE_START), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceEnd) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 26, 36);
  ASSERT_EQ(strcmp(slice->buffer, TEST_SLICE_END), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceBeforeStart) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, -10, 10);
  ASSERT_EQ(strcmp(slice->buffer, TEST_SLICE_START), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceAfterEnd) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 26, 50);
  ASSERT_EQ(strcmp(slice->buffer, TEST_SLICE_END), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceOffsetFromEnd) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 10, -16);
  ASSERT_EQ(strcmp(slice->buffer, TEST_SLICE), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceFromGreaterUpto) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 10, 5);
  ASSERT_EQ(strcmp(slice->buffer, ""), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceFromEqualsUpto) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 10, 10);
  ASSERT_EQ(strcmp(slice->buffer, ""), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceUptoOffsetFromEndBeforeFrom) {
  string = str_wrap(TEST_STRING);
  str_t *slice = str_slice(string, 10, -28);
  ASSERT_EQ(strcmp(slice->buffer, ""), 0);
  data_release(slice);
}

TEST_F(StrTest, SliceNull) {
  str_t *slice = str_slice(NULL, 10, 20);
  ASSERT_TRUE(str_is_null(slice));
  data_release(slice);
}

TEST_F(StrTest, SliceNullStr) {
  string = str_wrap(NULL);
  ASSERT_TRUE(str_is_null(string));
  str_t *slice = str_slice(string, 10, 20);
  ASSERT_TRUE(str_is_null(slice));
  data_release(slice);
}

/* -- str_join ----------------------------------------------------------- */

TEST_F(StrTest, Join) {
  list_t *list = list_create();

  list_append(list, (void *) "The");
  list_append(list, (void *) "Quick");
  list_append(list, (void *) "Brown");
  list_append(list, (void *) "Fox");
  string = str_join(" ", list, _list_reduce);
  ASSERT_TRUE(string);
  ASSERT_STREQ(str_chars(string), "The Quick Brown Fox");
  list_free(list);
}

TEST_F(StrTest, JoinArray) {
  array_t *arr = array_create(4);

  array_set(arr, 0, (void *) "The");
  array_set(arr, 1, (void *) "Quick");
  array_set(arr, 2, (void *) "Brown");
  array_set(arr, 3, (void *) "Fox");
  string = str_join(" ", arr, array_reduce);
  ASSERT_TRUE(string);
  ASSERT_STREQ(str_chars(string), "The Quick Brown Fox");
  array_free(arr);
}

TEST_F(StrTest, JoinNullGlue) {
  list_t *list = list_create();

  list_append(list, (void *) "The");
  list_append(list, (void *) "Quick");
  list_append(list, (void *) "Brown");
  list_append(list, (void *) "Fox");
  string = str_join(NULL, list, _list_reduce);
  ASSERT_TRUE(string);
  ASSERT_STREQ(str_chars(string), "TheQuickBrownFox");
  list_free(list);
}

TEST_F(StrTest, JoinNullCollection) {
  string = str_join(" ", NULL, _list_reduce);
  ASSERT_FALSE(string);
}

TEST_F(StrTest, JoinWithNullReducer) {
  list_t *list = list_create();

  list_append(list, (void *) "The");
  list_append(list, (void *) "Quick");
  list_append(list, (void *) "Brown");
  list_append(list, (void *) "Fox");
  string = str_join(" ", list, NULL);
  ASSERT_FALSE(string);
  list_free(list);
}
