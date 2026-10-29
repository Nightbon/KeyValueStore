#ifndef VIEW_H
#define VIEW_H

#include <string>

#include "controller/controller.h"
#include "model/response.h"

namespace s21 {

class View {
 public:
  explicit View(Controller& controller);

  void Run();

 private:
  void SelectStore();
  void ReadEvalPrint();
  void Render(const Response& response) const;

  Controller& controller_;
};

}  // namespace s21

#endif  // VIEW_H
