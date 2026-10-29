// Тесты, специфичные для хэш-таблицы: цепочки, рехэш, ленивое вытеснение.

#include "model/hash_table.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "test_helpers.h"

namespace {

TEST(HashTableTest, EmptyTableBasics) {
  s21::HashTable store;
  EXPECT_EQ(store.Size(), 0u);
  EXPECT_TRUE(store.Keys().empty());
  EXPECT_TRUE(store.ShowAll().empty());
  EXPECT_TRUE(store.Find(s21::StudentFilter{.last_name = "Vasilev"}).empty());
  EXPECT_EQ(store.Get("foo"), std::nullopt);
  EXPECT_FALSE(store.Exists("foo"));
  EXPECT_FALSE(store.Del("foo"));
  EXPECT_EQ(store.Ttl("foo"), std::nullopt);
}

TEST(HashTableTest, RehashKeepsAllRecords) {
  s21::HashTable store;
  for (int i = 0; i < 500; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }
  ASSERT_EQ(store.Size(), 500u);
  for (int i = 0; i < 500; ++i) {
    EXPECT_EQ(store.Get(std::to_string(i)),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }
}

TEST(HashTableTest, EraseThenReinsertSameKey) {
  s21::HashTable store;
  store.Set("foo", tests::MakeStudent("Vasilev", "Ivan", 2000, "Moscow", 55));
  ASSERT_TRUE(store.Del("foo"));
  store.Set("foo", tests::MakeStudent("Petrov", "Petr", 2001, "Tver", 42));
  EXPECT_EQ(store.Size(), 1u);
  EXPECT_EQ(store.Get("foo"),
            tests::MakeStudent("Petrov", "Petr", 2001, "Tver", 42));
}

TEST(HashTableTest, ExpiredEntriesPurgedFromBuckets) {
  s21::HashTable store;
  for (int i = 0; i < 50; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i),
              std::chrono::seconds{1});
  }
  std::this_thread::sleep_for(std::chrono::milliseconds{1150});
  EXPECT_EQ(store.Keys().size(), 0u);
  EXPECT_EQ(store.Size(), 0u);

  for (int i = 0; i < 500; ++i) {
    store.Set(std::to_string(i),
              tests::MakeStudent("Last", "First", 2000, "City", i));
  }
  EXPECT_EQ(store.Size(), 500u);
}

}  // namespace
