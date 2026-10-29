#ifndef PARSER_H
#define PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "model/record.h"

namespace s21 {

std::vector<std::string> Tokenize(const std::string& line);
std::optional<int> ParseInt(std::string_view text) noexcept;
std::optional<std::pair<std::string, Student>> ParseRecord(
    const std::string& line);

}  // namespace s21

#endif  // PARSER_H
