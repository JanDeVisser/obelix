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
  string = str(TEST_STRING);
  char *dest;

  ASSERT_TRUE(string);
  dest = str_reassign(string);
  ASSERT_STREQ(dest, TEST_STRING);
  ASSERT_EQ(((data_t *) string)->is_live, 0);
  free(dest);
}

TEST_F(StrTest, ReassignNull) {
  char *dest;

  dest = str_reassign(NULL);
  ASSERT_FALSE(dest);
}

TEST_F(StrTest, ReassignNullStr) {
  char *dest;
  string = str_wrap(NULL);

  dest = str_reassign(NULL);
  ASSERT_FALSE(dest);
}

TEST_F(StrTest, ReassignNullString) {
  string = str_adopt(NULL);
  char *dest;

  ASSERT_TRUE(string && str_is_null(string));
  ASSERT_TRUE(((data_t *) string)->is_live);
  dest = str_reassign(string);
  ASSERT_FALSE(((data_t *) string)->is_live);
  ASSERT_FALSE(dest);
}

/* -- str_append --------------------------------------------------------- */

TEST_F(StrTest, Append) {
  string = str(ALPHABET);
  str_t *append = str_wrap(DIGITS);
  str_t *appended = str_append(string, append);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, TEST_STRING);
  ASSERT_GE(string->bufsize, TEST_STRING_LEN + 1);
  data_set_free(append);
}

TEST_F(StrTest, AppendToStatic) {
  string = str_wrap(ALPHABET);
  str_t *append = str_wrap(DIGITS);
  str_t *appended = str_append(string, append);
  ASSERT_FALSE(appended);
  data_set_free(append);
}

TEST_F(StrTest, AppendNull) {
  string = str(TEST_STRING);
  str_t *appended = str_append(string, NULL);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, TEST_STRING);
}

TEST_F(StrTest, AppendNullStr) {
  string = str(TEST_STRING);
  str_t *append = str_wrap(NULL);
  str_t *appended = str_append(string, append);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, TEST_STRING);
  data_set_free(append);
}

TEST_F(StrTest, AppendToNull) {
  string = str(TEST_STRING);
  str_t *appended = str_append(NULL, string);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendToNullStr) {
  str_t *string = str_wrap(NULL);
  str_t *append = str(TEST_STRING);
  str_t *appended = str_append(string, append);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, TEST_STRING);
  data_set_free(append);
}

/* -- str_append_chars --------------------------------------------------- */

TEST_F(StrTest, AppendChars) {
  string = str(ALPHABET);
  str_t *appended = str_append_chars(string, DIGITS);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, TEST_STRING);
  ASSERT_GE(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, AppendCharsNull) {
  string = str(ALPHABET);
  str_t *appended = str_append_chars(string, NULL);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, ALPHABET);
  ASSERT_GE(string->bufsize, strlen(ALPHABET) + 1);
}

TEST_F(StrTest, AppendCharsToStatic) {
  string = str_wrap(ALPHABET);
  str_t *appended = str_append_chars(string, DIGITS);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendCharsToNull) {
  str_t *appended = str_append_chars(NULL, DIGITS);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendCharsToNullStr) {
  string = str_wrap(NULL);
  str_t *appended = str_append_chars(string, DIGITS);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, DIGITS);
}

/* -- str_append_nchars -------------------------------------------------- */

TEST_F(StrTest, AppendNChars) {
  string = str(ALPHABET);
  str_t *appended = str_append_nchars(string, DIGITS, 6);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, TEST_STRING_UPTO_5);
  ASSERT_GE(string->bufsize, strlen(TEST_STRING_UPTO_5) + 1);
}

TEST_F(StrTest, AppendNCharsNIsZero) {
  string = str(ALPHABET);
  str_t *appended = str_append_nchars(string, DIGITS, 0);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, ALPHABET);
}

