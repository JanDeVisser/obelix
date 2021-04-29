/*
 * This file is part of obelix (https://github.com/JanDeVisser/obelix).
 *
 * (c) Copyright Jan de Visser 2014-2021
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "strtest.h"

/* -- str_reassign ------------------------------------------------------- */

TEST_F(StrTest, Reassign) {
  str = str_copy_chars(TEST_STRING);
  char *dest;

  ASSERT_TRUE(str);
  dest = str_reassign(str);
  ASSERT_STREQ(dest, TEST_STRING);
  ASSERT_EQ(((data_t *) str)->is_live, 0);
  free(dest);
}

TEST_F(StrTest, ReassignNull) {
  char *dest;

  dest = str_reassign(NULL);
  ASSERT_FALSE(dest);
}

TEST_F(StrTest, ReassignNullStr) {
  char *dest;
  str = str_wrap(NULL);

  dest = str_reassign(NULL);
  ASSERT_FALSE(dest);
}

TEST_F(StrTest, ReassignNullString) {
  str = str_adopt(NULL);
  char *dest;

  ASSERT_TRUE(str && str_is_null(str));
  ASSERT_TRUE(((data_t *) str)->is_live);
  dest = str_reassign(str);
  ASSERT_FALSE(((data_t *) str)->is_live);
  ASSERT_FALSE(dest);
}

/* -- str_append --------------------------------------------------------- */

TEST_F(StrTest, Append) {
  str = str_copy_chars(ALPHABET);
  str_t *append = str_wrap(DIGITS);
  str_t *appended = str_append(str, append);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, TEST_STRING);
  ASSERT_GE(str->bufsize, TEST_STRING_LEN + 1);
  data_set_free(append);
}

TEST_F(StrTest, AppendToStatic) {
  str = str_wrap(ALPHABET);
  str_t *append = str_wrap(DIGITS);
  str_t *appended = str_append(str, append);
  ASSERT_FALSE(appended);
  data_set_free(append);
}

TEST_F(StrTest, AppendNull) {
  str = str_copy_chars(TEST_STRING);
  str_t *appended = str_append(str, NULL);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, TEST_STRING);
}

TEST_F(StrTest, AppendNullStr) {
  str = str_copy_chars(TEST_STRING);
  str_t *append = str_wrap(NULL);
  str_t *appended = str_append(str, append);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, TEST_STRING);
  data_set_free(append);
}

TEST_F(StrTest, AppendToNull) {
  str = str_copy_chars(TEST_STRING);
  str_t *appended = str_append(NULL, str);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendToNullStr) {
  str_t *str = str_wrap(NULL);
  str_t *append = str_copy_chars(TEST_STRING);
  str_t *appended = str_append(str, append);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, TEST_STRING);
  data_set_free(append);
}

/* -- str_append_chars --------------------------------------------------- */

TEST_F(StrTest, AppendChars) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_chars(str, DIGITS);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, TEST_STRING);
  ASSERT_GE(str->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, AppendCharsNull) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_chars(str, NULL);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, ALPHABET);
  ASSERT_GE(str->bufsize, strlen(ALPHABET) + 1);
}

TEST_F(StrTest, AppendCharsToStatic) {
  str = str_wrap(ALPHABET);
  str_t *appended = str_append_chars(str, DIGITS);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendCharsToNull) {
  str_t *appended = str_append_chars(NULL, DIGITS);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendCharsToNullStr) {
  str = str_wrap(NULL);
  str_t *appended = str_append_chars(str, DIGITS);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, DIGITS);
}

/* -- str_append_nchars -------------------------------------------------- */

TEST_F(StrTest, AppendNChars) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_nchars(str, DIGITS, 6);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, TEST_STRING_UPTO_5);
  ASSERT_GE(str->bufsize, strlen(TEST_STRING_UPTO_5) + 1);
}

TEST_F(StrTest, AppendNCharsNIsZero) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_nchars(str, DIGITS, 0);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, ALPHABET);
}

TEST_F(StrTest, AppendNCharsNIsNegative) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_nchars(str, DIGITS, -2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(appended, str);
  ASSERT_STREQ(str->buffer, TEST_STRING);
}

