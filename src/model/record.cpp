#include "model/record.h"

#include <format>

namespace s21 {

bool Matches(const Student& student, const StudentFilter& filter) {
  const auto matches = [](const auto& expected, const auto& actual) {
    return !expected.has_value() || expected == actual;
  };

  return matches(filter.last_name, student.last_name) &&
         matches(filter.first_name, student.first_name) &&
         matches(filter.birth_year, student.birth_year) &&
         matches(filter.city, student.city) &&
         matches(filter.coins, student.coins);
}

void ApplyFilter(const StudentFilter& filter, Student& student) {
  const auto apply = [](const auto& expected, auto& actual) {
    if (expected.has_value()) {
      actual = *expected;
    }
  };

  apply(filter.last_name, student.last_name);
  apply(filter.first_name, student.first_name);
  apply(filter.birth_year, student.birth_year);
  apply(filter.city, student.city);
  apply(filter.coins, student.coins);
}

std::string FormatRecord(const std::string& key, const Student& student) {
  return std::format("{} \"{}\" \"{}\" {} \"{}\" {}", key, student.last_name,
                     student.first_name, student.birth_year, student.city,
                     student.coins);
}

}  // namespace s21
