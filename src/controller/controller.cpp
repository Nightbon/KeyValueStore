#include "controller/controller.h"

namespace s21 {

bool Controller::SelectStore(int kind) { return processor_.SelectStore(kind); }

bool Controller::HasStore() const noexcept { return processor_.HasStore(); }

std::optional<Response> Controller::Execute(const std::string& line) {
  return processor_.Execute(line);
}

}  // namespace s21