TEST_F(StrTest, AppendNCharsNIsNegative) {
  string = str(ALPHABET);
  str_t *appended = str_append_nchars(string, DIGITS, -2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(appended, string);
  ASSERT_STREQ(string->buffer, TEST_STRING);
}

TEST_F(StrTest, AppendNCharsNExactStringLength) {
  string = str(ALPHABET);
  str_t *appended = str_append_nchars(string, DIGITS, 10);
  ASSERT_TRUE(appended);
  ASSERT_EQ(appended, string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_GE(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, AppendNCharsNLargerThanStringLength) {
  string = str(ALPHABET);
  str_t *appended = str_append_nchars(string, DIGITS, 15);
  ASSERT_TRUE(appended);
  ASSERT_EQ(appended, string);
  ASSERT_EQ(strcmp(str_chars(string), TEST_STRING), 0);
  ASSERT_GE(string->bufsize, TEST_STRING_LEN + 1);
}

TEST_F(StrTest, AppendNCharsNull) {
  string = str(ALPHABET);
  str_t *appended = str_append_nchars(string, NULL, 10);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, ALPHABET);
  ASSERT_GE(string->bufsize, strlen(ALPHABET) + 1);
}

TEST_F(StrTest, AppendNCharsToStatic) {
  string = str_wrap(ALPHABET);
  str_t *appended = str_append_nchars(string, DIGITS, 6);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendNCharsToNull) {
  str_t *appended = str_append_nchars(NULL, DIGITS, 6);
  ASSERT_FALSE(appended);
}

TEST_F(StrTest, AppendNCharsToNullStr) {
  string = str_wrap(NULL);
  str_t *appended = str_append_nchars(string, DIGITS, 10);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(string->buffer, DIGITS);
}

/* -- str_append_printf -------------------------------------------------- */

TEST_F(StrTest, AppendPrintf) {
  char *s;
  string = str(ALPHABET);
  asprintf(&s, FMT, 1, 1, 2);
  str_t *appended = str_append_printf(string, FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_EQ(str_len(string), ALPHABET_LEN + strlen(s));
  free(s);
}

TEST_F(StrTest, AppendPrintfNull) {
  string = str(ALPHABET);
  str_t *appended = str_append_printf(string, NULL, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(str_chars(string), ALPHABET);
}

TEST_F(StrTest, AppendPrintfToNull) {
  string = str_append_printf(NULL, FMT, 1, 1, 2);
  ASSERT_FALSE(string);
}

TEST_F(StrTest, AppendPrintfToNullStr) {
  char *s;
  string = str_wrap(NULL);
  asprintf(&s, FMT, 1, 1, 2);
  str_t *appended = str_append_printf(string, FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(str_chars(string), s);
  free(s);
}

TEST_F(StrTest, AppendPrintfToStaticStr) {
  string = str_wrap(ALPHABET);
  str_t *appended = str_append_printf(string, FMT, 1, 1, 2);
  ASSERT_FALSE(appended);
}

/* -- str_append_vprintf ------------------------------------------------- */

TEST_F(StrTest, AppendVPrintf) {
  char *s;
  asprintf(&s, FMT, 1, 1, 2);
  string = str(ALPHABET);
  str_t *appended = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_EQ(str_len(string), ALPHABET_LEN + strlen(s));
  free(s);
}

TEST_F(StrTest, AppendVPrintfNull) {
  string = str(ALPHABET);
  str_t *appended = str_append_va_list_maker(NULL, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(str_chars(string), ALPHABET);
}

TEST_F(StrTest, AppendVPrintfToNull) {
  string = NULL;
  string = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_FALSE(string);
}

TEST_F(StrTest, AppendVPrintfToNullStr) {
  char *s;
  string = str_wrap(NULL);
  asprintf(&s, FMT, 1, 1, 2);
  str_t *appended = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_TRUE(appended);
  ASSERT_EQ(string, appended);
  ASSERT_STREQ(str_chars(string), s);
  free(s);
}

TEST_F(StrTest, AppendVPrintfToStaticStr) {
  string = str_wrap(ALPHABET);
  str_t *appended = str_append_va_list_maker(FMT, 1, 1, 2);
  ASSERT_FALSE(appended);
}

/* -- str_chop ----------------------------------------------------------- */

TEST_F(StrTest, Chop) {
  string = str(TEST_STRING);
  str_t *chopped = str_chop(string, 10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), ALPHABET);
}

TEST_F(StrTest, ChopStatic) {
  string = str_wrap(TEST_STRING);
  str_t *chopped = str_chop(string, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, ChopNull) {
  str_t *chopped = str_chop(NULL, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, ChopNullStr) {
  string = str_wrap(NULL);
  str_t *chopped = str_chop(string, 10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_TRUE(str_is_null(string));
}

TEST_F(StrTest, ChopZero) {
  string = str(TEST_STRING);
  str_t *chopped = str_chop(string, 0);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, ChopStrlen) {
  string = str(TEST_STRING);
  str_t *chopped = str_chop(string, TEST_STRING_LEN);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), "");
}

TEST_F(StrTest, ChopNegative) {
  string = str(TEST_STRING);
  str_t *chopped = str_chop(string, -10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, ChopLarge) {
  string = str(TEST_STRING);
  str_t *chopped = str_chop(string, 100);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), "");
}

/* -- str_lchop ---------------------------------------------------------- */

TEST_F(StrTest, LChop) {
  string = str(TEST_STRING);
  str_t *chopped = str_lchop(string, 26);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), DIGITS);
}

TEST_F(StrTest, LChopStatic) {
  string = str_wrap(TEST_STRING);
  str_t *chopped = str_lchop(string, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, LChopNull) {
  str_t *chopped = str_lchop(NULL, 10);
  ASSERT_FALSE(chopped);
}

TEST_F(StrTest, LChopNullStr) {
  string = str_wrap(NULL);
  str_t *chopped = str_lchop(string, 10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_TRUE(str_is_null(string));
}

TEST_F(StrTest, LChopZero) {
  string = str(TEST_STRING);
  str_t *chopped = str_lchop(string, 0);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, LChopStrlen) {
  string = str(TEST_STRING);
  str_t *chopped = str_lchop(string, TEST_STRING_LEN);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), "");
}

TEST_F(StrTest, LChopNegative) {
  string = str(TEST_STRING);
  str_t *chopped = str_lchop(string, -10);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, LChopLarge) {
  string = str(TEST_STRING);
  str_t *chopped = str_lchop(string, 100);
  ASSERT_TRUE(chopped);
  ASSERT_EQ(string, chopped);
  ASSERT_STREQ(str_chars(string), "");
}

/* -- str_erase ---------------------------------------------------------- */

TEST_F(StrTest, Erase) {
  string = str(TEST_STRING);
  str_t *erased = str_erase(string);
  ASSERT_TRUE(erased);
  ASSERT_EQ(string, erased);
  ASSERT_STREQ(str_chars(string), "");
}

TEST_F(StrTest, EraseStatic) {
  string = str_wrap(TEST_STRING);
  str_t *erased = str_erase(string);
  ASSERT_FALSE(erased);
}

TEST_F(StrTest, EraseNull) {
  str_t *erased = str_erase(NULL);
  ASSERT_FALSE(erased);
}

TEST_F(StrTest, EraseNullStr) {
  string = str_wrap(NULL);
  str_t *erased = str_erase(string);
  ASSERT_TRUE(erased);
  ASSERT_EQ(string, erased);
  ASSERT_TRUE(str_is_null(string));
}

/* -- str_set ------------------------------------------------------------ */

TEST_F(StrTest, Set) {
  string = str(TEST_STRING);
  str_t *mutated = str_set(string, 5, 'Q');
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING_MUTATION_5);
}

TEST_F(StrTest, SetZero) {
  string = str(TEST_STRING);
  str_t *mutated = str_set(string, 0, 'Q');
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING_MUTATION_0);
}

TEST_F(StrTest, SetStrlenMinusOne) {
  string = str(TEST_STRING);
  str_t *mutated = str_set(string, strlen(TEST_STRING) - 1, 'Q');
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING_MUTATION_35);
}

