// Тесты разбора текста: токенизатор, приведение к int, формат файла данных.

#include "model/parser.h"

#include <gtest/gtest.h>

#include <string>

#include "test_helpers.h"

namespace {

// ─── Tokenize ───────────────────────────────────────────────────────────────

TEST(ParserTest, TokenizePlainLine) {
  EXPECT_EQ(s21::Tokenize("SET foo Vasilev Ivan 2000 Moscow 55"),
            (std::vector<std::string>{"SET", "foo", "Vasilev", "Ivan", "2000",
                                      "Moscow", "55"}));
}

TEST(ParserTest, TokenizeExtraSpaces) {
  EXPECT_EQ(s21::Tokenize("  GET   foo  "),
            (std::vector<std::string>{"GET", "foo"}));
}

TEST(ParserTest, TokenizeEmptyLine) {
  EXPECT_TRUE(s21::Tokenize("").empty());
  EXPECT_TRUE(s21::Tokenize("   \t ").empty());
}

TEST(ParserTest, TokenizeRemovesQuotesAndGluesWords) {
  EXPECT_EQ(s21::Tokenize("key1 \"Vasilev\" \"Ivan\" 2001 \"New York\" 55"),
            (std::vector<std::string>{"key1", "Vasilev", "Ivan", "2001",
                                      "New York", "55"}));
}

TEST(ParserTest, TokenizeUnterminatedQuoteReadsTillEnd) {
  EXPECT_EQ(s21::Tokenize("\"Moscow"), (std::vector<std::string>{"Moscow"}));
}

// ─── ParseInt ───────────────────────────────────────────────────────────────

TEST(ParserTest, ParseIntValidValues) {
  EXPECT_EQ(s21::ParseInt("2000"), 2000);
  EXPECT_EQ(s21::ParseInt("-42"), -42);
  EXPECT_EQ(s21::ParseInt("0"), 0);
}

TEST(ParserTest, ParseIntRejectsGarbage) {
  EXPECT_EQ(s21::ParseInt("aaaaa"), std::nullopt);
  EXPECT_EQ(s21::ParseInt("12abc"), std::nullopt);
  EXPECT_EQ(s21::ParseInt(""), std::nullopt);
  EXPECT_EQ(s21::ParseInt("--5"), std::nullopt);
  EXPECT_EQ(s21::ParseInt("99999999999999999999"), std::nullopt);
}

// ─── ParseRecord / FormatRecord ─────────────────────────────────────────────

TEST(ParserTest, ParseRecordValidLine) {
  const auto record =
      s21::ParseRecord("key1 \"Vasilev\" \"Ivan\" 2001 \"Rostov\" 55");
  ASSERT_TRUE(record.has_value());
  EXPECT_EQ(record->first, "key1");
  EXPECT_EQ(record->second,
            tests::MakeStudent("Vasilev", "Ivan", 2001, "Rostov", 55));
}

TEST(ParserTest, ParseRecordRejectsBrokenLines) {
  EXPECT_EQ(s21::ParseRecord("key1 \"Vasilev\" \"Ivan\" 2001"), std::nullopt);
  EXPECT_EQ(s21::ParseRecord("key1 \"Vasilev\" \"Ivan\" abcd \"Rostov\" 55"),
            std::nullopt);
  EXPECT_EQ(s21::ParseRecord(""), std::nullopt);
}

TEST(ParserTest, FormatRecordProducesFileLine) {
  EXPECT_EQ(s21::FormatRecord("key1", tests::MakeStudent("Vasilev", "Ivan",
                                                         2001, "Rostov", 55)),
            "key1 \"Vasilev\" \"Ivan\" 2001 \"Rostov\" 55");
}

TEST(ParserTest, RecordFormatRoundTrip) {
  const s21::Student student =
      tests::MakeStudent("Sidorov", "Sergei", 1847, "Suzdal", 12312313);
  const auto parsed = s21::ParseRecord(s21::FormatRecord("key101", student));
  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(parsed->first, "key101");
  EXPECT_EQ(parsed->second, student);
}

}  // namespace
