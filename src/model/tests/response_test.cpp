// Тесты протокола ответов модели: сравнение по умолчанию для всех альтернатив.

#include "model/response.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "test_helpers.h"

namespace {

using s21::Response;

TEST(ResponseTest, EqualAlternativesCompareEqual) {
  EXPECT_EQ((Response{s21::Ok{}}), (Response{s21::Ok{}}));
  EXPECT_EQ((Response{s21::Flag{true}}), (Response{s21::Flag{true}}));
  EXPECT_EQ((Response{s21::MaybeRecord{std::nullopt}}),
            (Response{s21::MaybeRecord{std::nullopt}}));
  EXPECT_EQ(
      (Response{s21::MaybeRecord{tests::MakeStudent("A", "B", 2000, "C", 1)}}),
      (Response{s21::MaybeRecord{tests::MakeStudent("A", "B", 2000, "C", 1)}}));
  EXPECT_EQ((Response{s21::MaybeTtl{30}}), (Response{s21::MaybeTtl{30}}));
  EXPECT_EQ((Response{s21::KeyList{{"a", "b"}}}),
            (Response{s21::KeyList{{"a", "b"}}}));
  EXPECT_EQ((Response{s21::StudentTable{
                {tests::MakeStudent("A", "B", 2000, "C", 1)}}}),
            (Response{s21::StudentTable{
                {tests::MakeStudent("A", "B", 2000, "C", 1)}}}));
  EXPECT_EQ((Response{s21::Count{2}}), (Response{s21::Count{2}}));
  EXPECT_EQ((Response{s21::Error{"oops"}}), (Response{s21::Error{"oops"}}));
}

TEST(ResponseTest, DifferentValuesOrAlternativesCompareNotEqual) {
  EXPECT_NE((Response{s21::Flag{true}}), (Response{s21::Flag{false}}));
  EXPECT_NE((Response{s21::MaybeTtl{std::nullopt}}),
            (Response{s21::MaybeTtl{30}}));
  EXPECT_NE((Response{s21::KeyList{{"a"}}}), (Response{s21::KeyList{{"b"}}}));
  EXPECT_NE((Response{s21::Count{2}}), (Response{s21::Count{3}}));
  EXPECT_NE((Response{s21::Error{"oops"}}), (Response{s21::Error{"oh no"}}));
  EXPECT_NE((Response{s21::Ok{}}), (Response{s21::Flag{true}}));
  EXPECT_NE((Response{s21::Count{1}}), (Response{s21::Error{"1"}}));
}

}  // namespace
