// Тесты, специфичные для AVL-дерева: упорядоченность и сбалансированность.

#include "model/self_balancing_binary_search_tree.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

#include "test_helpers.h"

namespace {

int MaxAvlHeight(std::size_t node_count) {
  // Классическая верхняя граница высоты AVL-дерева.
  const double bound =
      1.4405 * std::log2(static_cast<double>(node_count) + 2.0) - 0.3277;
  return static_cast<int>(bound) + 1;
}

TEST(SelfBalancingBinarySearchTreeTest, EmptyTreeBasics) {
  s21::SelfBalancingBinarySearchTree store;
  EXPECT_EQ(store.Size(), 0u);
  EXPECT_EQ(store.Height(), 0);
  EXPECT_TRUE(store.Keys().empty());
  EXPECT_TRUE(store.ShowAll().empty());
  EXPECT_EQ(store.Get("foo"), std::nullopt);
  EXPECT_FALSE(store.Exists("foo"));
  EXPECT_FALSE(store.Del("foo"));
}

TEST(SelfBalancingBinarySearchTreeTest, KeysAreSortedInOrder) {
  s21::SelfBalancingBinarySearchTree store;
  const std::vector<std::string> keys = {"foo", "boo", "bar",
                                         "abc", "zzz", "key"};
  for (const std::string& key : keys) {
    store.Set(key, tests::MakeStudent("Last", "First", 2000, "City", 1));
  }

  const std::vector<std::string> sorted = store.Keys();
  EXPECT_EQ(sorted.size(), keys.size());
  EXPECT_TRUE(std::ranges::is_sorted(sorted));
}

TEST(SelfBalancingBinarySearchTreeTest, StaysBalancedOnSequentialInserts) {
  s21::SelfBalancingBinarySearchTree store;
  for (int i = 0; i < 1024; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }
  EXPECT_EQ(store.Size(), 1024u);
  EXPECT_LE(store.Height(), MaxAvlHeight(1024));
  for (int i = 0; i < 1024; i += 97) {
    EXPECT_TRUE(store.Exists(std::to_string(i)));
  }
}

TEST(SelfBalancingBinarySearchTreeTest, StaysBalancedOnDeletes) {
  s21::SelfBalancingBinarySearchTree store;
  for (int i = 0; i < 512; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }
  for (int i = 0; i < 512; i += 2) {
    EXPECT_TRUE(store.Del(std::to_string(i)));
  }
  EXPECT_EQ(store.Size(), 256u);
  EXPECT_LE(store.Height(), MaxAvlHeight(256));
  EXPECT_TRUE(std::ranges::is_sorted(store.Keys()));
}

TEST(SelfBalancingBinarySearchTreeTest, EraseHandlesTwoChildrenCase) {
  s21::SelfBalancingBinarySearchTree store;
  for (int i = 1; i <= 7; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }

  EXPECT_TRUE(store.Del("4"));  // узел с двумя потомками
  EXPECT_TRUE(store.Del("2"));
  EXPECT_TRUE(store.Del("7"));

  EXPECT_EQ(store.Size(), 4u);
  EXPECT_EQ(store.Get("4"), std::nullopt);
  EXPECT_TRUE(std::ranges::is_sorted(store.Keys()));

  for (const std::string& key : store.Keys()) {
    EXPECT_TRUE(store.Exists(key));
  }
}

TEST(SelfBalancingBinarySearchTreeTest, ExpiryRebalanceAfterPurge) {
  s21::SelfBalancingBinarySearchTree store;
  for (int i = 0; i < 64; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i),
              std::chrono::seconds{1});
  }
  std::this_thread::sleep_for(std::chrono::milliseconds{1150});

  EXPECT_EQ(store.Keys().size(), 0u);
  EXPECT_EQ(store.Size(), 0u);
  EXPECT_EQ(store.Height(), 0);

  store.Set("foo", tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  EXPECT_EQ(store.Height(), 1);
}

}  // namespace
