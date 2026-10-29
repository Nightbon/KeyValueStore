#include "model/command_processor.h"

#include <chrono>
#include <cstdlib>
#include <format>
#include <memory>

#include "model/hash_table.h"
#include "model/parser.h"
#include "model/self_balancing_binary_search_tree.h"

namespace s21 {

namespace {

std::string CastError(const std::string& value) {
  return std::format("unable cast value \"{}\" to type int", value);
}

std::string KeyError(const std::string& key) {
  return std::format("key \"{}\" does not exist", key);
}

std::string WrongArity(const std::string& command) {
  return std::format("wrong number of arguments for \"{}\"", command);
}

std::string ExpandPath(const std::string& path) {
  if (path.empty() || path.front() != '~') {
    return path;
  }
  const char* home = std::getenv("HOME");
  return home != nullptr ? home + path.substr(1) : path;
}

struct FieldsOrError {
  std::optional<StudentFilter> fields;
  std::string error;
};

FieldsOrError ParseFields(const std::vector<std::string>& tokens,
                          std::size_t from, bool dash_is_empty) {
  const auto is_dash = [&](std::size_t index) {
    return dash_is_empty && tokens[index] == "-";
  };
  const auto text = [&](std::size_t index) -> std::optional<std::string> {
    return is_dash(index) ? std::nullopt : std::optional(tokens[index]);
  };
  const auto number = [&](std::size_t index) -> std::optional<int> {
    return is_dash(index) ? std::nullopt : ParseInt(tokens[index]);
  };

  const auto birth_year = number(from + 2);
  const auto coins = number(from + 4);
  if (!birth_year.has_value() && !is_dash(from + 2)) {
    return {.error = CastError(tokens[from + 2])};
  }
  if (!coins.has_value() && !is_dash(from + 4)) {
    return {.error = CastError(tokens[from + 4])};
  }

  return {.fields = StudentFilter{text(from), text(from + 1), birth_year,
                                  text(from + 3), coins}};
}

Student ToStudent(const StudentFilter& filter) {
  return {*filter.last_name, *filter.first_name, *filter.birth_year,
          *filter.city, *filter.coins};
}

Response ExecuteSet(KeyValueStore& store,
                    const std::vector<std::string>& tokens) {
  // SET должен иметь либо 7 либо 9 аргументов.
  if (tokens.size() != 7 && tokens.size() != 9) {
    return Error{WrongArity("SET")};
  }

  const FieldsOrError parsed = ParseFields(tokens, 2, false);
  if (!parsed.fields.has_value()) {
    return Error{parsed.error};
  }

  KeyValueStore::Expiry ttl;
  if (tokens.size() == 9) {
    if (tokens[7] != "EX") {
      return Error{"expected EX parameter"};
    }
    const std::optional<int> seconds = ParseInt(tokens[8]);
    if (!seconds.has_value() || *seconds < 0) {
      return Error{CastError(tokens[8])};
    }
    ttl = std::chrono::seconds{*seconds};
  }

  store.Set(tokens[1], ToStudent(*parsed.fields), ttl);
  return Ok{};
}

Response ExecuteUpdate(KeyValueStore& store,
                       const std::vector<std::string>& tokens) {
  if (tokens.size() != 7) {
    return Error{WrongArity("UPDATE")};
  }
  const FieldsOrError parsed = ParseFields(tokens, 2, true);
  if (!parsed.fields.has_value()) {
    return Error{parsed.error};
  }
  if (!store.Update(tokens[1], *parsed.fields)) {
    return Error{KeyError(tokens[1])};
  }
  return Ok{};
}

Response ExecuteFind(KeyValueStore& store,
                     const std::vector<std::string>& tokens) {
  if (tokens.size() != 6) {
    return Error{WrongArity("FIND")};
  }
  const FieldsOrError parsed = ParseFields(tokens, 1, true);
  if (!parsed.fields.has_value()) {
    return Error{parsed.error};
  }
  return KeyList{store.Find(*parsed.fields)};
}

Response ExecuteFile(KeyValueStore& store,
                     const std::vector<std::string>& tokens) {
  if (tokens.size() != 2) {
    return Error{WrongArity(tokens.front())};
  }
  try {
    const std::string path = ExpandPath(tokens[1]);
    return Count{tokens.front() == "UPLOAD" ? store.Upload(path)
                                            : store.Export(path)};
  } catch (const std::exception& error) {
    return Error{error.what()};
  }
}

constexpr uint32_t str_hash(std::string_view str) {
  uint32_t hash = 2166136261u;
  for (char c : str) {
    hash ^= static_cast<uint32_t>(c);
    hash *= 16777619u;
  }
  return hash;
}

}  // namespace

CommandProcessor::CommandProcessor(
    std::unique_ptr<KeyValueStore> store) noexcept
    : store_(std::move(store)) {}

bool CommandProcessor::SelectStore(int kind) {
  switch (kind) {
    case 1:
      store_ = std::make_unique<HashTable>();
      return true;
    case 2:
      store_ = std::make_unique<SelfBalancingBinarySearchTree>();
      return true;
    default:
      return false;
  }
}

std::optional<Response> CommandProcessor::Execute(const std::string& line) {
  const std::vector<std::string> tokens = Tokenize(line);
  if (tokens.empty()) {
    return std::nullopt;
  }
  if (store_ == nullptr) {
    return Response{Error{"storage is not selected"}};
  }

  const std::string& command = tokens.front();

  auto CheckArity = [&](size_t expected) -> std::optional<Response> {
    if (tokens.size() != expected) {
      return Response{Error{WrongArity(command)}};
    }
    return std::nullopt;
  };

  switch (str_hash(command)) {
    case str_hash("SET"):
      return ExecuteSet(*store_, tokens);
    case str_hash("UPDATE"):
      return ExecuteUpdate(*store_, tokens);
    case str_hash("FIND"):
      return ExecuteFind(*store_, tokens);
    case str_hash("UPLOAD"):
    case str_hash("EXPORT"):
      return ExecuteFile(*store_, tokens);
    case str_hash("GET"):
      if (auto err = CheckArity(2); err) return err;
      return MaybeRecord{store_->Get(tokens[1])};
    case str_hash("EXISTS"):
      if (auto err = CheckArity(2); err) return err;
      return Flag{store_->Exists(tokens[1])};
    case str_hash("DEL"):
      if (auto err = CheckArity(2); err) return err;
      return Flag{store_->Del(tokens[1])};
    case str_hash("TTL"):
      if (auto err = CheckArity(2); err) return err;
      return MaybeTtl{store_->Ttl(tokens[1])};
    case str_hash("RENAME"):
      if (auto err = CheckArity(3); err) return err;
      if (!store_->Rename(tokens[1], tokens[2]))
        return Error{KeyError(tokens[1])};
      return Ok{};
    case str_hash("KEYS"):
      if (auto err = CheckArity(1); err) return err;
      return KeyList{store_->Keys()};
    case str_hash("SHOWALL"):
      if (auto err = CheckArity(1); err) return err;
      return StudentTable{store_->ShowAll()};
    default:
      return Error{std::format("unknown command \"{}\"", command)};
  }
}

}  // namespace s21
