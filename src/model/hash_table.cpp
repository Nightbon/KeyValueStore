#include "model/hash_table.h"

#include <algorithm>
#include <ranges>

namespace {
size_t BucketIndex(const std::string& key, size_t bucket_count) noexcept {
  return std::hash<std::string>{}(key) % bucket_count;
}
}  // namespace

namespace s21 {

Entry* HashTable::FindLive(const std::string& key) {
  Bucket& bucket = buckets_[BucketIndex(key, buckets_.size())];
  const auto it = std::ranges::find(bucket, key, &Node::first);
  if (it == bucket.end()) {
    return nullptr;
  }
  if (it->second.IsExpired(Now())) {
    bucket.erase(it);
    --size_;
    return nullptr;
  }
  return &it->second;
}

std::optional<Entry> HashTable::Detach(const std::string& key) {
  Bucket& bucket = buckets_[BucketIndex(key, buckets_.size())];
  const auto it = std::ranges::find(bucket, key, &Node::first);
  if (it == bucket.end()) {
    return std::nullopt;
  }

  Entry entry = std::move(it->second);
  bucket.erase(it);
  --size_;

  if (entry.IsExpired(Now())) {
    return std::nullopt;
  }
  return entry;
}

void HashTable::Attach(const std::string& key, Entry entry) {
  if (static_cast<double>(size_ + 1) > kMaxLoadFactor * buckets_.size()) {
    Rehash(buckets_.size() * 2);
  }
  buckets_[BucketIndex(key, buckets_.size())].emplace_back(key,
                                                           std::move(entry));
  ++size_;
}

void HashTable::ForEach(const Visitor& visit) {
  std::ranges::for_each(
      buckets_ | std::views::join,
      [&visit](const Node& node) { visit(node.first, node.second); });
}

void HashTable::Rehash(size_t bucket_count) {
  std::vector<Bucket> fresh(bucket_count);
  for (auto& [key, value] : buckets_ | std::views::join) {
    fresh[BucketIndex(key, bucket_count)].emplace_back(std::move(key),
                                                       std::move(value));
  }

  buckets_ = std::move(fresh);
}

}  // namespace s21
