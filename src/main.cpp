#include "controller/controller.h"
#include "view/view.h"

int main() {
  s21::Controller controller;
  s21::View view(controller);
  view.Run();
  return 0;
}
