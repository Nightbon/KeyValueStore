#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <optional>
#include <string>

#include "model/key_value_store.h"
#include "model/response.h"

namespace s21 {

class CommandProcessor {
 public:
  CommandProcessor() = default;
  ~CommandProcessor() = default;

  explicit CommandProcessor(std::unique_ptr<KeyValueStore> store) noexcept;

  bool SelectStore(int kind);
  [[nodiscard]] bool HasStore() const noexcept { return store_ != nullptr; }
  [[nodiscard]] std::optional<Response> Execute(const std::string& line);

 private:
  std::unique_ptr<KeyValueStore> store_;
};

}  // namespace s21

#endif  // COMMAND_PROCESSOR_H