TEST_F(StrTest, AppendNCharsNExactStringLength) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_nchars(str, DIGITS, 10);
  ASSERT_TRUE(appended);
  ASSERT_EQ(appended, str);
  ASSERT_EQ(strcmp(str_chars(str), TEST_STRING), 0);
  ASSERT_GE(str->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, AppendNCharsNLargerThanStringLength) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_nchars(str, DIGITS, 15);
  ASSERT_TRUE(appended);
  ASSERT_EQ(appended, str);
  ASSERT_EQ(strcmp(str_chars(str), TEST_STRING), 0);
  ASSERT_GE(str->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, AppendNCharsNull) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_nchars(str, NULL, 10);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, ALPHABET);
  ASSERT_GE(str->bufsize, strlen(ALPHABET) + 1);
}

TEST_F(StrTest, AppendNCharsToStatic) {
  str = str_wrap(ALPHABET);
  str_t *appended = str_append_nchars(str, DIGITS, 6);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendNCharsToNull) {
  str_t *appended = str_append_nchars(NULL, DIGITS, 6);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendNCharsToNullStr) {
  str = str_wrap(NULL);
  str_t *appended = str_append_nchars(str, DIGITS, 10);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str->buffer, DIGITS);
}

/* -- str_append_printf -------------------------------------------------- */

TEST_F(StrTest, AppendPrintf) {
  char *s;
  str = str_copy_chars(ALPHABET);
  asprintf(&s, FMT, 1, 1, 2);
  str_t *appended = str_append_printf(str, FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_EQ(str_len(str), ALPHABET_LEN + strlen(s));
  free(s);
}

TEST_F(StrTest, AppendPrintfNull) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_printf(str, NULL, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str_chars(str), ALPHABET);
}

TEST_F(StrTest, AppendPrintfToNull) {
  str = str_append_printf(NULL, FMT, 1, 1, 2);
  ASSERT_FALSE(str);
}

TEST_F(StrTest, AppendPrintfToNullStr) {
  char *s;
  str = str_wrap(NULL);
  asprintf(&s, FMT, 1, 1, 2);
  str_t *appended = str_append_printf(str, FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str_chars(str), s);
  free(s);
}

TEST_F(StrTest, AppendPrintfToStaticStr) {
  str = str_wrap(ALPHABET);
  str_t *appended = str_append_printf(str, FMT, 1, 1, 2);
  ASSERT_FALSE(appended);
}

/* -- str_append_vprintf ------------------------------------------------- */

TEST_F(StrTest, AppendVPrintf) {
  char *s;
  asprintf(&s, FMT, 1, 1, 2);
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_EQ(str_len(str), ALPHABET_LEN + strlen(s));
  free(s);
}

TEST_F(StrTest, AppendVPrintfNull) {
  str = str_copy_chars(ALPHABET);
  str_t *appended = str_append_va_list_maker(NULL, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str_chars(str), ALPHABET);
}

TEST_F(StrTest, AppendVPrintfToNull) {
  str = NULL;
  str = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_FALSE(str);
}

TEST_F(StrTest, AppendVPrintfToNullStr) {
  char *s;
  str = str_wrap(NULL);
  asprintf(&s, FMT, 1, 1, 2);
  str_t *appended = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(str, appended);
  ASSERT_STREQ(str_chars(str), s);
  free(s);
}

TEST_F(StrTest, AppendVPrintfToStaticStr) {
  str = str_wrap(ALPHABET);
  str_t *appended = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_FALSE(appended);
}

/* -- str_chop ----------------------------------------------------------- */

TEST_F(StrTest, Chop) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_chop(str, 10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), ALPHABET);
}

TEST_F(StrTest, ChopStatic) {
  str = str_wrap(TEST_STRING);
  str_t *chopped = str_chop(str, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, ChopNull) {
  str_t *chopped = str_chop(NULL, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, ChopNullStr) {
  str = str_wrap(NULL);
  str_t *chopped = str_chop(str, 10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_TRUE(str_is_null(str));
}

TEST_F(StrTest, ChopZero) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_chop(str, 0);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, ChopStrlen) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_chop(str, TEST_STRING_LEN);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), "");
}

