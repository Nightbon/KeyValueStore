#include "model/parser.h"

#include <charconv>
#include <system_error>

namespace {

std::string ReadToken(const std::string& line, size_t& pos) {
  const bool quoted = line[pos] == '"';
  const size_t begin = quoted ? pos + 1 : pos;

  const auto is_delimiter = [&line, quoted](size_t index) {
    return quoted ? line[index] == '"'
                  : std::isspace(static_cast<unsigned char>(line[index])) != 0;
  };

  size_t end = begin;
  while (end < line.size() && !is_delimiter(end)) {
    ++end;
  }

  pos = quoted && end < line.size() ? end + 1 : end;
  return line.substr(begin, end - begin);
}

}  // namespace

namespace s21 {

std::vector<std::string> Tokenize(const std::string& line) {
  std::vector<std::string> tokens;
  size_t pos = 0;
  while (pos < line.size()) {
    if (std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
      ++pos;
    } else {
      tokens.push_back(ReadToken(line, pos));
    }
  }
  return tokens;
}

std::optional<int> ParseInt(std::string_view text) noexcept {
  int value = 0;
  const auto [end, code] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (code != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::pair<std::string, Student>> ParseRecord(
    const std::string& line) {
  const std::vector<std::string> tokens = Tokenize(line);
  static constexpr int tokens_in_line = 6;
  if (tokens.size() != tokens_in_line) {
    return std::nullopt;
  }

  const auto birth_year = ParseInt(tokens[3]);
  const auto coins = ParseInt(tokens[5]);
  if (!birth_year.has_value() || !coins.has_value()) {
    return std::nullopt;
  }

  return std::pair{
      tokens[0], Student{tokens[1], tokens[2], *birth_year, tokens[4], *coins}};
}

}  // namespace s21
