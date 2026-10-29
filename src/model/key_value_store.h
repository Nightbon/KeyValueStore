#ifndef KEY_VALUE_STORE_H
#define KEY_VALUE_STORE_H

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "model/record.h"

namespace s21 {

using TimePoint = std::chrono::steady_clock::time_point;

struct Entry {
  Student value;
  std::optional<TimePoint> expires_at_;

  [[nodiscard]] bool IsExpired(TimePoint now) const noexcept;
  [[nodiscard]] int TimeToLive(TimePoint now) const noexcept;
};

class KeyValueStore {
 public:
  using Expiry = std::optional<std::chrono::seconds>;

  virtual ~KeyValueStore() = default;

  void Set(const std::string& key, Student value, Expiry ttl = std::nullopt);
  [[nodiscard]] std::optional<Student> Get(const std::string& key);
  [[nodiscard]] bool Exists(const std::string& key);
  [[nodiscard]] bool Del(const std::string& key);
  [[nodiscard]] bool Update(const std::string& key,
                            const StudentFilter& filter);
  [[nodiscard]] std::vector<std::string> Keys();
  [[nodiscard]] bool Rename(const std::string& from, const std::string& to);
  [[nodiscard]] std::optional<int> Ttl(const std::string& key);
  [[nodiscard]] std::vector<std::string> Find(const StudentFilter& filter);
  [[nodiscard]] std::vector<Student> ShowAll();
  [[nodiscard]] size_t Upload(const std::string& path);
  [[nodiscard]] size_t Export(const std::string& path);

 protected:
  using Visitor =
      std::function<void(const std::string& key, const Entry& entry)>;

  [[nodiscard]] virtual size_t Size() const noexcept = 0;
  [[nodiscard]] virtual Entry* FindLive(const std::string& key) = 0;
  [[nodiscard]] virtual std::optional<Entry> Detach(const std::string& key) = 0;
  virtual void Attach(const std::string& key, Entry entry) = 0;
  virtual void ForEach(const Visitor& visit) = 0;

  void PurgeExpired();
  [[nodiscard]] static std::optional<TimePoint> MakeExpiry(Expiry ttl) noexcept;
  [[nodiscard]] static TimePoint Now() noexcept {
    return std::chrono::steady_clock::now();
  }
};

}  // namespace s21

#endif  // KEY_VALUE_STORE_H