TEST_F(StrTest, ChopNegative) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_chop(str, -10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, ChopLarge) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_chop(str, 100);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), "");
}

/* -- str_lchop ---------------------------------------------------------- */

TEST_F(StrTest, LChop) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_lchop(str, 26);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), DIGITS);
}

TEST_F(StrTest, LChopStatic) {
  str = str_wrap(TEST_STRING);
  str_t *chopped = str_lchop(str, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, LChopNull) {
  str_t *chopped = str_lchop(NULL, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, LChopNullStr) {
  str = str_wrap(NULL);
  str_t *chopped = str_lchop(str, 10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_TRUE(str_is_null(str));
}

TEST_F(StrTest, LChopZero) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_lchop(str, 0);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, LChopStrlen) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_lchop(str, TEST_STRING_LEN);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), "");
}

TEST_F(StrTest, LChopNegative) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_lchop(str, -10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, LChopLarge) {
  str = str_copy_chars(TEST_STRING);
  str_t *chopped = str_lchop(str, 100);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(str, chopped);
  ASSERT_STREQ(str_chars(str), "");
}

/* -- str_erase ---------------------------------------------------------- */

TEST_F(StrTest, Erase) {
  str = str_copy_chars(TEST_STRING);
  str_t *erased = str_erase(str);
  ASSERT_TRUE(erased);
  ASSERT_EQ(str, erased);
  ASSERT_STREQ(str_chars(str), "");
}

TEST_F(StrTest, EraseStatic) {
  str = str_wrap(TEST_STRING);
  str_t *erased = str_erase(str);
  ASSERT_FALSE(erased);
}

TEST_F(StrTest, EraseNull) {
  str_t *erased = str_erase(NULL);
  ASSERT_FALSE(erased);
}

TEST_F(StrTest, EraseNullStr) {
  str = str_wrap(NULL);
  str_t *erased = str_erase(str);
  ASSERT_TRUE(erased);
  ASSERT_EQ(str, erased);
  ASSERT_TRUE(str_is_null(str));
}

/* -- str_set ------------------------------------------------------------ */

TEST_F(StrTest, Set) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_set(str, 5, 'Q');
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING_MUTATION_5);
}

TEST_F(StrTest, SetZero) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_set(str, 0, 'Q');
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING_MUTATION_0);
}

TEST_F(StrTest, SetStrlenMinusOne) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_set(str, strlen(TEST_STRING) - 1, 'Q');
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING_MUTATION_35);
}

TEST_F(StrTest, SetStrlen) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_set(str, strlen(TEST_STRING), 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetNull) {
  str_t *mutated = str_set(NULL, 5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetNullStr) {
  str = str_wrap(NULL);
  str_t *mutated = str_set(str, 5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetStatic) {
  str = str_wrap(ALPHABET);
  str_t *mutated = str_set(str, 5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetNegative) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_set(str, -5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetAfterEnd) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_set(str, 100, 'Q');
  ASSERT_FALSE(mutated);
}

/* -- str_forcecase ------------------------------------------------------ */

TEST_F(StrTest, ForceCaseToUpper) {
  str = str_copy_chars(TEST_STRING_LOWER);
  str_t *mutated = str_forcecase(str, TRUE);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, ForceCaseToLower) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_forcecase(str, FALSE);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING_LOWER);
}

TEST_F(StrTest, ForceCaseStatic) {
  str = str_wrap(TEST_STRING);
  str_t *mutated = str_forcecase(str, TRUE);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ForceCaseNull) {
  str_t *mutated = str_forcecase(NULL, TRUE);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ForceCaseNullStr) {
  str = str_wrap(NULL);
  str_t *mutated = str_forcecase(str, TRUE);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_TRUE(str_is_null(str));
}

/* -- str_toupper -------------------------------------------------------- */

TEST_F(StrTest, ToUpper) {
  str = str_copy_chars(TEST_STRING_LOWER);
  str_t *mutated = str_toupper(str);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, ToUpperStatic) {
  str = str_wrap(TEST_STRING);
  str_t *mutated = str_toupper(str);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToUpperNull) {
  str_t *mutated = str_toupper(NULL);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToUpperNullStr) {
  str = str_wrap(NULL);
  str_t *mutated = str_toupper(str);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_TRUE(str_is_null(str));
}