TEST_F(StrTest, SetStrlen) {
  string = str(TEST_STRING);
  str_t *mutated = str_set(string, strlen(TEST_STRING), 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetNull) {
  str_t *mutated = str_set(NULL, 5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetNullStr) {
  string = str_wrap(NULL);
  str_t *mutated = str_set(string, 5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetStatic) {
  string = str_wrap(ALPHABET);
  str_t *mutated = str_set(string, 5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetNegative) {
  string = str(TEST_STRING);
  str_t *mutated = str_set(string, -5, 'Q');
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, SetAfterEnd) {
  string = str(TEST_STRING);
  str_t *mutated = str_set(string, 100, 'Q');
  ASSERT_FALSE(mutated);
}

/* -- str_forcecase ------------------------------------------------------ */

TEST_F(StrTest, ForceCaseToUpper) {
  string = str(TEST_STRING_LOWER);
  str_t *mutated = str_forcecase(string, TRUE);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, ForceCaseToLower) {
  string = str(TEST_STRING);
  str_t *mutated = str_forcecase(string, FALSE);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING_LOWER);
}

TEST_F(StrTest, ForceCaseStatic) {
  string = str_wrap(TEST_STRING);
  str_t *mutated = str_forcecase(string, TRUE);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ForceCaseNull) {
  str_t *mutated = str_forcecase(NULL, TRUE);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ForceCaseNullStr) {
  string = str_wrap(NULL);
  str_t *mutated = str_forcecase(string, TRUE);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_TRUE(str_is_null(string));
}

/* -- str_toupper -------------------------------------------------------- */

TEST_F(StrTest, ToUpper) {
  string = str(TEST_STRING_LOWER);
  str_t *mutated = str_toupper(string);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, ToUpperStatic) {
  string = str_wrap(TEST_STRING);
  str_t *mutated = str_toupper(string);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToUpperNull) {
  str_t *mutated = str_toupper(NULL);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToUpperNullStr) {
  string = str_wrap(NULL);
  str_t *mutated = str_toupper(string);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_TRUE(str_is_null(string));
}

/* -- str_tolower -------------------------------------------------------- */

TEST_F(StrTest, ToLower) {
  string = str(TEST_STRING);
  str_t *mutated = str_tolower(string);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_STREQ(str_chars(string), TEST_STRING_LOWER);
}

TEST_F(StrTest, ToLowerStatic) {
  string = str_wrap(TEST_STRING);
  str_t *mutated = str_tolower(string);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToLowerNull) {
  str_t *mutated = str_tolower(NULL);
  ASSERT_FALSE(mutated);
}

TEST_F(StrTest, ToLowerNullStr) {
  string = str_wrap(NULL);
  str_t *mutated = str_tolower(string);
  ASSERT_TRUE(mutated);
  ASSERT_EQ(string, mutated);
  ASSERT_TRUE(str_is_null(string));
}

/* -- str_replace -------------------------------------------------------- */

TEST_F(StrTest, ReplaceOne) {
  string = str("The Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, "Pattern", "Brown", 0), 1);
  ASSERT_STREQ(str_chars(string), "The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceTwo) {
  string = str("Mr Pattern Chased The Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, "Pattern", "Brown", 0), 2);
  ASSERT_STREQ(str_chars(string), "Mr Brown Chased The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceAllNegativeMax) {
  string = str("Mr Pattern Chased Pattern Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, "Pattern", "Brown", -2), 3);
  ASSERT_STREQ(str_chars(string), "Mr Brown Chased Brown Quick Brown Fox");
}

TEST_F(StrTest, ReplaceOneOfTwo) {
  string = str("Mr Pattern Chased The Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, "Pattern", "Brown", 1), 1);
  ASSERT_STREQ(str_chars(string), "Mr Brown Chased The Quick Pattern Fox");
}

TEST_F(StrTest, ReplaceStart) {
  string = str("Pattern Chased The Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, "Pattern", "Brown", 0), 2);
  ASSERT_STREQ(str_chars(string), "Brown Chased The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceEnd) {
  string = str("Mr Pattern Chased The Quick Pattern Fox Pattern");
  ASSERT_EQ(str_replace(string, "Pattern", "Brown", 0), 3);
  ASSERT_STREQ(str_chars(string), "Mr Brown Chased The Quick Brown Fox Brown");
}

TEST_F(StrTest, ReplaceShorterWithLonger) {
  string = str("The Quick Brown Fox");
  ASSERT_EQ(str_replace(string, "The", "That", 0), 1);
  ASSERT_STREQ(str_chars(string), "That Quick Brown Fox");
}

TEST_F(StrTest, ReplaceShorterWithLongerAtEnd) {
  string = str("The Quick Brown Fox");
  ASSERT_EQ(str_replace(string, "Fox", "Foxes", 0), 1);
  ASSERT_STREQ(str_chars(string), "The Quick Brown Foxes");
}

TEST_F(StrTest, ReplaceRecursive) {
  string = str("The Quick Br Fox");
  ASSERT_EQ(str_replace(string, "Br", "Brown", 0), 1);
  ASSERT_STREQ(str_chars(string), "The Quick Brown Fox");
}

TEST_F(StrTest, ReplaceInNull) {
  ASSERT_EQ(str_replace(NULL, "Br", "Brown", 0), -1);
}

TEST_F(StrTest, ReplaceInStatic) {
  string = str_wrap("The Quick Br Fox");
  ASSERT_EQ(str_replace(string, "Br", "Brown", 0), -1);
}

TEST_F(StrTest, ReplaceInNullStr) {
  string = str_wrap(NULL);
  ASSERT_EQ(str_replace(string, "Br", "Brown", 0), 0);
}

TEST_F(StrTest, ReplaceNullPattern) {
  string = str("The Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, NULL, "Brown", 0), -1);
}

TEST_F(StrTest, ReplaceNullReplacement) {
  string = str("The Quick Pattern Fox");
  ASSERT_EQ(str_replace(string, "Pattern", NULL, 0), -1);
}
