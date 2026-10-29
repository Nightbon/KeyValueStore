#ifndef RESPONSE_H
#define RESPONSE_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "model/record.h"

namespace s21 {

struct Ok {
  bool operator==(const Ok&) const = default;
};
struct Flag {
  bool value;
  bool operator==(const Flag&) const = default;
};
struct MaybeRecord {
  std::optional<Student> value;
  bool operator==(const MaybeRecord&) const = default;
};
struct MaybeTtl {
  std::optional<int> seconds;
  bool operator==(const MaybeTtl&) const = default;
};
struct KeyList {
  std::vector<std::string> keys;
  bool operator==(const KeyList&) const = default;
};
struct StudentTable {
  std::vector<Student> students;
  bool operator==(const StudentTable&) const = default;
};
struct Count {
  std::size_t value;
  bool operator==(const Count&) const = default;
};
struct Error {
  std::string message;
  bool operator==(const Error&) const = default;
};

using Response = std::variant<Ok, Flag, MaybeRecord, MaybeTtl, KeyList,
                              StudentTable, Count, Error>;

}  // namespace s21

#endif  // RESPONSE_H
