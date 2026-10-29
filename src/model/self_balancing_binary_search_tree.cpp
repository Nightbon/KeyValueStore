#include "model/self_balancing_binary_search_tree.h"

#include <algorithm>

namespace s21 {

Entry* SelfBalancingBinarySearchTree::FindLive(const std::string& key) {
  Node* node = FindNode(key);
  if (node == nullptr) {
    return nullptr;
  }
  if (node->entry.IsExpired(Now())) {
    Detach(key);
    return nullptr;
  }
  return &node->entry;
}

std::optional<Entry> SelfBalancingBinarySearchTree::Detach(
    const std::string& key) {
  Entry removed;
  if (!EraseAt(root_, key, removed)) {
    return std::nullopt;
  }
  return removed;
}

void SelfBalancingBinarySearchTree::Attach(const std::string& key,
                                           Entry entry) {
  InsertAt(root_, key, std::move(entry));
}

void SelfBalancingBinarySearchTree::ForEach(const Visitor& visit) {
  Walk(root_, visit);
}

SelfBalancingBinarySearchTree::Node* SelfBalancingBinarySearchTree::FindNode(
    const std::string& key) const {
  Node* node = root_.get();
  while (node != nullptr) {
    if (key < node->key) {
      node = node->left.get();
    } else if (node->key < key) {
      node = node->right.get();
    } else {
      return node;
    }
  }
  return nullptr;
}

void SelfBalancingBinarySearchTree::InsertAt(std::unique_ptr<Node>& node,
                                             const std::string& key,
                                             Entry&& entry) {
  if (!node) {
    node = std::make_unique<Node>(key, std::move(entry));
    ++size_;
    return;
  }
  if (key < node->key) {
    InsertAt(node->left, key, std::move(entry));
  } else if (node->key < key) {
    InsertAt(node->right, key, std::move(entry));
  } else {
    node->entry = std::move(entry);
    return;
  }
  Rebalance(node);
}

bool SelfBalancingBinarySearchTree::EraseAt(std::unique_ptr<Node>& node,
                                            const std::string& key,
                                            Entry& removed) {
  if (!node) {
    return false;
  }

  if (key < node->key || node->key < key) {
    std::unique_ptr<Node>& branch = key < node->key ? node->left : node->right;
    const bool erased = EraseAt(branch, key, removed);
    if (erased) {
      Rebalance(node);
    }
    return erased;
  }

  if (!node->left || !node->right) {
    removed = std::move(node->entry);
    node = std::move(node->left ? node->left : node->right);
    --size_;
    return true;
  }

  Node& successor = *MinOf(node->right.get());
  node->key = successor.key;
  std::swap(node->entry, successor.entry);
  EraseAt(node->right, node->key, removed);
  Rebalance(node);
  return true;
}

void SelfBalancingBinarySearchTree::Walk(const std::unique_ptr<Node>& node,
                                         const Visitor& visit) {
  if (!node) {
    return;
  }
  Walk(node->left, visit);
  visit(node->key, node->entry);
  Walk(node->right, visit);
}

int SelfBalancingBinarySearchTree::NodeHeight(const Node* node) noexcept {
  return node != nullptr ? node->height : 0;
}

int SelfBalancingBinarySearchTree::BalanceFactor(const Node& node) noexcept {
  return NodeHeight(node.left.get()) - NodeHeight(node.right.get());
}

void SelfBalancingBinarySearchTree::UpdateHeight(Node& node) noexcept {
  node.height =
      1 + std::max(NodeHeight(node.left.get()), NodeHeight(node.right.get()));
}

void SelfBalancingBinarySearchTree::Rebalance(std::unique_ptr<Node>& node) {
  UpdateHeight(*node);
  const int factor = BalanceFactor(*node);
  if (factor > 1) {
    if (BalanceFactor(*node->left) < 0) {
      RotateLeft(node->left);
    }
    RotateRight(node);
  } else if (factor < -1) {
    if (BalanceFactor(*node->right) > 0) {
      RotateRight(node->right);
    }
    RotateLeft(node);
  }
}

void SelfBalancingBinarySearchTree::RotateRight(std::unique_ptr<Node>& node) {
  std::unique_ptr<Node> pivot = std::move(node->left);
  node->left = std::move(pivot->right);
  UpdateHeight(*node);
  pivot->right = std::move(node);
  node = std::move(pivot);
  UpdateHeight(*node);
}

void SelfBalancingBinarySearchTree::RotateLeft(std::unique_ptr<Node>& node) {
  std::unique_ptr<Node> pivot = std::move(node->right);
  node->right = std::move(pivot->left);
  UpdateHeight(*node);
  pivot->left = std::move(node);
  node = std::move(pivot);
  UpdateHeight(*node);
}

SelfBalancingBinarySearchTree::Node* SelfBalancingBinarySearchTree::MinOf(
    Node* node) noexcept {
  while (node->left != nullptr) {
    node = node->left.get();
  }
  return node;
}

}  // namespace s21