/* -- str_tolower -------------------------------------------------------- */

TEST_F(StrTest, ToLower) {
  str = str_copy_chars(TEST_STRING);
  str_t *mutated = str_tolower(str);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_STREQ(str_chars(str), TEST_STRING_LOWER);
}

TEST_F(StrTest, ToLowerStatic) {
  str = str_wrap(TEST_STRING);
  str_t *mutated = str_tolower(str);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToLowerNull) {
  str_t *mutated = str_tolower(NULL);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToLowerNullStr) {
  str = str_wrap(NULL);
  str_t *mutated = str_tolower(str);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(str, mutated);
  ASSERT_TRUE(str_is_null(str));
}

/* -- str_replace -------------------------------------------------------- */

TEST_F(StrTest, ReplaceOne) {
  str = str_copy_chars("The Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, "Pattern", "Brown", 0), 1);
  ASSERT_STREQ(str_chars(str), "The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceTwo) {
  str = str_copy_chars("Mr Pattern Chased The Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, "Pattern", "Brown", 0), 2);
  ASSERT_STREQ(str_chars(str), "Mr Brown Chased The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceAllNegativeMax) {
  str = str_copy_chars("Mr Pattern Chased Pattern Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, "Pattern", "Brown", -2), 3);
  ASSERT_STREQ(str_chars(str), "Mr Brown Chased Brown Quick Brown Fox");
}

TEST_F(StrTest, ReplaceOneOfTwo) {
  str = str_copy_chars("Mr Pattern Chased The Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, "Pattern", "Brown", 1), 1);
  ASSERT_STREQ(str_chars(str), "Mr Brown Chased The Quick Pattern Fox");
}

TEST_F(StrTest, ReplaceStart) {
  str = str_copy_chars("Pattern Chased The Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, "Pattern", "Brown", 0), 2);
  ASSERT_STREQ(str_chars(str), "Brown Chased The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceEnd) {
  str = str_copy_chars("Mr Pattern Chased The Quick Pattern Fox Pattern");
  ASSERT_EQ(str_replace(str, "Pattern", "Brown", 0), 3);
  ASSERT_STREQ(str_chars(str), "Mr Brown Chased The Quick Brown Fox Brown");
}

TEST_F(StrTest, ReplaceShorterWithLonger) {
  str = str_copy_chars("The Quick Brown Fox");
  ASSERT_EQ(str_replace(str, "The", "That", 0), 1);
  ASSERT_STREQ(str_chars(str), "That Quick Brown Fox");
}

TEST_F(StrTest, ReplaceShorterWithLongerAtEnd) {
  str = str_copy_chars("The Quick Brown Fox");
  ASSERT_EQ(str_replace(str, "Fox", "Foxes", 0), 1);
  ASSERT_STREQ(str_chars(str), "The Quick Brown Foxes");
}

TEST_F(StrTest, ReplaceRecursive) {
  str = str_copy_chars("The Quick Br Fox");
  ASSERT_EQ(str_replace(str, "Br", "Brown", 0), 1);
  ASSERT_STREQ(str_chars(str), "The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceInNull) {
  ASSERT_EQ(str_replace(NULL, "Br", "Brown", 0), -1);
}

TEST_F(StrTest, ReplaceInStatic) {
  str = str_wrap("The Quick Br Fox");
  ASSERT_EQ(str_replace(str, "Br", "Brown", 0), -1);
}

TEST_F(StrTest, ReplaceInNullStr) {
  str = str_wrap(NULL);
  ASSERT_EQ(str_replace(str, "Br", "Brown", 0), 0);
}

TEST_F(StrTest, ReplaceNullPattern) {
  str = str_copy_chars("The Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, NULL, "Brown", 0), -1);
}

TEST_F(StrTest, ReplaceNullReplacement) {
  str = str_copy_chars("The Quick Pattern Fox");
  ASSERT_EQ(str_replace(str, "Pattern", NULL, 0), -1);
}
