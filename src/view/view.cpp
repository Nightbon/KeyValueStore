#include "view/view.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "model/parser.h"

namespace s21 {
namespace {

std::string StudentToLine(const Student& student) {
  return std::format("{} {} {} {} {}", student.last_name, student.first_name,
                     student.birth_year, student.city, student.coins);
}

void PrintNumbered(const std::vector<std::string>& items) {
  std::size_t number = 1;
  std::ranges::for_each(items, [&number](const std::string& item) {
    std::cout << number++ << ") " << item << "\n";
  });
}

std::size_t GetVisualLength(const std::string& s) {
  return std::ranges::count_if(
      s, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; });
}

void PrintTable(const std::vector<Student>& students) {
  if (students.empty()) {
    std::cout << "(empty)\n";
    return;
  }

  std::size_t w_num = GetVisualLength("№");
  std::size_t w_last = GetVisualLength("Last name");
  std::size_t w_first = GetVisualLength("First name");
  std::size_t w_year = GetVisualLength("Year");
  std::size_t w_city = GetVisualLength("City");
  std::size_t w_coins = GetVisualLength("Number of coins");

  w_num = std::max(w_num, std::to_string(students.size()).length());

  for (const auto& s : students) {
    w_last = std::max(w_last, GetVisualLength(s.last_name));
    w_first = std::max(w_first, GetVisualLength(s.first_name));
    w_year = std::max(w_year, std::to_string(s.birth_year).length());
    w_city = std::max(w_city, GetVisualLength(s.city));
    w_coins = std::max(w_coins, std::to_string(s.coins).length());
  }

  std::cout << std::format(
      "{:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} |\n", "№", w_num,
      "Last name", w_last, "First name", w_first, "Year", w_year, "City",
      w_city, "Number of coins", w_coins);

  std::cout << std::string(
                   w_num + w_last + w_first + w_year + w_city + w_coins + 17,
                   '-')
            << '\n';

  std::size_t number = 1;
  std::ranges::for_each(students, [&](const Student& student) {
    std::cout << std::format(
        "{:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} | {:<{}} |\n", number++,
        w_num, std::format("{}", student.last_name), w_last,
        std::format("{}", student.first_name), w_first, student.birth_year,
        w_year, std::format("{}", student.city), w_city, student.coins,
        w_coins);
  });
}

}  // namespace

View::View(Controller& controller) : controller_(controller) {}

void View::Run() {
  SelectStore();
  ReadEvalPrint();
}

void View::SelectStore() {
  while (!controller_.HasStore()) {
    std::cout.flush();
    std::cout << "Select the storage type:\n"
                 "  1 - hash table\n"
                 "  2 - self-balancing binary search tree\n> ";
    std::cout.flush();
    int choice = 0;
    std::cin >> choice;
    if (!std::cin >> choice || !controller_.SelectStore(choice)) {
      std::cout << "\033[31m" << "ERROR: invalid choice\n"
                << "\033[0m";
    }
  }
}

void View::ReadEvalPrint() {
  for (std::string line;;) {
    std::cout << "> ";
    std::cout.flush();
    if (!std::getline(std::cin, line)) {
      break;
    }
    if (const std::optional<Response> response = controller_.Execute(line)) {
      Render(*response);
    }
  }
}

template <typename... Alternatives>
struct Overloaded : Alternatives... {
  using Alternatives::operator()...;
};

void View::Render(const Response& response) const {
  std::visit(Overloaded{
                 [](const Ok&) {
                   std::cout << "\033[32m" << "OK\n"
                             << "\033[0m";
                 },
                 [](const Flag& flag) {
                   std::cout << (flag.value ? "true" : "false") << "\n";
                 },
                 [](const MaybeRecord& record) {
                   std::cout << (record.value.has_value()
                                     ? StudentToLine(*record.value)
                                     : std::string("(null)"))
                             << "\n";
                 },
                 [](const MaybeTtl& ttl) {
                   std::cout << (ttl.seconds.has_value()
                                     ? std::format("{}", *ttl.seconds)
                                     : std::string("(null)"))
                             << "\n";
                 },
                 [](const KeyList& keys) { PrintNumbered(keys.keys); },
                 [](const StudentTable& table) { PrintTable(table.students); },
                 [](const Count& count) {
                   std::cout << "\033[32m" << "OK " << "\033[0m" << count.value
                             << "\n";
                 },
                 [](const Error& error) {
                   std::cout << "\033[31m" << "ERROR: " << error.message << "\n"
                             << "\033[0m";
                 },
             },
             response);
}

}  // namespace s21
