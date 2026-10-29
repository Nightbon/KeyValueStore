#ifndef RECORD_H
#define RECORD_H

#include <optional>
#include <string>

namespace s21 {

struct Student {
  std::string last_name;
  std::string first_name;
  int birth_year = 0;
  std::string city;
  int coins = 0;

  bool operator==(const Student&) const = default;
};

struct StudentFilter {
  std::optional<std::string> last_name;
  std::optional<std::string> first_name;
  std::optional<int> birth_year;
  std::optional<std::string> city;
  std::optional<int> coins;
};

bool Matches(const Student& student, const StudentFilter& filter);
void ApplyFilter(const StudentFilter& filter, Student& student);

std::string FormatRecord(const std::string& key, const Student& student);

}  // namespace s21

#endif  // RECORD_H
