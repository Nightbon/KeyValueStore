#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <optional>
#include <string>

#include "model/command_processor.h"

namespace s21 {

class Controller {
 public:
  bool SelectStore(int kind);
  [[nodiscard]] bool HasStore() const noexcept;
  [[nodiscard]] std::optional<Response> Execute(const std::string& line);

 private:
  CommandProcessor processor_;
};

}  // namespace s21

#endif  // CONTROLLER_H
