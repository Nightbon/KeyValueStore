#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <list>
#include <optional>
#include <string>
#include <vector>

#include "model/key_value_store.h"

namespace s21 {

class HashTable final : public KeyValueStore {
 public:
  HashTable() : buckets_(kInitialBucketCount) {}

  size_t Size() const noexcept override { return size_; }

 private:
  Entry* FindLive(const std::string& key) override;
  std::optional<Entry> Detach(const std::string& key) override;
  void Attach(const std::string& key, Entry entry) override;
  void ForEach(const Visitor& visit) override;

  using Node = std::pair<std::string, Entry>;
  using Bucket = std::list<Node>;

  void Rehash(size_t bucket_count);

  static constexpr double kMaxLoadFactor = 1.0;
  static constexpr size_t kInitialBucketCount = 8;

  std::vector<Bucket> buckets_;
  size_t size_ = 0;
};

}  // namespace s21

#endif  // HASH_TABLE_H
