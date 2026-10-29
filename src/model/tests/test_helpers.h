#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <gtest/gtest.h>

#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "model/record.h"

namespace tests {

inline std::string WriteTempFile(const std::string& name,
                                 const std::string& content) {
  const std::string path = ::testing::TempDir() + name;
  std::ofstream file(path);
  file << content;
  return path;
}

inline std::string ReadFile(const std::string& path) {
  std::ifstream file(path);
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

inline std::vector<std::string> SplitLines(const std::string& text) {
  std::vector<std::string> lines;
  for (std::size_t begin = 0; begin < text.size();) {
    const std::size_t end = text.find('\n', begin);
    lines.push_back(
        text.substr(begin, end == std::string::npos ? end : end - begin));
    begin = end == std::string::npos ? text.size() : end + 1;
  }
  return lines;
}

inline s21::Student MakeStudent(std::string last_name, std::string first_name,
                                int birth_year, std::string city, int coins) {
  return s21::Student{std::move(last_name), std::move(first_name), birth_year,
                      std::move(city), coins};
}

inline void PrintTo(const s21::Student& student, std::ostream* os) {
  *os << std::format("(\"{}\" \"{}\" {} \"{}\" {})", student.last_name,
                     student.first_name, student.birth_year, student.city,
                     student.coins);
}

}  // namespace tests

#endif  // TEST_HELPERS_H
