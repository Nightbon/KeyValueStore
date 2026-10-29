#ifndef SELF_BALANCING_BINARY_SEARCH_TREE_H
#define SELF_BALANCING_BINARY_SEARCH_TREE_H

#include <optional>
#include <string>

#include "model/key_value_store.h"

namespace s21 {

class SelfBalancingBinarySearchTree final : public KeyValueStore {
 public:
  size_t Size() const noexcept override { return size_; }

  [[nodiscard]] int Height() const noexcept { return NodeHeight(root_.get()); }

 private:
  struct Node {
    std::string key;
    Entry entry;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    int height = 1;
  };

  Entry* FindLive(const std::string& key) override;
  std::optional<Entry> Detach(const std::string& key) override;
  void Attach(const std::string& key, Entry entry) override;
  void ForEach(const Visitor& visit) override;

  Node* FindNode(const std::string& key) const;
  void InsertAt(std::unique_ptr<Node>& node, const std::string& key,
                Entry&& entry);
  bool EraseAt(std::unique_ptr<Node>& node, const std::string& key,
               Entry& removed);
  static void Walk(const std::unique_ptr<Node>& node, const Visitor& visit);

  static int NodeHeight(const Node* node) noexcept;
  static int BalanceFactor(const Node& node) noexcept;
  static void UpdateHeight(Node& node) noexcept;
  static void Rebalance(std::unique_ptr<Node>& node);
  static void RotateLeft(std::unique_ptr<Node>& node);
  static void RotateRight(std::unique_ptr<Node>& node);
  static Node* MinOf(Node* node) noexcept;

  std::unique_ptr<Node> root_;
  size_t size_ = 0;
};

}  // namespace s21

#endif  // SELF_BALANCING_BINARY_SEARCH_TREE_H
