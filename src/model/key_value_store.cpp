#include "model/key_value_store.h"

#include <algorithm>
#include <fstream>
#include <ranges>
#include <stdexcept>

#include "model/parser.h"

namespace s21 {

bool Entry::IsExpired(TimePoint now) const noexcept {
  return expires_at_.has_value() && *expires_at_ <= now;
}

int Entry::TimeToLive(TimePoint now) const noexcept {
  if (!expires_at_.has_value()) {
    return -1;
  }
  const auto left = std::chrono::ceil<std::chrono::seconds>(*expires_at_ - now);
  return static_cast<int>(left.count());
}

std::optional<TimePoint> KeyValueStore::MakeExpiry(Expiry ttl) noexcept {
  if (!ttl.has_value()) {
    return std::nullopt;
  }
  return Now() + *ttl;
}

void KeyValueStore::Set(const std::string& key, Student value, Expiry ttl) {
  if (Entry* entry = FindLive(key)) {
    entry->value = std::move(value);
    entry->expires_at_ = MakeExpiry(ttl);
    return;
  }
  Attach(key, Entry{std::move(value), MakeExpiry(ttl)});
}

std::optional<Student> KeyValueStore::Get(const std::string& key) {
  if (Entry* entry = FindLive(key)) {
    return entry->value;
  }
  return std::nullopt;
}

bool KeyValueStore::Exists(const std::string& key) {
  return FindLive(key) != nullptr;
}

bool KeyValueStore::Del(const std::string& key) {
  return Detach(key).has_value();
}

bool KeyValueStore::Update(const std::string& key,
                           const StudentFilter& filter) {
  if (Entry* entry = FindLive(key)) {
    ApplyFilter(filter, entry->value);
    return true;
  }
  return false;
}

std::vector<std::string> KeyValueStore::Keys() {
  PurgeExpired();
  std::vector<std::string> keys;
  ForEach(
      [&keys](const std::string& key, const Entry&) { keys.push_back(key); });
  return keys;
}

bool KeyValueStore::Rename(const std::string& from, const std::string& to) {
  std::optional<Entry> entry = Detach(from);
  if (!entry.has_value()) {
    return false;
  }

  Detach(to);
  Attach(to, std::move(*entry));
  return true;
}

std::optional<int> KeyValueStore::Ttl(const std::string& key) {
  if (Entry* entry = FindLive(key)) {
    return entry->TimeToLive(Now());
  }
  return std::nullopt;
}

std::vector<std::string> KeyValueStore::Find(const StudentFilter& filter) {
  PurgeExpired();
  std::vector<std::string> keys;
  ForEach([&](const std::string& key, const Entry& entry) {
    if (Matches(entry.value, filter)) {
      keys.push_back(key);
    }
  });
  return keys;
}

std::vector<Student> KeyValueStore::ShowAll() {
  PurgeExpired();
  std::vector<Student> students;
  ForEach([&students](const std::string&, const Entry& entry) {
    students.push_back(entry.value);
  });
  return students;
}

void KeyValueStore::PurgeExpired() {
  std::vector<std::string> expired;
  ForEach([&](const std::string& key, const Entry& entry) {
    if (entry.IsExpired(Now())) {
      expired.push_back(key);
    }
  });
  std::ranges::for_each(
      expired, [this](const std::string& key) { static_cast<void>(Del(key)); });
}

size_t KeyValueStore::Upload(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("unable to open file \"" + path + "\"");
  }

  size_t count = 0;
  for (std::string line; std::getline(file, line);) {
    if (auto record = ParseRecord(line)) {
      auto& [key, student] = *record;
      Set(key, std::move(student));
      ++count;
    }
  }
  return count;
}

size_t KeyValueStore::Export(const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("unable to open file \"" + path + "\"");
  }

  auto lines = Keys() | std::views::filter([this](const auto& key) {
                 return Get(key).has_value();
               }) |
               std::views::transform([this](const auto& key) {
                 return FormatRecord(key, *Get(key));
               });

  size_t count = 0;
  for (const auto& line : lines) {
    file << line << "\n";
    ++count;
  }

  return count;
}

}  // namespace s21
