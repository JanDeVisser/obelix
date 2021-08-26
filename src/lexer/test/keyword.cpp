/*
 * This file is part of ${PROJECT} (https://github.com/JanDeVisser/${PROJECT}).
 *
 * (c) Copyright Jan de Visser 2014-2021
 *
 * Foobar is free software: you can redistribute it and/or modify
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
 * along withthis program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lexertest.h"


/* ----------------------------------------------------------------------- */

class KeywordTest : public LexerTest {
protected:
  void SetUp() override {
    LexerTest::SetUp();
    withScanners();
    EXPECT_EQ(dictionary_size(lexa->scanners), 3);
    lexa_build_lexer(lexa);
    EXPECT_TRUE(lexa);
  }
  
  unsigned int prepareWithBig() {
    scanner_config_t *kw_config;
    token_t          *token;
    unsigned int      ret;

    lexa_add_scanner(lexa, "keyword: keyword=Big");
    lexa_build_lexer(lexa);
    kw_config = lexa_get_scanner(lexa, "keyword");
    EXPECT_TRUE(kw_config);
    token = (token_t *) data_get_attribute((data_t *) kw_config, "Big");
    EXPECT_TRUE(token);
    ret = token_code(token);
    data_set_free(token);
    return ret;
  }

  void prepareWithBigBad(unsigned int *big, unsigned int *bad) {
    scanner_config_t *kw_config;
    token_t          *token;

    lexa_add_scanner(lexa, "keyword: keyword=Big;keyword=Bad");
    lexa_build_lexer(lexa);
    kw_config = lexa_get_scanner(lexa, "keyword");
    EXPECT_TRUE(kw_config);
    token = (token_t *) data_get_attribute((data_t *) kw_config, "Big");
    EXPECT_TRUE(token);
    *big = token_code(token);
    data_set_free(token);
    token = (token_t *) data_get_attribute((data_t *) kw_config, "Bad");
    EXPECT_TRUE(token);
    *bad = token_code(token);
    data_set_free(token);
  }

  unsigned int prepareWithAbc() {
    scanner_config_t *kw_config;
    token_t          *token;
    unsigned int      ret;
    str_t            *kw;
    int               ix;
    int_t            *size;
    constexpr static const char *_keywords[] = {
      "abb",
      "aca",
      "aba",
      "aaa",
      "aab",
      "abc",
      "aac",
      "acc",
      "acb",
      NULL
    };

    lexa_add_scanner(lexa, "keyword");
    lexa_build_lexer(lexa);
    kw_config = lexa_get_scanner(lexa, "keyword");
    EXPECT_TRUE(kw_config);

    for (ix = 0; _keywords[ix]; ix++) {
      kw = str_wrap(_keywords[ix]);
      scanner_config_setvalue(kw_config, "keyword", (data_t *) kw);
      data_set_free(kw);
    }

    token = (token_t *) data_get_attribute((data_t *) kw_config, "abc");
    EXPECT_TRUE(token);
    ret = token_code(token);
    data_set_free(token);
    EXPECT_TRUE(ret);

    size = (int_t *) data_get_attribute((data_t *) kw_config, "num_keywords");
    EXPECT_EQ(data_intval((data_t *) size), ix);

    return ret;
  }

  void tokenize(const char *s, int total_count, int big_count) {
    unsigned int code;

    code = prepareWithBig();
    lexa_set_stream(lexa, (data_t *) str(s));
    lexa_tokenize(lexa);
    EXPECT_EQ(lexa -> tokens, total_count);
    EXPECT_EQ(lexa_tokens_with_code(lexa, code), big_count);
  }

  void tokenizeBigBad(const char *s, int total_count, int big_count, int bad_count) {
    unsigned int big;
    unsigned int bad;

    prepareWithBigBad(&big, &bad);
    lexa_set_stream(lexa, (data_t *) str(s));
    lexa_tokenize(lexa);
    EXPECT_EQ(lexa -> tokens, total_count);
    EXPECT_EQ(lexa_tokens_with_code(lexa, big), big_count);
    EXPECT_EQ(lexa_tokens_with_code(lexa, bad), bad_count);
  }

};


TEST_F(KeywordTest, Keyword) {
  tokenize("Big", 2, 1);
}

TEST_F(KeywordTest, KeywordSpace) {
  tokenize("Big ", 3, 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
}

TEST_F(KeywordTest, KeywordIsPrefix) {
  tokenize("Bigger", 2, 0);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 1);
}

TEST_F(KeywordTest, KeywordAndIdentifiers) {
  tokenize("Hello Big World", 6, 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
}

TEST_F(KeywordTest, TwoKeywords) {
  tokenize("Hello Big Big Beautiful World", 10, 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 4);
}

TEST_F(KeywordTest, keyword_two_keywords_separated) {
  tokenize("Hello Big Beautiful Big World", 10, 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 4);
}

TEST_F(KeywordTest, keyword_big_bad_big) {
  tokenizeBigBad("Hello Big World", 6, 1, 0);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
}

TEST_F(KeywordTest, keyword_big_bad_bad) {
  tokenizeBigBad("Hello Bad World", 6, 0, 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
}

TEST_F(KeywordTest, keyword_big_bad_big_bad) {
  tokenizeBigBad("Hello Big Bad World", 8, 1, 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 3);
}

TEST_F(KeywordTest, keyword_big_bad_bad_big) {
  tokenizeBigBad("Hello Bad Big World", 8, 1, 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 3);
}

TEST_F(KeywordTest, keyword_abc) {
  unsigned int abc = prepareWithAbc();

  lexa_set_stream(lexa, (data_t *) str("yyz abc ams"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, abc), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
}
